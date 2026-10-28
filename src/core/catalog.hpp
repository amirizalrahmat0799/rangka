#pragma once
// What rangka can generate: the frontends, backends and databases, with the labels shown in the CLI and on the
// web page, and the versions of everything the generated projects depend on.

#include <optional>
#include <span>
#include <string_view>

namespace rangka {

enum class Frontend { None, React, Vue };
enum class Backend { None, Spring, Express, FastApi, Go, Cpp };
enum class Database { None, Postgres };

struct Choice {
    std::string_view id;           // used in flags and templates: "spring"
    std::string_view label;        // "Spring Boot"
    std::string_view language;     // "Java"
    std::string_view description;  // one line for menus
};

std::span<const Choice> frontend_choices();  // includes "none"
std::span<const Choice> backend_choices();   // includes "none"
std::span<const Choice> database_choices();  // includes "none"

const Choice& choice(Frontend f);
const Choice& choice(Backend b);
const Choice& choice(Database d);

std::optional<Frontend> parse_frontend(std::string_view id);
std::optional<Backend> parse_backend(std::string_view id);
std::optional<Database> parse_database(std::string_view id);

struct Version {
    std::string_view variable;  // template variable: "v_spring_boot"
    std::string_view name;      // "Spring Boot"
    std::string_view version;   // "4.1.1"
};

/// Versions used by the templates, in one place so they're easy to bump.
std::span<const Version> versions();

inline constexpr std::string_view kRangkaVersion = RANGKA_VERSION;

}  // namespace rangka
