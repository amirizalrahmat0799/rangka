# {% title %}

{% if fullstack %}
A {% frontend_label %} frontend and an API built with {% backend_label %} ({% backend_language %}){% if has_db %}, storing data in {% database_label %}{% endif %}: wired together and ready to run with one command.
{% endif %}
{% if has_backend and not has_frontend %}
An API built with {% backend_label %} ({% backend_language %}){% if has_db %}, storing data in {% database_label %}{% endif %}.
{% endif %}
{% if has_frontend and not has_backend %}
A {% frontend_label %} app built with Vite and TypeScript. Todos are kept in the browser.
{% endif %}
It starts as a small todo app, so every part already works end to end: replace the todos with your own features.

| Part | Built with | Folder |
|---|---|---|
{% if has_frontend %}
| Frontend | {% frontend_label %}, Vite, TypeScript | {% frontend_where %} |
{% endif %}
{% if has_backend %}
{% if spring %}
| Backend | Spring Boot {% v_spring_boot %}, Java {% v_java %}{% if has_db %}, JDBC, Flyway{% endif %} | {% backend_where %} |
{% endif %}
{% if express %}
| Backend | Express {% v_express %}, Node.js {% v_node %}, TypeScript{% if has_db %}, node-postgres{% endif %} | {% backend_where %} |
{% endif %}
{% if fastapi %}
| Backend | FastAPI, Python {% v_python %}{% if has_db %}, SQLAlchemy, psycopg{% endif %} | {% backend_where %} |
{% endif %}
{% if go %}
| Backend | Go {% v_go %}, standard library router{% if has_db %}, pgx{% endif %} | {% backend_where %} |
{% endif %}
{% if cpp %}
| Backend | C++20, cpp-httplib, nlohmann/json{% if has_db %}, libpq{% endif %}, CMake | {% backend_where %} |
{% endif %}
{% endif %}
{% if has_db %}
| Database | {% database_label %} {% v_postgres %} (Docker) | `docker-compose.yml` |
{% endif %}

## Run it
{% if multi %}

With Docker:

```bash
docker compose up --build
```

{% if has_frontend %}
Then open <http://localhost:3000>. The API is also on <http://localhost:8080/api/todos>.
{% else %}
Then try <http://localhost:8080/api/todos>.
{% endif %}

### Without Docker
{% if has_db %}

Start only the database in Docker:

```bash
docker compose up -d db
```
{% endif %}
{% endif %}
{% if has_backend %}

{% if fullstack %}
The backend, in one terminal:

{% endif %}
{% if multi and not fullstack %}
Then the API:

{% endif %}
```bash
{% if multi %}
cd backend
{% endif %}
{% if spring %}
mvn spring-boot:run
{% endif %}
{% if express %}
npm install
npm run dev
{% endif %}
{% if fastapi %}
python -m venv .venv
source .venv/bin/activate        # Windows: .venv\Scripts\activate
pip install -r requirements-dev.txt
uvicorn app.main:app --reload --port 8080
{% endif %}
{% if go %}
go mod tidy                      # downloads dependencies and writes go.sum
go run .
{% endif %}
{% if cpp %}
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/{% snake %}                  # Windows: build\Release\{% snake %}.exe
{% endif %}
```

It listens on <http://localhost:8080>.
{% endif %}
{% if has_frontend %}

{% if fullstack %}
The frontend, in another terminal:

{% endif %}
```bash
{% if multi %}
cd frontend
{% endif %}
npm install
npm run dev
```

Then open <http://localhost:5173>.{% if has_backend %} Vite forwards `/api` to the backend on port 8080.{% endif %}
{% endif %}
{% if has_backend %}

## API

| Method | Path | Body | Response |
|---|---|---|---|
| `GET` | `/api/health` | | `{"status": "ok"}` |
| `GET` | `/api/todos` | | `[{"id": 1, "title": "Buy milk", "done": false}]` |
| `POST` | `/api/todos` | `{"title": "Buy milk"}` | `201` and the new todo |
| `PATCH` | `/api/todos/{id}` | `{"done": true}` | the updated todo |
| `DELETE` | `/api/todos/{id}` | | `204` |

