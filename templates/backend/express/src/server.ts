import { createApp } from "./app.js";
{% if postgres %}
import { connect } from "./postgres.js";
{% endif %}
{% if mysql %}
import { connect } from "./mysql.js";
{% endif %}
{% if not db_server %}
import { MemoryTodoRepository } from "./todos.js";
{% endif %}

const port = Number(process.env.PORT ?? 8080);
{% if db_server %}
const databaseUrl = process.env.DATABASE_URL ?? "{% db_url_local %}";

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
{% if db_server %}
    server.close(() => void close().then(() => process.exit(0)));
{% else %}
    server.close(() => process.exit(0));
{% endif %}
  });
}
