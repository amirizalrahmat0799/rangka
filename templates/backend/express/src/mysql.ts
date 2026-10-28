{% if mysql %}
import { readdir, readFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import mysql, { type Pool, type ResultSetHeader, type RowDataPacket } from "mysql2/promise";
import type { Todo, TodoRepository } from "./todos.js";

const migrationsDir = fileURLToPath(new URL("../migrations/", import.meta.url));

/**
 * Applies the SQL files in migrations/ that haven't run yet, in name order (one statement per file).
 * MySQL commits DDL statements on its own, so there is no transaction around them.
 * A small stand-in for a migration tool; switch to Knex or a dedicated tool when you need rollbacks.
 */
async function migrate(pool: Pool) {
  await pool.query(
    "CREATE TABLE IF NOT EXISTS schema_migrations (name VARCHAR(255) PRIMARY KEY, applied_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)",
  );
  const [applied] = await pool.query<RowDataPacket[]>("SELECT name FROM schema_migrations");
  const done = new Set(applied.map((r) => r.name as string));
  const files = (await readdir(migrationsDir)).filter((f) => f.endsWith(".sql")).sort();
  for (const file of files) {
    if (done.has(file)) continue;
    await pool.query(await readFile(migrationsDir + file, "utf8"));
    await pool.query("INSERT INTO schema_migrations (name) VALUES (?)", [file]);
    console.log(`applied migration ${file}`);
  }
}

type TodoRow = RowDataPacket & { id: number; title: string; done: number };

// MySQL has no real boolean type: BOOLEAN columns come back as 0 or 1
const toTodo = (r: TodoRow): Todo => ({ id: Number(r.id), title: r.title, done: Boolean(r.done) });

export class MySqlTodoRepository implements TodoRepository {
  constructor(private readonly pool: Pool) {}

  async list(): Promise<Todo[]> {
    const [rows] = await this.pool.query<TodoRow[]>("SELECT id, title, done FROM todos ORDER BY id");
    return rows.map(toTodo);
  }

  async create(title: string): Promise<Todo> {
    const [result] = await this.pool.execute<ResultSetHeader>("INSERT INTO todos (title) VALUES (?)", [title]);
    return { id: result.insertId, title, done: false };
  }

  async setDone(id: number, done: boolean): Promise<Todo | undefined> {
    // read the row back rather than trusting affectedRows, which can be 0 when the value didn't change
    await this.pool.execute("UPDATE todos SET done = ? WHERE id = ?", [done, id]);
    const [rows] = await this.pool.execute<TodoRow[]>("SELECT id, title, done FROM todos WHERE id = ?", [id]);
    return rows.length ? toTodo(rows[0]) : undefined;
  }

  async remove(id: number): Promise<boolean> {
    const [result] = await this.pool.execute<ResultSetHeader>("DELETE FROM todos WHERE id = ?", [id]);
    return result.affectedRows === 1;
  }
}

/** Opens a connection pool, waits for the database (it may still be starting in Docker), and migrates it. */
export async function connect(url: string) {
  const pool = mysql.createPool({ uri: url, charset: "utf8mb4" });
  for (let attempt = 1; ; attempt++) {
    try {
      await pool.query("SELECT 1");
      break;
    } catch (err) {
      if (attempt === 60) throw err;
      console.log(`waiting for the database (${attempt})...`);
      await new Promise((resolve) => setTimeout(resolve, 1000));
    }
  }
  await migrate(pool);
  return { todos: new MySqlTodoRepository(pool), close: () => pool.end() };
}
{% endif %}
