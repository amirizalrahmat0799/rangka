// rangka: create a new project with a frontend, a backend, a database, or all three, already wired together.

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "cli/terminal.hpp"
#include "cli/writer.hpp"
#include "core/catalog.hpp"
#include "core/generator.hpp"
#include "core/names.hpp"

namespace fs = std::filesystem;
using namespace rangka;

namespace {

constexpr const char* kHelp = R"(rangka - start a new project: a frontend, a backend, a database, or all three, wired together

Usage:
  rangka new [name] [options]   create a project (asks for anything you leave out)
  rangka list                   show the frontends, backends and databases, with versions
  rangka --version

Options for new:
  -f, --frontend <react|vue|none>
  -b, --backend  <spring|express|fastapi|go|cpp|none>
  -d, --db       <postgres|none>
  -p, --package  <java.package>  Java package for Spring Boot (default com.example.<name>)
      --no-ci                    leave out the GitHub Actions workflow
  -o, --output   <folder>        where to create the project folder (default: here)
      --zip      [file]          write a .zip instead of a folder
      --dry-run                  list the files without writing anything
      --force                    write into a folder that already has files
  -y, --yes                      don't ask anything; use the options given

Examples:
  rangka new
  rangka new shop --frontend react --backend spring --db postgres
  rangka new api --backend go --db postgres
  rangka new site --frontend vue
)";

struct Args {
    std::vector<std::string> positional;
    std::map<std::string, std::string> values;
    std::map<std::string, bool> flags;
};

// short and long names that take a value, and the switches
const std::map<std::string, std::string> kValueOptions = {
    {"-f", "frontend"}, {"--frontend", "frontend"}, {"-b", "backend"},   {"--backend", "backend"},
    {"-d", "db"},       {"--db", "db"},             {"--database", "db"}, {"-p", "package"},
    {"--package", "package"}, {"-o", "output"},     {"--output", "output"}};
const std::map<std::string, std::string> kFlags = {{"--no-ci", "no-ci"}, {"--dry-run", "dry-run"},
                                                   {"--force", "force"}, {"-y", "yes"},
                                                   {"--yes", "yes"},     {"--zip", "zip"}};

std::optional<Args> parse_args(int argc, char** argv, int start) {
    Args args;
    for (int i = start; i < argc; ++i) {
        std::string a = argv[i];
        std::string value;
        bool has_value = false;
        if (a.rfind("--", 0) == 0 && a.find('=') != std::string::npos) {
            value = a.substr(a.find('=') + 1);
            a = a.substr(0, a.find('='));
            has_value = true;
        }
        if (auto it = kValueOptions.find(a); it != kValueOptions.end()) {
            if (!has_value) {
                if (i + 1 >= argc) {
                    std::cerr << term::red("error: ") << a << " needs a value\n";
                    return std::nullopt;
                }
                value = argv[++i];
            }
            args.values[it->second] = value;
        } else if (auto f = kFlags.find(a); f != kFlags.end()) {
            args.flags[f->second] = true;
            if (f->second == "zip") {
                if (has_value) args.values["zip"] = value;
                else if (i + 1 < argc && argv[i + 1][0] != '-' && std::string(argv[i + 1]).size() > 4 &&
                         std::string(argv[i + 1]).substr(std::string(argv[i + 1]).size() - 4) == ".zip") {
                    args.values["zip"] = argv[++i];
                }
            }
        } else if (!a.empty() && a[0] == '-') {
            std::cerr << term::red("error: ") << "unknown option " << a << " (see rangka --help)\n";
            return std::nullopt;
        } else {
            args.positional.push_back(a);
        }
    }
    return args;
}

std::vector<term::MenuItem> menu(std::span<const Choice> choices, bool include_none) {
    std::vector<term::MenuItem> items;
    for (const auto& c : choices) {
        if (c.id == "none" && !include_none) continue;
        std::string label(c.label);
        if (!c.language.empty() && c.language != c.label) label += " (" + std::string(c.language) + ")";
        items.push_back({label, std::string(c.description)});
    }
    return items;
}

std::string describe(const Choice& c) {
    if (c.id == "none") return "-";
    std::string s(c.label);
    if (!c.language.empty()) s += " (" + std::string(c.language) + ")";
    return s;
}

