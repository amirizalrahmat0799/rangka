{% if postgres %}
import { readdir, readFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import pg from "pg";
import type { Todo, TodoRepository } from "./todos.js";

const migrationsDir = fileURLToPath(new URL("../migrations/", import.meta.url));

// BIGINT columns arrive as strings by default (they can exceed 2^53); ids here never will, so use numbers.
pg.types.setTypeParser(pg.types.builtins.INT8, (value) => Number(value));

/**
 * Applies the SQL files in migrations/ that haven't run yet, in name order, each in its own transaction.
 * A small stand-in for a migration tool; switch to node-pg-migrate or Knex when you need rollbacks.
 */
async function migrate(pool: pg.Pool) {
  await pool.query("CREATE TABLE IF NOT EXISTS schema_migrations (name TEXT PRIMARY KEY, applied_at TIMESTAMPTZ NOT NULL DEFAULT now())");
  const done = new Set((await pool.query<{ name: string }>("SELECT name FROM schema_migrations")).rows.map((r) => r.name));
  const files = (await readdir(migrationsDir)).filter((f) => f.endsWith(".sql")).sort();
  for (const file of files) {
    if (done.has(file)) continue;
    const sql = await readFile(migrationsDir + file, "utf8");
    const client = await pool.connect();
    try {
      await client.query("BEGIN");
      await client.query(sql);
      await client.query("INSERT INTO schema_migrations (name) VALUES ($1)", [file]);
      await client.query("COMMIT");
      console.log(`applied migration ${file}`);
    } catch (err) {
      await client.query("ROLLBACK");
      throw err;
    } finally {
      client.release();
    }
  }
}

export class PostgresTodoRepository implements TodoRepository {
  constructor(private readonly pool: pg.Pool) {}

  async list(): Promise<Todo[]> {
    const { rows } = await this.pool.query<Todo>("SELECT id, title, done FROM todos ORDER BY id");
    return rows;
  }

  async create(title: string): Promise<Todo> {
    const { rows } = await this.pool.query<Todo>(
      "INSERT INTO todos (title) VALUES ($1) RETURNING id, title, done",
      [title],
    );
    return rows[0];
  }

  async setDone(id: number, done: boolean): Promise<Todo | undefined> {
    const { rows } = await this.pool.query<Todo>(
      "UPDATE todos SET done = $2 WHERE id = $1 RETURNING id, title, done",
      [id, done],
    );
    return rows[0];
  }

  async remove(id: number): Promise<boolean> {
    const { rowCount } = await this.pool.query("DELETE FROM todos WHERE id = $1", [id]);
    return rowCount === 1;
  }
}

/** Opens a connection pool, waits for the database (it may still be starting in Docker), and migrates it. */
export async function connect(url: string) {
  const pool = new pg.Pool({ connectionString: url });
  for (let attempt = 1; ; attempt++) {
    try {
      await pool.query("SELECT 1");
      break;
    } catch (err) {
      if (attempt === 30) throw err;
      console.log(`waiting for the database (${attempt})...`);
      await new Promise((resolve) => setTimeout(resolve, 1000));
    }
  }
  await migrate(pool);
  return { todos: new PostgresTodoRepository(pool), close: () => pool.end() };
}
{% endif %}
