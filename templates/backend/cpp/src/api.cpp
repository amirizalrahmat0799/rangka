#include "api.hpp"

#include <charconv>
#include <cstdio>
#include <exception>
#include <functional>
#include <string>

using json = nlohmann::ordered_json;

namespace {

// every error is {"error": "..."}, like the other backends rangka generates
void send(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

void send_error(httplib::Response& res, int status, const std::string& message) {
    send(res, status, {{"error", message}});
}

std::optional<std::int64_t> parse_id(const std::string& text) {
    std::int64_t id = 0;
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), id);
    if (ec != std::errc() || end != text.data() + text.size()) return std::nullopt;
    return id;
}

std::string trim(const std::string& s) {
    const char* space = " \t\r\n";
    auto first = s.find_first_not_of(space);
    if (first == std::string::npos) return {};
    return s.substr(first, s.find_last_not_of(space) - first + 1);
}

std::size_t utf8_length(const std::string& s) {
    std::size_t n = 0;
    for (unsigned char c : s) n += (c & 0xC0) != 0x80;  // count everything except continuation bytes
    return n;
}

// Runs a handler, turning a malformed body into 400 and anything unexpected into 500.
httplib::Server::Handler guarded(std::function<void(const httplib::Request&, httplib::Response&)> fn) {
    return [fn = std::move(fn)](const httplib::Request& req, httplib::Response& res) {
        try {
            fn(req, res);
        } catch (const json::parse_error&) {
            send_error(res, 400, "the request body is not valid JSON");
        } catch (const std::exception& e) {
            std::fprintf(stderr, "error: %s\n", e.what());
            send_error(res, 500, "internal server error");
        }
    };
}

}  // namespace

void register_routes(httplib::Server& server, Store& store) {
    server.Get("/api/health", guarded([](const httplib::Request&, httplib::Response& res) {
        send(res, 200, {{"status", "ok"}});
    }));

    server.Get("/api/todos", guarded([&store](const httplib::Request&, httplib::Response& res) {
        send(res, 200, store.list());
    }));

    server.Post("/api/todos", guarded([&store](const httplib::Request& req, httplib::Response& res) {
        json body = json::parse(req.body);
        std::string title;
        if (body.is_object() && body.contains("title") && body["title"].is_string()) {
            title = trim(body["title"].get<std::string>());
        }
        if (title.empty()) return send_error(res, 400, "title is required");
        if (utf8_length(title) > kMaxTitleLength) {
            return send_error(res, 400, "title must be at most " + std::to_string(kMaxTitleLength) + " characters");
        }
        send(res, 201, store.create(title));
    }));

    server.Patch(R"(/api/todos/([^/]+))", guarded([&store](const httplib::Request& req, httplib::Response& res) {
        json body = json::parse(req.body);
        if (!body.is_object() || !body.contains("done") || !body["done"].is_boolean()) {
            return send_error(res, 400, "done must be true or false");
        }
        auto id = parse_id(req.matches[1]);
        auto todo = id ? store.set_done(*id, body["done"].get<bool>()) : std::nullopt;
        if (!todo) return send_error(res, 404, "todo not found");
        send(res, 200, *todo);
    }));

    server.Delete(R"(/api/todos/([^/]+))", guarded([&store](const httplib::Request& req, httplib::Response& res) {
        auto id = parse_id(req.matches[1]);
        if (!id || !store.remove(*id)) return send_error(res, 404, "todo not found");
        res.status = 204;
    }));

    // unknown routes: JSON instead of the default empty body
    server.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        if (res.status == 404 && res.body.empty()) send_error(res, 404, "not found");
    });
}
