#pragma once
// The files under templates/, compiled into the program by tools/embed.cpp, so rangka needs nothing next to it.

#include <cstddef>
#include <span>
#include <string_view>

namespace rangka {

struct EmbeddedFile {
    std::string_view path;     // relative to templates/, with '/' separators: "backend/go/main.go"
    std::string_view content;
};

std::span<const EmbeddedFile> embedded_templates();

}  // namespace rangka
