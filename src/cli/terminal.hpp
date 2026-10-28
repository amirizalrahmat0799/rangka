#pragma once
// Terminal input and output for the interactive CLI: colours, arrow-key menus and text prompts.
// Works in Windows Terminal / PowerShell / cmd and on Linux and macOS. When stdin isn't a terminal (piped
// input, CI), menus fall back to typing a number.

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace term {

/// Call once at startup: enables UTF-8 and ANSI colours on Windows, and decides whether to use colour at all
/// (not when NO_COLOR is set or stdout isn't a terminal).
void init();

bool interactive();  // stdin and stdout are terminals

std::string bold(std::string_view s);
std::string dim(std::string_view s);
std::string cyan(std::string_view s);
std::string green(std::string_view s);
std::string red(std::string_view s);
std::string yellow(std::string_view s);

struct MenuItem {
    std::string label;
    std::string hint;  // shown dimmed after the label
};

/// Lets the user pick one item with the arrow keys (or j/k, or its number). Returns nullopt on Ctrl+C / Esc.
std::optional<size_t> select(std::string_view question, const std::vector<MenuItem>& items, size_t initial = 0);

/// Asks for a line of text; an empty answer gives `fallback`. nullopt on Ctrl+C / end of input.
std::optional<std::string> ask(std::string_view question, std::string_view fallback);

/// Yes/no question. nullopt on Ctrl+C / end of input.
std::optional<bool> confirm(std::string_view question, bool fallback);

}  // namespace term
