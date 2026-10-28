{% if mysql %}
#include "mysql_store.hpp"

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
    id         BIGINT AUTO_INCREMENT PRIMARY KEY,
    title      VARCHAR(200) NOT NULL,
    done       BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) CHARACTER SET utf8mb4
)sql"},
};

using ResultPtr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

// mysql://user:password@host:port/database
std::string between(const std::string& s, size_t from, size_t to) { return s.substr(from, to - from); }

}  // namespace

MySqlStore::MySqlStore(const std::string& url) {
    const std::string prefix = "mysql://";
    auto at = url.rfind('@');
    if (url.rfind(prefix, 0) != 0 || at == std::string::npos) {
        throw std::runtime_error("expected mysql://user:password@host:port/database, got " + url);
    }
    std::string credentials = between(url, prefix.size(), at);
    std::string location = url.substr(at + 1);
    auto colon = credentials.find(':');
    settings_.user = credentials.substr(0, colon);
    settings_.password = colon == std::string::npos ? "" : credentials.substr(colon + 1);
    auto slash = location.find('/');
    std::string host_port = location.substr(0, slash);
    settings_.database = slash == std::string::npos ? "" : location.substr(slash + 1);
    auto port_colon = host_port.find(':');
    settings_.host = host_port.substr(0, port_colon);
    if (port_colon != std::string::npos) settings_.port = static_cast<unsigned>(std::stoul(host_port.substr(port_colon + 1)));

    for (int attempt = 1;; ++attempt) {
        if (open()) break;
        std::string error = mysql_error(conn_);
        mysql_close(conn_);
        conn_ = nullptr;
        if (attempt == 60) throw std::runtime_error(error);
        std::fprintf(stderr, "waiting for the database (%d)...\n", attempt);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    migrate();
}

MySqlStore::~MySqlStore() {
    if (conn_) mysql_close(conn_);
}

bool MySqlStore::open() {
    conn_ = mysql_init(nullptr);
    mysql_options(conn_, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    return mysql_real_connect(conn_, settings_.host.c_str(), settings_.user.c_str(), settings_.password.c_str(),
                              settings_.database.c_str(), settings_.port, nullptr, 0) != nullptr;
}

MYSQL* MySqlStore::connection() {
    if (mysql_ping(conn_) != 0) {
        mysql_close(conn_);
        if (!open()) throw std::runtime_error(mysql_error(conn_));
    }
    return conn_;
}

void MySqlStore::execute(const std::string& sql) {
    MYSQL* c = connection();
    if (mysql_real_query(c, sql.data(), sql.size()) != 0) throw std::runtime_error(mysql_error(c));
    // drain any result so the connection is ready for the next statement
    ResultPtr discard(mysql_store_result(c), &mysql_free_result);
}

std::vector<Todo> MySqlStore::query(const std::string& sql) {
    MYSQL* c = connection();
    if (mysql_real_query(c, sql.data(), sql.size()) != 0) throw std::runtime_error(mysql_error(c));
    ResultPtr result(mysql_store_result(c), &mysql_free_result);
    if (!result) throw std::runtime_error(mysql_error(c));
    std::vector<Todo> todos;
    while (MYSQL_ROW row = mysql_fetch_row(result.get())) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());
        todos.push_back(Todo{std::stoll(row[0]), std::string(row[1], lengths[1]), row[2][0] == '1'});
    }
    return todos;
}

std::string MySqlStore::quote(const std::string& text) {
    std::string out(text.size() * 2 + 1, '\0');
    auto n = mysql_real_escape_string(connection(), out.data(), text.data(), static_cast<unsigned long>(text.size()));
    out.resize(n);
    return "'" + out + "'";
}

void MySqlStore::migrate() {
    std::lock_guard lock(mutex_);
    execute("CREATE TABLE IF NOT EXISTS schema_migrations (name VARCHAR(255) PRIMARY KEY, "
            "applied_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)");
    for (const auto& [name, sql] : kMigrations) {
        MYSQL* c = connection();
        std::string check = "SELECT 1 FROM schema_migrations WHERE name = " + quote(name);
        if (mysql_real_query(c, check.data(), check.size()) != 0) throw std::runtime_error(mysql_error(c));
        ResultPtr found(mysql_store_result(c), &mysql_free_result);
        if (found && mysql_num_rows(found.get()) > 0) continue;
        // MySQL commits DDL statements on its own, so there is no transaction here
        execute(sql);
        execute("INSERT INTO schema_migrations (name) VALUES (" + quote(name) + ")");
        std::printf("applied migration %s\n", name.c_str());
    }
}

// Strings go through quote() (mysql_real_escape_string) and numbers through std::to_string, so user input
// can't change the SQL. For more complex queries, switch to prepared statements (mysql_stmt_*).

std::vector<Todo> MySqlStore::list() {
    std::lock_guard lock(mutex_);
    return query("SELECT id, title, done FROM todos ORDER BY id");
}

Todo MySqlStore::create(const std::string& title) {
    std::lock_guard lock(mutex_);
    execute("INSERT INTO todos (title) VALUES (" + quote(title) + ")");
    return Todo{static_cast<std::int64_t>(mysql_insert_id(conn_)), title, false};
}

std::optional<Todo> MySqlStore::set_done(std::int64_t id, bool done) {
    std::lock_guard lock(mutex_);
    execute("UPDATE todos SET done = " + std::string(done ? "TRUE" : "FALSE") + " WHERE id = " + std::to_string(id));
    auto rows = query("SELECT id, title, done FROM todos WHERE id = " + std::to_string(id));
    if (rows.empty()) return std::nullopt;
    return rows[0];
}

bool MySqlStore::remove(std::int64_t id) {
    std::lock_guard lock(mutex_);
    execute("DELETE FROM todos WHERE id = " + std::to_string(id));
    return mysql_affected_rows(conn_) == 1;
}
{% endif %}
