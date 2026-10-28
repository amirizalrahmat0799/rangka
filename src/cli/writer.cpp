#include "cli/writer.hpp"

#include <chrono>
#include <fstream>
#include <system_error>

#include "core/zip.hpp"

namespace fs = std::filesystem;

namespace cli {

std::string write_project(const rangka::Project& project, const fs::path& dir, bool force) {
    std::error_code ec;
    if (fs::exists(dir, ec)) {
        if (!fs::is_directory(dir, ec)) return dir.string() + " exists and is not a folder";
        if (!force && !fs::is_empty(dir, ec)) {
            return "the folder " + dir.string() + " already has files in it (use --force to write into it anyway)";
        }
    }
    for (const auto& file : project.files) {
        fs::path path = dir / fs::path(file.path);  // file.path uses '/', which works on Windows too
        fs::create_directories(path.parent_path(), ec);
        if (ec) return "can't create " + path.parent_path().string() + ": " + ec.message();
        std::ofstream out(path, std::ios::binary);  // binary: keep LF line endings on Windows as well
        out << file.content;
        if (!out) return "can't write " + path.string();
        out.close();
        if (file.executable) {
            fs::permissions(path, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                            fs::perm_options::add, ec);
        }
    }
    return {};
}

std::string write_zip(const rangka::Project& project, const fs::path& file) {
    auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
    auto bytes = rangka::make_zip(project, now.count());
    std::ofstream out(file, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!out) return "can't write " + file.string();
    return {};
}

}  // namespace cli
