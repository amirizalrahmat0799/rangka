// The functions the web page calls. Built for wasm32-wasi as a "reactor" (a library, no main).
//
// Results are returned as a pointer to a buffer laid out as [uint32 length][bytes]; the page reads it and then
// frees it with rangka_free. Options come in as "key=value" lines (see options_from_text).

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#include "core/catalog.hpp"
#include "core/generator.hpp"
#include "core/zip.hpp"

#define EXPORT(name) extern "C" __attribute__((export_name(name)))

namespace {

using namespace rangka;

uint8_t* to_buffer(const void* data, size_t size) {
    auto* out = static_cast<uint8_t*>(std::malloc(size + 4));
    uint32_t n = static_cast<uint32_t>(size);
    std::memcpy(out, &n, 4);  // wasm is little-endian
    if (size) std::memcpy(out + 4, data, size);
    return out;
}

uint8_t* to_buffer(const std::string& s) { return to_buffer(s.data(), s.size()); }

void json_string(std::string& out, std::string_view s) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    out += '"';
}

void json_choices(std::string& out, std::span<const Choice> choices) {
    out += '[';
    bool first = true;
    for (const auto& c : choices) {
        if (!first) out += ',';
        first = false;
        out += "{\"id\":";
        json_string(out, c.id);
        out += ",\"label\":";
        json_string(out, c.label);
        out += ",\"language\":";
        json_string(out, c.language);
        out += ",\"description\":";
        json_string(out, c.description);
        out += '}';
    }
    out += ']';
}

std::string error_json(const std::string& message) {
    std::string out = "{\"ok\":false,\"error\":";
    json_string(out, message);
    out += '}';
    return out;
}

}  // namespace

EXPORT("rangka_alloc") void* rangka_alloc(size_t size) { return std::malloc(size); }
EXPORT("rangka_free") void rangka_free(void* p) { std::free(p); }

/// {"version", "frontends", "backends", "databases", "versions"}
EXPORT("rangka_catalog") uint8_t* rangka_catalog() {
    std::string out = "{\"version\":";
    json_string(out, kRangkaVersion);
    out += ",\"frontends\":";
    json_choices(out, frontend_choices());
    out += ",\"backends\":";
    json_choices(out, backend_choices());
    out += ",\"databases\":";
    json_choices(out, database_choices());
    out += ",\"versions\":[";
    bool first = true;
    for (const auto& v : versions()) {
        if (!first) out += ',';
        first = false;
        out += "{\"name\":";
        json_string(out, v.name);
        out += ",\"version\":";
        json_string(out, v.version);
        out += '}';
    }
    out += "]}";
    return to_buffer(out);
}

/// {"ok": true, "name", "files": [{"path", "content", "executable"}], "nextSteps": [...]} or {"ok": false, "error"}
EXPORT("rangka_generate") uint8_t* rangka_generate(const char* options, size_t length) {
    auto opts = options_from_text(std::string_view(options, length));
    if (!opts) return to_buffer(error_json(opts.error()));
    auto project = generate(*opts);
    if (!project) return to_buffer(error_json(project.error()));

    std::string out = "{\"ok\":true,\"name\":";
    json_string(out, project->name);
    out += ",\"files\":[";
    bool first = true;
    for (const auto& f : project->files) {
        if (!first) out += ',';
        first = false;
        out += "{\"path\":";
        json_string(out, f.path);
        out += ",\"content\":";
        json_string(out, f.content);
        out += f.executable ? ",\"executable\":true}" : ",\"executable\":false}";
    }
    out += "],\"nextSteps\":[";
    first = true;
    for (const auto& step : project->next_steps) {
        if (!first) out += ',';
        first = false;
        json_string(out, step);
    }
    out += "]}";
    return to_buffer(out);
}

/// The project as a .zip, or error JSON (which starts with '{'; a zip starts with "PK").
EXPORT("rangka_zip") uint8_t* rangka_zip(const char* options, size_t length, double unix_time) {
    auto opts = options_from_text(std::string_view(options, length));
    if (!opts) return to_buffer(error_json(opts.error()));
    auto project = generate(*opts);
    if (!project) return to_buffer(error_json(project.error()));
    auto zip = make_zip(*project, static_cast<int64_t>(unix_time));
    return to_buffer(zip.data(), zip.size());
}
