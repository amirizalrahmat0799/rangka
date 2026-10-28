import { createApp } from "./app.js";
{% if postgres %}
import { connect } from "./postgres.js";
{% else %}
import { MemoryTodoRepository } from "./todos.js";
{% endif %}

const port = Number(process.env.PORT ?? 8080);
{% if postgres %}
const databaseUrl = process.env.DATABASE_URL ?? "postgres://{% db_user %}:{% db_password %}@localhost:5432/{% db_name %}";

const { todos, close } = await connect(databaseUrl);
{% else %}
const todos = new MemoryTodoRepository();
{% endif %}
const server = createApp(todos).listen(port, () => {
  console.log(`{% name %} API listening on http://localhost:${port}`);
});

// Docker sends SIGTERM on `docker compose down`: finish open requests, then exit.
for (const signal of ["SIGTERM", "SIGINT"] as const) {
  process.on(signal, () => {
{% if postgres %}
    server.close(() => void close().then(() => process.exit(0)));
{% else %}
    server.close(() => process.exit(0));
{% endif %}
  });
}
