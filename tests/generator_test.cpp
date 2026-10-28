#include <algorithm>
#include <set>

#include "check.hpp"
#include "core/generator.hpp"

using namespace rangka;

namespace {

const GeneratedFile* find(const Project& p, std::string_view path) {
    for (const auto& f : p.files) {
        if (f.path == path) return &f;
    }
    return nullptr;
}

Options opts(Frontend f, Backend b, Database d) {
    Options o;
    o.name = "My Shop";
    o.frontend = f;
    o.backend = b;
    o.database = d;
    return o;
}

}  // namespace

TEST("invalid combinations are refused") {
    CHECK(!generate(opts(Frontend::None, Backend::None, Database::None)).ok());
    CHECK(!generate(opts(Frontend::React, Backend::None, Database::Postgres)).ok());
    auto bad_name = opts(Frontend::React, Backend::None, Database::None);
    bad_name.name = "???";
    CHECK(!generate(bad_name).ok());
    auto bad_pkg = opts(Frontend::None, Backend::Spring, Database::None);
    bad_pkg.java_package = "Com.Shop";
    CHECK(!generate(bad_pkg).ok());
}

TEST("layering with synthetic templates") {
    std::vector<EmbeddedFile> t = {
        {"shared/README.md", "# {% title %}\n"},
        {"compose/docker-compose.yml", "services: {}\n"},
        {"backend/go/main.go", "package main // {% go_module %}\n"},
        {"backend/go/db.go", "{% if postgres %}\npackage main\n{% endif %}\n"},
        {"frontend/common/src/App.txt", "common\n"},
        {"frontend/common/src/only-common.txt", "common\n"},
        {"frontend/react/src/App.txt", "react\n"},
        {"frontend/react/src/only-common.txt", "{% if vue %}\nnever\n{% endif %}\n"},
        {"frontend/vue/src/App.txt", "vue\n"},
        {"backend/go/run.sh", "go run .\n"},
        {"backend/go/__init__.py", ""},
    };
    auto full = generate(opts(Frontend::React, Backend::Go, Database::None), t);
    CHECK(full.ok());
    if (!full) return;
    CHECK_EQ(full->name, "my-shop");
    CHECK(find(*full, "README.md") && find(*full, "README.md")->content == "# My Shop\n");
    CHECK(find(*full, "docker-compose.yml") != nullptr);
    CHECK(find(*full, "backend/main.go") && find(*full, "backend/main.go")->content.find("example.com/my-shop") !=
                                                 std::string::npos);
    CHECK(find(*full, "backend/db.go") == nullptr);           // switched off without postgres
    CHECK(find(*full, "frontend/src/App.txt") && find(*full, "frontend/src/App.txt")->content == "react\n");
    CHECK(find(*full, "frontend/src/only-common.txt") == nullptr);  // the react layer removed it
    CHECK(find(*full, "backend/run.sh") && find(*full, "backend/run.sh")->executable);
    CHECK(find(*full, "backend/__init__.py") != nullptr);  // empty on purpose, kept

    auto alone = generate(opts(Frontend::None, Backend::Go, Database::None), t);
    CHECK(alone.ok());
    if (!alone) return;
    CHECK(find(*alone, "main.go") != nullptr);  // a backend on its own sits at the root
    CHECK(find(*alone, "docker-compose.yml") == nullptr);
}

TEST("every combination renders the real templates") {
    std::set<std::string> seen_paths;
    int combos = 0;
    for (auto f : {Frontend::None, Frontend::React, Frontend::Vue}) {
        for (auto b : {Backend::None, Backend::Spring, Backend::Express, Backend::FastApi, Backend::Go, Backend::Cpp}) {
            for (auto d : {Database::None, Database::Postgres}) {
                for (bool ci : {true, false}) {
                    auto o = opts(f, b, d);
                    o.ci = ci;
                    if (!validate(o).ok()) continue;
                    ++combos;
                    auto p = generate(o);
                    if (!p) {
                        std::cerr << "generate failed: " << p.error() << "\n";
                        CHECK(p.ok());
                        continue;
                    }
                    CHECK(!p->files.empty());
                    CHECK(find(*p, "README.md") != nullptr);
                    CHECK(find(*p, ".gitignore") != nullptr);
                    CHECK((find(*p, ".github/workflows/ci.yml") != nullptr) == ci);
                    for (const auto& file : p->files) {
                        seen_paths.insert(file.path);
                        // nothing of the template language may survive, and every file ends with a newline
                        if (file.content.find("{%") != std::string::npos || file.content.find("%}") != std::string::npos) {
                            std::cerr << "leftover tag in " << file.path << "\n";
                            CHECK(false);
                        }
                        if (!file.content.empty() && file.content.back() != '\n') {
                            std::cerr << "no final newline: " << file.path << "\n";
                            CHECK(false);
                        }
                        if (file.content.find("\n\n\n") != std::string::npos && !file.path.ends_with(".py")) {
                            std::cerr << "two blank lines in a row: " << file.path << "\n";
                            CHECK(false);
                        }
                    }
                }
            }
        }
    }
    CHECK_EQ(combos, 2 * (2 + 5 * 2 + 2 * 5 * 2));  // ci on/off x (frontend only, backend +/- db, both +/- db)
}

TEST("options from text") {
    auto o = options_from_text("name=Kedai\nfrontend=vue\nbackend=fastapi\ndatabase=postgres\nci=false\n");
    CHECK(o.ok());
    if (!o) return;
    CHECK_EQ(o->name, "Kedai");
    CHECK(o->frontend == Frontend::Vue);
    CHECK(o->backend == Backend::FastApi);
    CHECK(o->database == Database::Postgres);
    CHECK(!o->ci);
    CHECK(!options_from_text("backend=cobol").ok());
    CHECK(!options_from_text("colour=blue").ok());
    CHECK(!options_from_text("nonsense").ok());
}

TEST("java package defaults from the name") {
    auto o = opts(Frontend::None, Backend::Spring, Database::None);
    auto v = variables_for(o);
    CHECK(v.ok());
    if (!v) return;
    CHECK_EQ(*v->find("package"), "com.example.myshop");
    CHECK_EQ(*v->find("package_path"), "com/example/myshop");
    CHECK_EQ(*v->find("app_class"), "MyShopApplication");
}