int cancelled() {
    std::cerr << term::dim("cancelled") << "\n";
    return 130;
}

int cmd_list() {
    std::cout << term::bold("Frontends") << "\n";
    for (const auto& c : frontend_choices()) {
        if (c.id != "none") std::printf("  %-8s %s\n", std::string(c.id).c_str(), std::string(c.description).c_str());
    }
    std::cout << term::bold("Backends") << "\n";
    for (const auto& c : backend_choices()) {
        if (c.id != "none") std::printf("  %-8s %s\n", std::string(c.id).c_str(), std::string(c.description).c_str());
    }
    std::cout << term::bold("Databases") << "\n";
    for (const auto& c : database_choices()) {
        if (c.id != "none") std::printf("  %-8s %s\n", std::string(c.id).c_str(), std::string(c.description).c_str());
    }
    std::cout << term::bold("Versions used") << "\n";
    for (const auto& v : versions()) {
        std::printf("  %-22s %s\n", std::string(v.name).c_str(), std::string(v.version).c_str());
    }
    return 0;
}

int cmd_new(const Args& args) {
    const bool ask = term::interactive() && !args.flags.count("yes");
    Options o;
    o.ci = !args.flags.count("no-ci");

    // what was given on the command line
    bool frontend_given = false, backend_given = false, db_given = false;
    if (auto it = args.values.find("frontend"); it != args.values.end()) {
        auto f = parse_frontend(it->second);
        if (!f) {
            std::cerr << term::red("error: ") << "unknown frontend \"" << it->second << "\" (react, vue or none)\n";
            return 2;
        }
        o.frontend = *f;
        frontend_given = true;
    }
    if (auto it = args.values.find("backend"); it != args.values.end()) {
        auto b = parse_backend(it->second);
        if (!b) {
            std::cerr << term::red("error: ") << "unknown backend \"" << it->second
                      << "\" (spring, express, fastapi, go, cpp or none)\n";
            return 2;
        }
        o.backend = *b;
        backend_given = true;
    }
    if (auto it = args.values.find("db"); it != args.values.end()) {
        auto d = parse_database(it->second);
        if (!d) {
            std::cerr << term::red("error: ") << "unknown database \"" << it->second << "\" (postgres or none)\n";
            return 2;
        }
        o.database = *d;
        db_given = true;
    }
    if (auto it = args.values.find("package"); it != args.values.end()) o.java_package = it->second;

    if (ask) std::cout << term::bold("rangka") << " " << term::dim(std::string(kRangkaVersion)) << "\n\n";

    // name
    if (!args.positional.empty()) {
        o.name = args.positional[0];
    } else if (ask) {
        while (true) {
            auto answer = term::ask("Project name:", "my-app");
            if (!answer) return cancelled();
            auto name = project_name(*answer);
            if (name) {
                o.name = *name;
                break;
            }
            std::cout << term::red("  " + name.error()) << "\n";
        }
    } else {
        std::cerr << term::red("error: ") << "give the project a name: rangka new <name> ...\n";
        return 2;
    }
    if (auto name = project_name(o.name); !name) {
        std::cerr << term::red("error: ") << name.error() << "\n";
        return 2;
    }

    // what to build
    if (!frontend_given && !backend_given) {
        if (!ask) {
            std::cerr << term::red("error: ") << "choose --frontend, --backend or both\n";
            return 2;
        }
        auto kind = term::select("What are you building?",
                                 {{"Full stack", "frontend + backend + database, wired together"},
                                  {"Backend API", "a backend on its own, with or without a database"},
                                  {"Frontend", "a frontend on its own, keeping data in the browser"}});
        if (!kind) return cancelled();
        bool want_frontend = *kind == 0 || *kind == 2;
        bool want_backend = *kind == 0 || *kind == 1;
        if (want_frontend) {
            auto f = term::select("Frontend:", menu(frontend_choices(), false));
            if (!f) return cancelled();
            o.frontend = *parse_frontend(frontend_choices()[*f].id);
        }
        if (want_backend) {
            auto b = term::select("Backend:", menu(backend_choices(), false));
            if (!b) return cancelled();
            o.backend = *parse_backend(backend_choices()[*b].id);
            if (!db_given) {
                auto d = term::select("Database:", menu(database_choices(), true), 0);
                if (!d) return cancelled();
                o.database = *parse_database(database_choices()[*d].id);
            }
        }
    } else if (ask && o.backend != Backend::None && !db_given) {
        auto d = term::select("Database:", menu(database_choices(), true), 0);
        if (!d) return cancelled();
        o.database = *parse_database(database_choices()[*d].id);
    }

    if (o.backend == Backend::Spring && o.java_package.empty() && ask) {
        auto defaults = validate(o);
        std::string suggestion = defaults ? defaults->java_package : "com.example.app";
        while (true) {
            auto answer = term::ask("Java package:", suggestion);
            if (!answer) return cancelled();
            if (auto pkg = java_package(*answer); pkg) {
                o.java_package = *pkg;
                break;
            } else {
                std::cout << term::red("  " + pkg.error()) << "\n";
            }
        }
    }

    auto project = generate(o);
    if (!project) {
        std::cerr << term::red("error: ") << project.error() << "\n";
        return 2;
    }

    fs::path parent = args.values.count("output") ? fs::path(args.values.at("output")) : fs::current_path();
    fs::path dir = parent / project->name;

    if (args.flags.count("dry-run")) {
        size_t total = 0;
        for (const auto& f : project->files) {
            std::printf("  %6zu  %s/%s\n", f.content.size(), project->name.c_str(), f.path.c_str());
            total += f.content.size();
        }
        std::printf("%zu files, %zu bytes (nothing written)\n", project->files.size(), total);
        return 0;
    }

    if (ask) {
        std::cout << "\n  " << term::dim("Frontend ") << " " << describe(choice(o.frontend)) << "\n  "
                  << term::dim("Backend  ") << " " << describe(choice(o.backend)) << "\n  " << term::dim("Database ")
                  << " " << describe(choice(o.database)) << "\n\n";
        auto go = term::confirm("Create " + project->name + "?", true);
        if (!go) return cancelled();
        if (!*go) return cancelled();
    }

    std::string error;
    std::string where;
    if (args.flags.count("zip")) {
        fs::path zip = args.values.count("zip") ? fs::path(args.values.at("zip")) : parent / (project->name + ".zip");
        error = cli::write_zip(*project, zip);
        std::error_code ec;
        auto rel = fs::relative(zip, fs::current_path(), ec);
        where = (!ec && !rel.empty() && rel.native()[0] != '.') ? rel.string() : zip.string();
    } else {
        error = cli::write_project(*project, dir, args.flags.count("force") > 0);
        // show the folder the way the user would type it: relative when it's under the current folder
        std::error_code ec;
        auto rel = fs::relative(dir, fs::current_path(), ec);
        where = (!ec && !rel.empty() && rel.native()[0] != '.') ? rel.string() : dir.string();
    }
    if (!error.empty()) {
        std::cerr << term::red("error: ") << error << "\n";
        return 1;
    }

    std::cout << term::green("Created ") << term::bold(where) << " " << term::dim("(" + std::to_string(project->files.size()) + " files)")
              << "\n\n" << term::bold("Next steps") << "\n";
    for (const auto& step : project->next_steps) {
        if (step.rfind("# ", 0) == 0) std::cout << "  " << term::dim(step.substr(2)) << "\n";
        else std::cout << "  " << term::cyan(step) << "\n";
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    term::init();
    std::string command = argc > 1 ? argv[1] : "";

    if (command == "--version" || command == "-v" || command == "version") {
        std::cout << "rangka " << kRangkaVersion << "\n";
        return 0;
    }
    if (command == "--help" || command == "-h" || command == "help") {
        std::cout << kHelp;
        return 0;
    }
    if (command == "list") return cmd_list();
    if (command == "new" || command.empty()) {
        if (command.empty() && !term::interactive()) {
            std::cout << kHelp;
            return 0;
        }
        auto args = parse_args(argc, argv, command.empty() ? 1 : 2);
        if (!args) return 2;
        return cmd_new(*args);
    }
    std::cerr << term::red("error: ") << "unknown command \"" << command << "\"\n\n" << kHelp;
    return 2;
}
