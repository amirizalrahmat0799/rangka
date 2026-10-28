{% if postgres %}
#include "postgres_store.hpp"

#include <chrono>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {

// Applied in order, once each (recorded in schema_migrations). Add new ones at the end; never edit old ones.
const std::vector<std::pair<std::string, std::string>> kMigrations = {
    {"001_create_todos", R"sql(
CREATE TABLE todos (
    id         BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    title      VARCHAR(200) NOT NULL,
    done       BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
)sql"},
};

using ResultPtr = std::unique_ptr<PGresult, decltype(&PQclear)>;

// Runs a parameterised query (parameters are sent separately, so no SQL injection) and checks the result.
ResultPtr exec(PGconn* conn, const char* sql, const std::vector<std::string>& params = {}) {
    std::vector<const char*> values;
    for (const auto& p : params) values.push_back(p.c_str());
    ResultPtr result(PQexecParams(conn, sql, static_cast<int>(values.size()), nullptr, values.data(), nullptr,
                                  nullptr, 0),
                     &PQclear);
    auto status = PQresultStatus(result.get());
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
        throw std::runtime_error(PQerrorMessage(conn));
    }
    return result;
}

Todo row_to_todo(PGresult* r, int row) {
    return Todo{std::stoll(PQgetvalue(r, row, 0)), PQgetvalue(r, row, 1), PQgetvalue(r, row, 2)[0] == 't'};
}

}  // namespace

PostgresStore::PostgresStore(const std::string& url) {
    for (int attempt = 1;; ++attempt) {
        conn_ = PQconnectdb(url.c_str());
        if (PQstatus(conn_) == CONNECTION_OK) break;
        std::string error = PQerrorMessage(conn_);
        PQfinish(conn_);
        conn_ = nullptr;
        if (attempt == 30) throw std::runtime_error(error);
        std::fprintf(stderr, "waiting for the database (%d)...\n", attempt);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    migrate();
}

PostgresStore::~PostgresStore() {
    if (conn_) PQfinish(conn_);
}

PGconn* PostgresStore::connection() {
    if (PQstatus(conn_) != CONNECTION_OK) PQreset(conn_);
    return conn_;
}

void PostgresStore::migrate() {
    std::lock_guard lock(mutex_);
    exec(conn_, "CREATE TABLE IF NOT EXISTS schema_migrations (name TEXT PRIMARY KEY, "
                "applied_at TIMESTAMPTZ NOT NULL DEFAULT now())");
    for (const auto& [name, sql] : kMigrations) {
        if (PQntuples(exec(conn_, "SELECT 1 FROM schema_migrations WHERE name = $1", {name}).get()) > 0) continue;
        exec(conn_, "BEGIN");
        try {
            exec(conn_, sql.c_str());
            exec(conn_, "INSERT INTO schema_migrations (name) VALUES ($1)", {name});
            exec(conn_, "COMMIT");
        } catch (...) {
            PQclear(PQexec(conn_, "ROLLBACK"));
            throw;
        }
        std::printf("applied migration %s\n", name.c_str());
    }
}

std::vector<Todo> PostgresStore::list() {
    std::lock_guard lock(mutex_);
    auto r = exec(connection(), "SELECT id, title, done FROM todos ORDER BY id");
    std::vector<Todo> todos;
    for (int i = 0; i < PQntuples(r.get()); ++i) todos.push_back(row_to_todo(r.get(), i));
    return todos;
}

Todo PostgresStore::create(const std::string& title) {
    std::lock_guard lock(mutex_);
    auto r = exec(connection(), "INSERT INTO todos (title) VALUES ($1) RETURNING id, title, done", {title});
    return row_to_todo(r.get(), 0);
}

std::optional<Todo> PostgresStore::set_done(std::int64_t id, bool done) {
    std::lock_guard lock(mutex_);
    auto r = exec(connection(), "UPDATE todos SET done = $2 WHERE id = $1 RETURNING id, title, done",
                  {std::to_string(id), done ? "true" : "false"});
    if (PQntuples(r.get()) == 0) return std::nullopt;
    return row_to_todo(r.get(), 0);
}

bool PostgresStore::remove(std::int64_t id) {
    std::lock_guard lock(mutex_);
    auto r = exec(connection(), "DELETE FROM todos WHERE id = $1", {std::to_string(id)});
    return std::string(PQcmdTuples(r.get())) == "1";
}
{% endif %}
