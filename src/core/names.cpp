#include "core/names.hpp"

#include <array>

namespace rangka {
namespace {

bool is_lower(char c) { return c >= 'a' && c <= 'z'; }
bool is_upper(char c) { return c >= 'A' && c <= 'Z'; }
bool is_digit(char c) { return c >= '0' && c <= '9'; }
bool is_alnum(char c) { return is_lower(c) || is_upper(c) || is_digit(c); }
char lower(char c) { return is_upper(c) ? static_cast<char>(c - 'A' + 'a') : c; }
char upper(char c) { return is_lower(c) ? static_cast<char>(c - 'a' + 'A') : c; }

std::string join(const std::vector<std::string>& words, std::string_view sep) {
    std::string out;
    for (size_t i = 0; i < words.size(); ++i) {
        if (i) out += sep;
        out += words[i];
    }
    return out;
}

constexpr std::array<std::string_view, 53> kJavaKeywords = {
    "abstract", "assert",     "boolean",   "break",     "byte",      "case",      "catch",    "char",
    "class",    "const",      "continue",  "default",   "do",        "double",    "else",     "enum",
    "extends",  "final",      "finally",   "float",     "for",       "goto",      "if",       "implements",
    "import",   "instanceof", "int",       "interface", "long",      "native",    "new",      "package",
    "private",  "protected",  "public",    "return",    "short",     "static",    "strictfp", "super",
    "switch",   "synchronized", "this",    "throw",     "throws",    "transient", "try",      "void",
    "volatile", "while",      "true",      "false",     "null"};

}  // namespace

std::vector<std::string> split_words(std::string_view text) {
    std::vector<std::string> words;
    std::string current;
    char prev = 0;
    for (char c : text) {
        if (!is_alnum(c)) {
            if (!current.empty()) words.push_back(std::move(current));
            current.clear();
        } else {
            // camelCase boundary: "myShop" -> "my", "shop"
            if (is_upper(c) && (is_lower(prev) || is_digit(prev)) && !current.empty()) {
                words.push_back(std::move(current));
                current.clear();
            }
            current += lower(c);
        }
        prev = c;
    }
    if (!current.empty()) words.push_back(std::move(current));
    return words;
}

std::string to_kebab(std::string_view text) { return join(split_words(text), "-"); }
std::string to_snake(std::string_view text) { return join(split_words(text), "_"); }
std::string to_flat(std::string_view text) { return join(split_words(text), ""); }

std::string to_pascal(std::string_view text) {
    std::string out;
    for (auto& w : split_words(text)) {
        w[0] = upper(w[0]);
        out += w;
    }
    return out;
}

std::string to_title(std::string_view text) {
    auto words = split_words(text);
    for (auto& w : words) w[0] = upper(w[0]);
    return join(words, " ");
}

Result<std::string> project_name(std::string_view text) {
    std::string name = to_kebab(text);
    if (name.empty()) return fail("the project name needs at least one letter or digit");
    if (!is_lower(name[0])) return fail("the project name must start with a letter: \"" + name + "\"");
    if (name.size() > 64) return fail("the project name is too long (64 characters at most)");
    return name;
}

Result<std::string> java_package(std::string_view text) {
    if (text.empty()) return fail("the Java package is empty");
    std::string pkg(text);
    size_t start = 0;
    while (true) {
        size_t dot = pkg.find('.', start);
        std::string_view part = std::string_view(pkg).substr(start, dot == std::string::npos ? std::string::npos
                                                                                             : dot - start);
        if (part.empty()) return fail("the Java package \"" + pkg + "\" has an empty part");
        if (!is_lower(part[0])) {
            return fail("each part of the Java package must start with a lowercase letter: \"" + pkg + "\"");
        }
        for (char c : part) {
            if (!is_lower(c) && !is_digit(c) && c != '_') {
                return fail("the Java package may only contain lowercase letters, digits, '_' and '.': \"" + pkg +
                            "\"");
            }
        }
        for (auto kw : kJavaKeywords) {
            if (part == kw) return fail("\"" + std::string(part) + "\" is a Java keyword and can't be a package name");
        }
        if (dot == std::string::npos) break;
        start = dot + 1;
    }
    return pkg;
}

}  // namespace rangka
