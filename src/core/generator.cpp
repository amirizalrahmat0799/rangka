#include "core/generator.hpp"

#include <algorithm>
#include <map>

#include "core/names.hpp"

namespace rangka {
namespace {

bool starts_with(std::string_view s, std::string_view prefix) { return s.substr(0, prefix.size()) == prefix; }
bool ends_with(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

bool is_blank(std::string_view s) {
    return std::all_of(s.begin(), s.end(), [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; });
}

std::string join_path(std::string_view dir, std::string_view rel) {
    if (dir.empty() || dir == ".") return std::string(rel);
    return std::string(dir) + "/" + std::string(rel);
}

std::string_view trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.remove_suffix(1);
    return s;
}

}  // namespace

Result<Options> validate(Options o) {
    auto name = project_name(o.name);
    if (!name) return fail(name.error());
    o.name = *name;
    if (o.frontend == Frontend::None && o.backend == Backend::None) {
        return fail("choose a frontend, a backend or both");
    }
    if (o.database != Database::None && o.backend == Backend::None) {
        return fail("a database needs a backend to talk to it");
    }
    if (o.database == Database::H2 && o.backend != Backend::Spring) {
        return fail("H2 is a Java database that runs inside the app, so it only works with Spring Boot "
                    "(choose PostgreSQL or MySQL for this backend)");
    }
    if (o.backend == Backend::Spring) {
        if (o.java_package.empty()) {
            std::string flat = to_flat(o.name);
            if (flat.empty() || !(flat[0] >= 'a' && flat[0] <= 'z')) flat = "app" + flat;
            o.java_package = "com.example." + flat;
        }
        auto pkg = java_package(o.java_package);
        if (!pkg) return fail(pkg.error());
    }
    return o;
}

Result<Variables> variables_for(const Options& input) {
    auto checked = validate(input);
    if (!checked) return fail(checked.error());
    const Options& o = *checked;

    Variables v;
    v.set("name", o.name);
    v.set("title", to_title(o.name));
    v.set("pascal", to_pascal(o.name));
    v.set("snake", to_snake(o.name));
    v.set("rangka_version", std::string(kRangkaVersion));

    const bool has_frontend = o.frontend != Frontend::None;
    const bool has_backend = o.backend != Backend::None;
    const bool has_db = o.database != Database::None;
    // a database server (not H2, which runs inside the app) needs Docker Compose, and so its own layout
    const bool db_server = o.database == Database::Postgres || o.database == Database::MySql;
    const bool multi = (has_frontend && has_backend) || db_server;

    v.set("has_frontend", has_frontend);
    v.set("has_backend", has_backend);
    v.set("has_db", has_db);
    v.set("multi", multi);
    v.set("fullstack", has_frontend && has_backend);
    v.set("ci", o.ci);

    v.set("frontend", std::string(choice(o.frontend).id));
    v.set("frontend_label", std::string(choice(o.frontend).label));
    v.set("react", o.frontend == Frontend::React);
    v.set("vue", o.frontend == Frontend::Vue);
    v.set("angular", o.frontend == Frontend::Angular);
    v.set("vite", o.frontend == Frontend::React || o.frontend == Frontend::Vue);
    v.set("dev_port", o.frontend == Frontend::Angular ? "4200" : "5173");
    v.set("dev_server", o.frontend == Frontend::Angular ? "The Angular dev server" : "Vite");

    v.set("backend", std::string(choice(o.backend).id));
    v.set("backend_label", std::string(choice(o.backend).label));
    v.set("backend_language", std::string(choice(o.backend).language));
    v.set("spring", o.backend == Backend::Spring);
    v.set("express", o.backend == Backend::Express);
    v.set("fastapi", o.backend == Backend::FastApi);
    v.set("go", o.backend == Backend::Go);
    v.set("cpp", o.backend == Backend::Cpp);

    v.set("database", std::string(choice(o.database).id));
    v.set("database_label", std::string(choice(o.database).label));
    v.set("postgres", o.database == Database::Postgres);
    v.set("mysql", o.database == Database::MySql);
    v.set("h2", o.database == Database::H2);
    v.set("db_server", db_server);
    const std::string db_name = to_snake(o.name);
    v.set("db_name", db_name);
    v.set("db_user", "app");
    v.set("db_password", "app");
    const bool mysql = o.database == Database::MySql;
    v.set("db_port", mysql ? "3306" : "5432");
    v.set("db_scheme", mysql ? "mysql" : "postgres");
    // connection settings for running the backend on your machine, against the database in Docker
    v.set("db_url_local", std::string(mysql ? "mysql" : "postgres") + "://app:app@localhost:" + (mysql ? "3306" : "5432") +
                              "/" + db_name);
    v.set("jdbc_url_local", o.database == Database::H2 ? "jdbc:h2:file:./data/" + db_name
                            : mysql                    ? "jdbc:mysql://localhost:3306/" + db_name
                                                       : "jdbc:postgresql://localhost:5432/" + db_name);

    // where each part goes: its own folder when there's more than one part, otherwise the project root
    v.set("backend_dir", multi ? "backend" : ".");
    v.set("frontend_dir", multi ? "frontend" : ".");
    v.set("backend_prefix", multi ? "backend/" : "");
    v.set("frontend_prefix", multi ? "frontend/" : "");
    v.set("backend_where", multi ? "`backend/`" : "this folder");
    v.set("frontend_where", multi ? "`frontend/`" : "this folder");

    // Java
    std::string pkg = o.java_package;
    std::string package_path = pkg;
    std::replace(package_path.begin(), package_path.end(), '.', '/');
    v.set("package", pkg);
    v.set("package_path", package_path);
    v.set("app_class", to_pascal(o.name) + "Application");
    // Go
    v.set("go_module", "example.com/" + o.name);

    for (const auto& ver : versions()) v.set(std::string(ver.variable), std::string(ver.version));
    return v;
}

Result<Project> generate(const Options& input, std::span<const EmbeddedFile> templates) {
    auto checked = validate(input);
    if (!checked) return fail(checked.error());
    const Options& o = *checked;
    auto vars_result = variables_for(o);
    if (!vars_result) return fail(vars_result.error());
    Variables vars = std::move(*vars_result);

    const bool multi = vars.truthy("multi");
    const std::string backend_dir = *vars.find("backend_dir");
    const std::string frontend_dir = *vars.find("frontend_dir");

    // template folder -> output folder, in order; later folders override earlier ones for the same output path
    std::vector<std::pair<std::string, std::string>> layers = {{"shared/", "."}};
    if (multi) layers.push_back({"compose/", "."});
    if (o.ci) layers.push_back({"ci/", "."});
    if (o.backend != Backend::None) {
        layers.push_back({"backend/" + std::string(choice(o.backend).id) + "/", backend_dir});
    }
    if (o.frontend != Frontend::None) {
        layers.push_back({"frontend/common/", frontend_dir});
        layers.push_back({"frontend/" + std::string(choice(o.frontend).id) + "/", frontend_dir});
    }

    std::map<std::string, GeneratedFile> files;
    for (const auto& [prefix, out_dir] : layers) {
        for (const auto& t : templates) {
            if (!starts_with(t.path, prefix)) continue;
            std::string rel = render_path(t.path.substr(prefix.size()), vars);
            std::string out_path = join_path(out_dir, rel);
            auto content = render(t.content, vars, t.path);
            if (!content) return fail(content.error());
            if (is_blank(*content) && !is_blank(t.content)) {
                files.erase(out_path);  // switched off for these options (also drops a common file it overrides)
                continue;
            }
            bool executable = ends_with(rel, ".sh");
            files[out_path] = GeneratedFile{out_path, std::move(*content), executable};
        }
    }

    Project project;
    project.name = o.name;
    for (auto& [path, file] : files) project.files.push_back(std::move(file));

    // what to do next
    auto& steps = project.next_steps;
    steps.push_back("cd " + o.name);
    const bool has_frontend = o.frontend != Frontend::None;
    if (multi) {
        steps.push_back("docker compose up --build");
        steps.push_back(has_frontend ? "# then open http://localhost:3000"
                                     : "# then try http://localhost:8080/api/todos");
    }
    std::string bdir = multi ? "cd backend && " : "";
    std::string fdir = multi ? "cd frontend && " : "";
    const bool fullstack = has_frontend && o.backend != Backend::None;
    if (multi) steps.push_back("# or without Docker (see README.md):");
    if (o.database == Database::Postgres || o.database == Database::MySql) steps.push_back("docker compose up -d db");
    if (fullstack) steps.push_back("# the backend, in one terminal:");
    switch (o.backend) {
        case Backend::Spring: steps.push_back(bdir + "mvn spring-boot:run"); break;
        case Backend::Express: steps.push_back(bdir + "npm install && npm run dev"); break;
        case Backend::FastApi:
            steps.push_back(bdir + "python -m venv .venv && . .venv/bin/activate && pip install -r requirements-dev.txt");
            steps.push_back("uvicorn app.main:app --reload --port 8080");
            break;
        case Backend::Go: steps.push_back(bdir + "go mod tidy && go run ."); break;
        case Backend::Cpp:
            steps.push_back(bdir + "cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build");
            steps.push_back("./build/" + to_snake(o.name));
            break;
        case Backend::None: break;
    }
    const std::string dev_url = o.frontend == Frontend::Angular ? "http://localhost:4200" : "http://localhost:5173";
    if (fullstack) steps.push_back("# the frontend, in another (then open " + dev_url + "):");
    if (has_frontend) {
        steps.push_back(fdir + "npm install && npm run dev");
        if (!fullstack) steps.push_back("# then open " + dev_url);
    }

    project.variables = std::move(vars);
    return project;
}

Result<Options> options_from_text(std::string_view text) {
    Options o;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t nl = text.find('\n', pos);
        std::string_view line = text.substr(pos, nl == std::string_view::npos ? std::string_view::npos : nl - pos);
        pos = nl == std::string_view::npos ? text.size() : nl + 1;
        line = trim(line);
        if (line.empty()) continue;
        size_t eq = line.find('=');
        if (eq == std::string_view::npos) return fail("expected key=value, got \"" + std::string(line) + "\"");
        std::string_view key = trim(line.substr(0, eq));
        std::string_view value = trim(line.substr(eq + 1));
        if (key == "name") {
            o.name = std::string(value);
        } else if (key == "frontend") {
            auto f = parse_frontend(value);
            if (!f) return fail("unknown frontend \"" + std::string(value) + "\"");
            o.frontend = *f;
        } else if (key == "backend") {
            auto b = parse_backend(value);
            if (!b) return fail("unknown backend \"" + std::string(value) + "\"");
            o.backend = *b;
        } else if (key == "database") {
            auto d = parse_database(value);
            if (!d) return fail("unknown database \"" + std::string(value) + "\"");
            o.database = *d;
        } else if (key == "package") {
            o.java_package = std::string(value);
        } else if (key == "ci") {
            o.ci = value != "false" && value != "0" && value != "no";
        } else {
            return fail("unknown option \"" + std::string(key) + "\"");
        }
    }
    return o;
}

}  // namespace rangka
