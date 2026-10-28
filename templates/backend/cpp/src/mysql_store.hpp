{% if mysql %}
#pragma once

#include <mutex>
#include <string>

#include <mysql.h>

#include "store.hpp"

/// Keeps todos in MySQL through the MySQL C API. One connection shared behind a mutex: simple, and plenty
/// for a start; use a pool when you need more throughput.
class MySqlStore : public Store {
public:
    /// Connects to mysql://user:password@host:port/database (waiting for the database to come up, as it may
    /// still be starting in Docker) and migrates it. Throws std::runtime_error when it can't.
    explicit MySqlStore(const std::string& url);
    ~MySqlStore() override;
    MySqlStore(const MySqlStore&) = delete;
    MySqlStore& operator=(const MySqlStore&) = delete;

    std::vector<Todo> list() override;
    Todo create(const std::string& title) override;
    std::optional<Todo> set_done(std::int64_t id, bool done) override;
    bool remove(std::int64_t id) override;

private:
    struct Settings {
        std::string host, user, password, database;
        unsigned port = 3306;
    };

    bool open();
    void migrate();
    MYSQL* connection();                              // reconnects if the connection dropped
    void execute(const std::string& sql);              // throws on error
    std::vector<Todo> query(const std::string& sql);  // SELECT id, title, done ...
    std::string quote(const std::string& text);        // escaped and quoted, safe to put in SQL

    std::mutex mutex_;
    Settings settings_;
    MYSQL* conn_ = nullptr;
};
{% endif %}
