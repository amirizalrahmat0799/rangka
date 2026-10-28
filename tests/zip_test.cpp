#include <cstring>

#include "check.hpp"
#include "core/zip.hpp"

using namespace rangka;

namespace {
uint32_t read32(const std::vector<uint8_t>& b, size_t at) {
    return b[at] | (b[at + 1] << 8) | (b[at + 2] << 16) | (static_cast<uint32_t>(b[at + 3]) << 24);
}
}  // namespace

TEST("crc32 matches the standard check value") {
    const char* s = "123456789";
    CHECK_EQ(crc32(s, std::strlen(s)), 0xCBF43926u);
    CHECK_EQ(crc32("", 0), 0u);
}

TEST("zip layout") {
    Project p;
    p.name = "shop";
    p.files.push_back({"README.md", "# Shop\n", false});
    p.files.push_back({"backend/run.sh", "echo hi\n", true});
    auto zip = make_zip(p, 1790000000);

    CHECK_EQ(read32(zip, 0), 0x04034b50u);
    // end of central directory: 22 bytes at the end, entry count at +10
    size_t eocd = zip.size() - 22;
    CHECK_EQ(read32(zip, eocd), 0x06054b50u);
    unsigned entries = zip[eocd + 10] | (zip[eocd + 11] << 8);
    CHECK_EQ(entries, 4u);  // shop/, shop/backend/, two files
    uint32_t central = read32(zip, eocd + 16);
    CHECK_EQ(read32(zip, central), 0x02014b50u);

    std::string all(zip.begin(), zip.end());
    CHECK(all.find("shop/backend/run.sh") != std::string::npos);
    CHECK(all.find("# Shop\n") != std::string::npos);
}
