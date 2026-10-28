#pragma once
// Writing a generated project to disk.

#include <filesystem>
#include <string>

#include "core/generator.hpp"

namespace cli {

/// Writes every file under `dir`. Refuses a folder that already has files in it unless `force` is set.
/// Returns an error message, or an empty string on success.
std::string write_project(const rangka::Project& project, const std::filesystem::path& dir, bool force);

/// Writes a .zip of the project. Returns an error message, or an empty string on success.
std::string write_zip(const rangka::Project& project, const std::filesystem::path& file);

}  // namespace cli
