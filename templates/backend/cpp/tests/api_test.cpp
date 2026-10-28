// Runs the real routes on a local port with an in-memory store, and calls them over HTTP.

#include <cstdio>
#include <string>
#include <thread>

#include "api.hpp"

namespace {
int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            ++failures;                                                      \
            std::fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)

using json = nlohmann::ordered_json;
const char* kJson = "application/json";

json body(const httplib::Result& res) { return res && !res->body.empty() ? json::parse(res->body) : json(); }

}  // namespace

int main() {
    MemoryStore store;
    httplib::Server server;
    register_routes(server, store);
    int port = server.bind_to_any_port("127.0.0.1");
    std::thread thread([&] { server.listen_after_bind(); });
    server.wait_until_ready();
    httplib::Client client("127.0.0.1", port);

    // health
    auto res = client.Get("/api/health");
    CHECK(res && res->status == 200 && body(res) == json({{"status", "ok"}}));

    // create, complete, list, delete
    res = client.Post("/api/todos", R"({"title": "  Buy milk  "})", kJson);
    CHECK(res && res->status == 201);
    CHECK(body(res) == json({{"id", 1}, {"title", "Buy milk"}, {"done", false}}));

    res = client.Patch("/api/todos/1", R"({"done": true})", kJson);
    CHECK(res && res->status == 200 && body(res)["done"] == true);

    res = client.Get("/api/todos");
    CHECK(res && body(res) == json::parse(R"([{"id": 1, "title": "Buy milk", "done": true}])"));

    res = client.Delete("/api/todos/1");
    CHECK(res && res->status == 204);
    CHECK(body(client.Get("/api/todos")) == json::array());

    // validation
    for (const char* bad : {R"({})", R"({"title": "   "})", R"({"title": 42})"}) {
        res = client.Post("/api/todos", bad, kJson);
        CHECK(res && res->status == 400 && body(res)["error"] == "title is required");
    }
    res = client.Post("/api/todos", json({{"title", std::string(201, 'x')}}).dump(), kJson);
    CHECK(res && res->status == 400);
    res = client.Post("/api/todos", "{oops", kJson);
    CHECK(res && res->status == 400 && body(res)["error"].get<std::string>().find("JSON") != std::string::npos);

    // unknown todos and routes
    CHECK(client.Patch("/api/todos/99", R"({"done": true})", kJson)->status == 404);
    CHECK(client.Delete("/api/todos/99")->status == 404);
    CHECK(client.Delete("/api/todos/abc")->status == 404);
    res = client.Get("/api/nope");
    CHECK(res && res->status == 404 && body(res)["error"] == "not found");

    // done must be a boolean
    client.Post("/api/todos", R"({"title": "Walk"})", kJson);
    CHECK(client.Patch("/api/todos/2", R"({"done": "yes"})", kJson)->status == 400);

    server.stop();
    thread.join();
    std::printf("%s\n", failures == 0 ? "all API tests passed" : "API tests failed");
    return failures == 0 ? 0 : 1;
}
