#pragma once
// Turns the chosen options into the files of a new project.
//
// Templates live under templates/ in folders that are combined like this:
//   shared/           always, at the project root
//   compose/          at the root when there is more than one part (frontend + backend, or a database)
//   ci/               at the root unless CI is turned off
//   backend/<id>/     in backend/ (or the root, for a backend on its own)
//   frontend/common/  then frontend/<id>/, in frontend/ (or the root); a file in <id> replaces one in common
// A template that renders to nothing (only whitespace) is left out, so a file can depend on an option by
// wrapping all of it in {% if ... %}.

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/catalog.hpp"
#include "core/embedded.hpp"
#include "core/result.hpp"
#include "core/template.hpp"

namespace rangka {

struct Options {
    std::string name;  // as typed: "My Shop" becomes my-shop
    Frontend frontend = Frontend::None;
    Backend backend = Backend::None;
    Database database = Database::None;
    std::string java_package;  // empty: com.example.<name>
    bool ci = true;
};

struct GeneratedFile {
    std::string path;  // relative to the project folder, '/' separated
    std::string content;
    bool executable = false;
};

struct Project {
    std::string name;  // folder name
    std::vector<GeneratedFile> files;
    std::vector<std::string> next_steps;  // shell commands / hints shown after generating
    Variables variables;
};

/// Checks the combination (something to build, a database needs a backend, valid names).
Result<Options> validate(Options options);

Result<Project> generate(const Options& options, std::span<const EmbeddedFile> templates = embedded_templates());

/// Options from "key=value" lines (name, frontend, backend, database, package, ci), as sent by the web page.
Result<Options> options_from_text(std::string_view text);

/// The variables the templates see, for these options. Exposed for tests and `rangka vars`.
Result<Variables> variables_for(const Options& options);

}  // namespace rangka
