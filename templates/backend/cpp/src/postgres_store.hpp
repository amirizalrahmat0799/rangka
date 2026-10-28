{% if postgres %}
#pragma once

#include <mutex>
#include <string>

#include <libpq-fe.h>

#include "store.hpp"

/// Keeps todos in PostgreSQL through libpq. One connection shared behind a mutex: simple, and plenty for a
/// start; use a pool (or libpqxx) when you need more throughput.
class PostgresStore : public Store {
public:
    /// Connects (waiting for the database to come up, as it may still be starting in Docker) and migrates it.
    /// Throws std::runtime_error when it can't.
    explicit PostgresStore(const std::string& url);
    ~PostgresStore() override;
    PostgresStore(const PostgresStore&) = delete;
    PostgresStore& operator=(const PostgresStore&) = delete;

    std::vector<Todo> list() override;
    Todo create(const std::string& title) override;
    std::optional<Todo> set_done(std::int64_t id, bool done) override;
    bool remove(std::int64_t id) override;

private:
    void migrate();
    PGconn* connection();  // reconnects if the connection dropped

    std::mutex mutex_;
    PGconn* conn_ = nullptr;
};
{% endif %}
