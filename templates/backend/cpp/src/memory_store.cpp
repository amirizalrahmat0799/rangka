#include "store.hpp"

std::vector<Todo> MemoryStore::list() {
    std::lock_guard lock(mutex_);
    std::vector<Todo> out;
    for (const auto& [id, todo] : todos_) out.push_back(todo);
    return out;
}

Todo MemoryStore::create(const std::string& title) {
    std::lock_guard lock(mutex_);
    Todo todo{next_id_++, title, false};
    todos_[todo.id] = todo;
    return todo;
}

std::optional<Todo> MemoryStore::set_done(std::int64_t id, bool done) {
    std::lock_guard lock(mutex_);
    auto it = todos_.find(id);
    if (it == todos_.end()) return std::nullopt;
    it->second.done = done;
    return it->second;
}

bool MemoryStore::remove(std::int64_t id) {
    std::lock_guard lock(mutex_);
    return todos_.erase(id) == 1;
}
