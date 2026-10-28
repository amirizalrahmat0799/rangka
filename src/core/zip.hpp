#pragma once
// A minimal ZIP writer (stored entries, no compression) for the web page's download and `rangka new --zip`.
// Source code is small, and leaving out deflate keeps the WebAssembly module small too.

#include <cstdint>
#include <string>
#include <vector>

#include "core/generator.hpp"

namespace rangka {

uint32_t crc32(const void* data, size_t size, uint32_t crc = 0);

/// Writes the project into a zip with everything under "<project.name>/". `unix_time` stamps the entries
/// (seconds since 1970, UTC); executable files (*.sh) keep their execute bit when unzipped on Linux and macOS.
std::vector<uint8_t> make_zip(const Project& project, int64_t unix_time);

}  // namespace rangka
