#include "core/catalog.hpp"

#include <array>

namespace rangka {
namespace {

constexpr std::array<Choice, 3> kFrontends = {{
    {"react", "React", "TypeScript", "React 19, Vite and TypeScript"},
    {"vue", "Vue", "TypeScript", "Vue 3, Vite and TypeScript"},
    {"none", "None", "", "no frontend"},
}};

constexpr std::array<Choice, 6> kBackends = {{
    {"spring", "Spring Boot", "Java", "Spring Boot 4 on Java 21, JDBC and Flyway"},
    {"express", "Express", "TypeScript", "Express 5 on Node.js 24, node-postgres"},
    {"fastapi", "FastAPI", "Python", "FastAPI with SQLAlchemy on Python 3.13"},
    {"go", "net/http", "Go", "Go's standard library router and pgx"},
    {"cpp", "cpp-httplib", "C++", "C++20 with cpp-httplib, nlohmann/json and libpq"},
    {"none", "None", "", "no backend"},
}};

constexpr std::array<Choice, 2> kDatabases = {{
    {"postgres", "PostgreSQL", "", "PostgreSQL 17, with migrations"},
    {"none", "None", "", "data kept in memory"},
}};

// Bump versions here; every template reads them from these variables.
constexpr std::array<Version, 32> kVersions = {{
    // Java
    {"v_java", "Java", "21"},
    {"v_spring_boot", "Spring Boot", "4.1.1"},
    // TypeScript (backend and frontend)
    {"v_node", "Node.js", "24"},
    {"v_typescript", "TypeScript", "6.0.3"},
    {"v_types_node", "@types/node", "24.19.2"},
    {"v_express", "Express", "5.3.0"},
    {"v_types_express", "@types/express", "5.0.6"},
    {"v_pg", "node-postgres", "8.23.1"},
    {"v_types_pg", "@types/pg", "8.23.1"},
    {"v_tsx", "tsx", "4.23.15"},
    {"v_vitest", "Vitest", "5.0.5"},
    {"v_supertest", "SuperTest", "7.3.1"},
    {"v_types_supertest", "@types/supertest", "7.2.1"},
    {"v_vite", "Vite", "8.3.5"},
    {"v_react", "React", "19.3.1"},
    {"v_types_react", "@types/react", "19.3.0"},
    {"v_vite_react", "@vitejs/plugin-react", "6.1.2"},
    {"v_vue", "Vue", "3.5.44"},
    {"v_vite_vue", "@vitejs/plugin-vue", "6.0.10"},
    {"v_vue_tsc", "vue-tsc", "3.3.13"},
    // Python
    {"v_python", "Python", "3.13"},
    {"v_fastapi", "FastAPI", "0.162.0"},
    {"v_uvicorn", "Uvicorn", "0.54.0"},
    {"v_sqlalchemy", "SQLAlchemy", "2.1.6"},
    {"v_psycopg", "psycopg", "3.3.6"},
    {"v_pytest", "pytest", "9.2.0"},
    {"v_httpx2", "HTTPX 2", "2.16.0"},
    // Go
    {"v_go", "Go", "1.26"},
    {"v_pgx", "pgx", "5.11.0"},
    // C++
    {"v_httplib", "cpp-httplib", "0.46.0"},
    {"v_json", "nlohmann/json", "3.12.0"},
    // Database
    {"v_postgres", "PostgreSQL", "17"},
}};

template <typename E, size_t N>
const Choice& lookup(const std::array<Choice, N>& table, E value, std::initializer_list<E> order) {
    size_t i = 0;
    for (E e : order) {
        if (e == value) return table[i];
        ++i;
    }
    return table[N - 1];
}

}  // namespace

std::span<const Choice> frontend_choices() { return kFrontends; }
std::span<const Choice> backend_choices() { return kBackends; }
std::span<const Choice> database_choices() { return kDatabases; }
std::span<const Version> versions() { return kVersions; }

const Choice& choice(Frontend f) { return lookup(kFrontends, f, {Frontend::React, Frontend::Vue, Frontend::None}); }
const Choice& choice(Backend b) {
    return lookup(kBackends, b,
                  {Backend::Spring, Backend::Express, Backend::FastApi, Backend::Go, Backend::Cpp, Backend::None});
}
const Choice& choice(Database d) { return lookup(kDatabases, d, {Database::Postgres, Database::None}); }

std::optional<Frontend> parse_frontend(std::string_view id) {
    if (id == "react") return Frontend::React;
    if (id == "vue") return Frontend::Vue;
    if (id == "none" || id.empty()) return Frontend::None;
    return std::nullopt;
}

std::optional<Backend> parse_backend(std::string_view id) {
    if (id == "spring" || id == "spring-boot" || id == "java") return Backend::Spring;
    if (id == "express" || id == "node" || id == "typescript") return Backend::Express;
    if (id == "fastapi" || id == "python") return Backend::FastApi;
    if (id == "go" || id == "golang") return Backend::Go;
    if (id == "cpp" || id == "c++") return Backend::Cpp;
    if (id == "none" || id.empty()) return Backend::None;
    return std::nullopt;
}

std::optional<Database> parse_database(std::string_view id) {
    if (id == "postgres" || id == "postgresql") return Database::Postgres;
    if (id == "none" || id.empty()) return Database::None;
    return std::nullopt;
}

}  // namespace rangka
