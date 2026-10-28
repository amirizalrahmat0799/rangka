#pragma once
// The todo model and where todos are kept. The API only talks to Store, so the storage can change freely.

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

constexpr std::size_t kMaxTitleLength = 200;

struct Todo {
    std::int64_t id = 0;
    std::string title;
    bool done = false;
};

// {"id": 1, "title": "...", "done": false}, keys in that order
inline void to_json(nlohmann::ordered_json& j, const Todo& t) { j = {{"id", t.id}, {"title", t.title}, {"done", t.done}}; }

class Store {
public:
    virtual ~Store() = default;
    virtual std::vector<Todo> list() = 0;
    virtual Todo create(const std::string& title) = 0;
    /// Empty when there is no todo with this id.
    virtual std::optional<Todo> set_done(std::int64_t id, bool done) = 0;
    /// False when there is no todo with this id.
    virtual bool remove(std::int64_t id) = 0;
};

/// Keeps todos in memory: used by the tests{% if not db_server %}, and by the app until it gets a database{% endif %}.
/// Thread safe, since the HTTP server handles requests on several threads.
class MemoryStore : public Store {
public:
    std::vector<Todo> list() override;
    Todo create(const std::string& title) override;
    std::optional<Todo> set_done(std::int64_t id, bool done) override;
    bool remove(std::int64_t id) override;

private:
    std::mutex mutex_;
    std::map<std::int64_t, Todo> todos_;  // ordered by id
    std::int64_t next_id_ = 1;
};
