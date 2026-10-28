#include "core/zip.hpp"

#include <array>
#include <set>

namespace rangka {
namespace {

std::array<uint32_t, 256> make_crc_table() {
    std::array<uint32_t, 256> table{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        table[i] = c;
    }
    return table;
}

void put16(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v));
    out.push_back(static_cast<uint8_t>(v >> 8));
}
void put32(std::vector<uint8_t>& out, uint32_t v) {
    put16(out, v & 0xFFFF);
    put16(out, v >> 16);
}
void put_bytes(std::vector<uint8_t>& out, const std::string& s) { out.insert(out.end(), s.begin(), s.end()); }

// MS-DOS date and time, which is what zip headers store
void dos_time(int64_t unix_time, uint16_t& date, uint16_t& time) {
    if (unix_time < 315532800) unix_time = 315532800;  // 1980-01-01, the earliest DOS date
    int64_t days = unix_time / 86400;
    int64_t secs = unix_time % 86400;
    // civil-from-days (Howard Hinnant's algorithm)
    days += 719468;
    int64_t era = days / 146097;
    int64_t doe = days - era * 146097;
    int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int64_t y = yoe + era * 400;
    int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    int64_t mp = (5 * doy + 2) / 153;
    int64_t d = doy - (153 * mp + 2) / 5 + 1;
    int64_t m = mp < 10 ? mp + 3 : mp - 9;
    if (m <= 2) ++y;
    date = static_cast<uint16_t>(((y - 1980) << 9) | (m << 5) | d);
    time = static_cast<uint16_t>(((secs / 3600) << 11) | (((secs / 60) % 60) << 5) | ((secs % 60) / 2));
}

struct Entry {
    std::string name;
    const std::string* data;  // null for directories
    bool executable;
    uint32_t crc;
    uint32_t offset;
};

}  // namespace

uint32_t crc32(const void* data, size_t size, uint32_t crc) {
    static const auto table = make_crc_table();
    const auto* p = static_cast<const uint8_t*>(data);
    crc = ~crc;
    for (size_t i = 0; i < size; ++i) crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

std::vector<uint8_t> make_zip(const Project& project, int64_t unix_time) {
    uint16_t date = 0, time = 0;
    dos_time(unix_time, date, time);

    // directories first (some unzip tools want them), then files
    std::set<std::string> dirs;
    dirs.insert(project.name + "/");
    for (const auto& f : project.files) {
        std::string path = project.name + "/" + f.path;
        for (size_t slash = path.find('/'); slash != std::string::npos; slash = path.find('/', slash + 1)) {
            dirs.insert(path.substr(0, slash + 1));
        }
    }
    std::vector<Entry> entries;
    for (const auto& d : dirs) entries.push_back({d, nullptr, false, 0, 0});
    for (const auto& f : project.files) {
        entries.push_back({project.name + "/" + f.path, &f.content, f.executable,
                           crc32(f.content.data(), f.content.size()), 0});
    }

    std::vector<uint8_t> out;
    for (auto& e : entries) {
        e.offset = static_cast<uint32_t>(out.size());
        uint32_t size = e.data ? static_cast<uint32_t>(e.data->size()) : 0;
        put32(out, 0x04034b50);  // local file header
        put16(out, 10);          // version needed: 1.0
        put16(out, 0x0800);      // flags: names are UTF-8
        put16(out, 0);           // method: stored
        put16(out, time);
        put16(out, date);
        put32(out, e.crc);
        put32(out, size);
        put32(out, size);
        put16(out, static_cast<uint32_t>(e.name.size()));
        put16(out, 0);
        put_bytes(out, e.name);
        if (e.data) put_bytes(out, *e.data);
    }

    uint32_t central_start = static_cast<uint32_t>(out.size());
    for (const auto& e : entries) {
        uint32_t size = e.data ? static_cast<uint32_t>(e.data->size()) : 0;
        bool dir = e.data == nullptr;
        uint32_t mode = dir ? 040755 : (e.executable ? 0100755 : 0100644);
        put32(out, 0x02014b50);  // central directory header
        put16(out, (3 << 8) | 20);  // made by: Unix, zip 2.0 (so the mode bits are honoured)
        put16(out, 10);
        put16(out, 0x0800);
        put16(out, 0);
        put16(out, time);
        put16(out, date);
        put32(out, e.crc);
        put32(out, size);
        put32(out, size);
        put16(out, static_cast<uint32_t>(e.name.size()));
        put16(out, 0);  // extra
        put16(out, 0);  // comment
        put16(out, 0);  // disk
        put16(out, 0);  // internal attributes
        put32(out, (mode << 16) | (dir ? 0x10 : 0));  // external: Unix mode, plus the MS-DOS directory flag
        put32(out, e.offset);
        put_bytes(out, e.name);
    }
    uint32_t central_size = static_cast<uint32_t>(out.size()) - central_start;

    put32(out, 0x06054b50);  // end of central directory
    put16(out, 0);
    put16(out, 0);
    put16(out, static_cast<uint32_t>(entries.size()));
    put16(out, static_cast<uint32_t>(entries.size()));
    put32(out, central_size);
    put32(out, central_start);
    put16(out, 0);
    return out;
}

}  // namespace rangka