Errors come back as `{"error": "..."}` with status 400 (invalid input) or 404 (no such todo).

```bash
curl -X POST localhost:8080/api/todos -H "Content-Type: application/json" -d '{"title": "Buy milk"}'
```
{% endif %}

## Tests

```bash
{% if has_backend %}
{% if spring %}
{% if multi %}(cd backend && mvn verify){% else %}mvn verify{% endif %}
{% endif %}
{% if express %}
{% if multi %}(cd backend && npm test){% else %}npm test{% endif %}
{% endif %}
{% if fastapi %}
{% if multi %}(cd backend && python -m pytest){% else %}python -m pytest{% endif %}
{% endif %}
{% if go %}
{% if multi %}(cd backend && go test ./...){% else %}go test ./...{% endif %}
{% endif %}
{% if cpp %}
{% if multi %}(cd backend && ctest --test-dir build --output-on-failure){% else %}ctest --test-dir build --output-on-failure{% endif %}
{% endif %}
{% endif %}
{% if has_frontend %}
{% if multi %}(cd frontend && npm test){% else %}npm test{% endif %}
{% endif %}
```

{% if has_backend %}
{% if has_db %}
The backend tests check the API{% if spring %} against an in-memory H2 database running the real migrations{% endif %}{% if fastapi %} with an in-memory store, and the database code against SQLite{% endif %}{% if express or go or cpp %} with an in-memory store{% endif %}; the Docker Compose check in CI runs it against PostgreSQL.
{% else %}
The backend tests call the real API{% if spring %} on a random port{% endif %} with an in-memory store.
{% endif %}

{% endif %}
{% if ci %}
GitHub Actions runs all of this on every push ([.github/workflows/ci.yml](.github/workflows/ci.yml)){% if multi %}, then starts everything with Docker Compose and tries the API{% endif %}.

{% endif %}
## Where things are

| Path | What it is |
|---|---|
{% if has_backend %}
{% if spring %}
| `{% backend_prefix %}src/main/java/.../todo/` | controller, repository and error handling |
{% if has_db %}
| `{% backend_prefix %}src/main/resources/db/migration/` | database migrations (Flyway) |
{% endif %}
{% endif %}
{% if express %}
| `{% backend_prefix %}src/app.ts` | the routes |
| `{% backend_prefix %}src/todos.ts` | the model and the in-memory repository |
{% if has_db %}
| `{% backend_prefix %}src/postgres.ts` | the PostgreSQL repository and migration runner |
| `{% backend_prefix %}migrations/` | SQL migrations, applied at startup |
{% endif %}
{% endif %}
{% if fastapi %}
| `{% backend_prefix %}app/api.py` | the routes |
| `{% backend_prefix %}app/todos.py` | the model and the in-memory store |
{% if has_db %}
| `{% backend_prefix %}app/database.py` | the SQLAlchemy store |
{% endif %}
{% endif %}
{% if go %}
| `{% backend_prefix %}handler.go` | the routes |
| `{% backend_prefix %}todos.go` | the model and the in-memory store |
{% if has_db %}
| `{% backend_prefix %}postgres.go` | the PostgreSQL store and migration runner |
| `{% backend_prefix %}migrations/` | SQL migrations, embedded in the binary |
{% endif %}
{% endif %}
{% if cpp %}
| `{% backend_prefix %}src/api.cpp` | the routes |
| `{% backend_prefix %}src/store.hpp` | the model, the store interface and the in-memory store |
{% if has_db %}
| `{% backend_prefix %}src/postgres_store.cpp` | the PostgreSQL store and its migrations |
{% endif %}
{% endif %}
{% endif %}
{% if has_frontend %}
| `{% frontend_prefix %}src/App.{% if react %}tsx{% else %}vue{% endif %}` | the page |
| `{% frontend_prefix %}src/api.ts` | {% if has_backend %}calls to the backend{% else %}where todos are saved (this browser){% endif %} |
| `{% frontend_prefix %}src/todos.ts` | validation, shared by the page and its tests |
{% endif %}
{% if multi %}
| `docker-compose.yml` | everything, started together |
{% endif %}

---

Created with [rangka](https://github.com/amirizalrahmat0799/rangka) {% rangka_version %}.
