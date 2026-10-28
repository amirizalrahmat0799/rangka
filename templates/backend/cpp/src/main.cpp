#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

#include "api.hpp"
{% if postgres %}
#include "postgres_store.hpp"
{% endif %}

namespace {
httplib::Server* g_server = nullptr;

// Docker sends SIGTERM on `docker compose down`: stop accepting requests and exit.
void on_signal(int) {
    if (g_server) g_server->stop();
}

std::string env(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return value && *value ? value : fallback;
}
}  // namespace

int main() {
    int port = std::stoi(env("PORT", "8080"));

{% if postgres %}
    std::unique_ptr<Store> store;
    try {
        store = std::make_unique<PostgresStore>(
            env("DATABASE_URL", "postgres://{% db_user %}:{% db_password %}@localhost:5432/{% db_name %}"));
    } catch (const std::exception& e) {
        std::fprintf(stderr, "database: %s\n", e.what());
        return 1;
    }
{% else %}
    auto store = std::make_unique<MemoryStore>();
{% endif %}

    httplib::Server server;
    register_routes(server, *store);
    g_server = &server;
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    std::printf("{% name %} API listening on http://localhost:%d\n", port);
    std::fflush(stdout);
    if (!server.listen("0.0.0.0", port)) {
        std::fprintf(stderr, "can't listen on port %d\n", port);
        return 1;
    }
    return 0;
}
