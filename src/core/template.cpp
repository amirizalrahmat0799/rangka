#include "core/template.hpp"

#include <vector>

namespace rangka {
namespace {

bool is_space(char c) { return c == ' ' || c == '\t' || c == '\r'; }
bool is_ident_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

std::string_view trim(std::string_view s) {
    while (!s.empty() && (is_space(s.front()) || s.front() == '\n')) s.remove_prefix(1);
    while (!s.empty() && (is_space(s.back()) || s.back() == '\n')) s.remove_suffix(1);
    return s;
}

bool is_identifier(std::string_view s) {
    if (s.empty() || (s[0] >= '0' && s[0] <= '9')) return false;
    for (char c : s) {
        if (!is_ident_char(c)) return false;
    }
    return true;
}

enum class Kind { Text, Var, If, Else, EndIf, Comment };

struct Token {
    Kind kind;
    std::string_view text;  // Text: the text; Var: the name; If: the condition
    int line;
};

std::vector<std::string_view> words(std::string_view s) {
    std::vector<std::string_view> out;
    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && is_space(s[i])) ++i;
        size_t start = i;
        while (i < s.size() && !is_space(s[i])) ++i;
        if (i > start) out.push_back(s.substr(start, i - start));
    }
    return out;
}

std::string where(std::string_view source, int line) {
    return std::string(source) + ":" + std::to_string(line) + ": ";
}

// Evaluates "a and not b or c". Returns an error for unknown variables or malformed conditions.
Result<bool> evaluate(std::string_view condition, const Variables& vars, std::string_view source, int line) {
    auto ws = words(condition);
    if (ws.empty()) return fail(where(source, line) + "`if` needs a condition");
    bool any = false;     // value of the `or` chain so far
    bool all = true;      // value of the current `and` chain
    bool negate = false;
    bool expect_term = true;
    for (auto w : ws) {
        if (expect_term) {
            if (w == "not") {
                negate = !negate;
                continue;
            }
            if (!is_identifier(w) || w == "and" || w == "or") {
                return fail(where(source, line) + "unexpected `" + std::string(w) + "` in condition `" +
                            std::string(condition) + "`");
            }
            if (!vars.has(w)) {
                return fail(where(source, line) + "unknown variable `" + std::string(w) + "` in condition");
            }
            bool v = vars.truthy(w) != negate;
            all = all && v;
            negate = false;
            expect_term = false;
        } else if (w == "and") {
            expect_term = true;
        } else if (w == "or") {
            any = any || all;
            all = true;
            expect_term = true;
        } else {
            return fail(where(source, line) + "expected `and` or `or` before `" + std::string(w) + "` in `" +
                        std::string(condition) + "`");
        }
    }
    if (expect_term) return fail(where(source, line) + "condition `" + std::string(condition) + "` is incomplete");
    return any || all;
}

Result<std::vector<Token>> tokenize(std::string_view text, std::string_view source) {
    std::vector<Token> tokens;
    size_t pos = 0;
    int line = 1;
    bool skip_rest_of_line = false;  // set after a standalone block tag

    auto count_lines = [](std::string_view s) {
        int n = 0;
        for (char c : s) n += c == '\n';
        return n;
    };

    while (pos <= text.size()) {
        size_t open = text.find("{%", pos);
        std::string_view chunk = text.substr(pos, open == std::string_view::npos ? std::string_view::npos : open - pos);
        int chunk_line = line;
        line += count_lines(chunk);

        if (skip_rest_of_line) {
            // drop the whitespace and newline that followed a standalone tag
            size_t nl = chunk.find('\n');
            chunk.remove_prefix(nl + 1);
            ++chunk_line;
            skip_rest_of_line = false;
        }
        if (!chunk.empty()) tokens.push_back({Kind::Text, chunk, chunk_line});
        if (open == std::string_view::npos) break;

        size_t close = text.find("%}", open + 2);
        if (close == std::string_view::npos) return fail(where(source, line) + "`{%` is never closed with `%}`");
        std::string_view inner = text.substr(open + 2, close - open - 2);
        if (inner.find('\n') != std::string_view::npos) {
            return fail(where(source, line) + "a `{% ... %}` tag must fit on one line");
        }
        pos = close + 2;

        Token tag{Kind::Var, {}, line};
        std::string_view body = trim(inner);
        if (!body.empty() && body[0] == '#') {
            tag.kind = Kind::Comment;
        } else {
            auto ws = words(body);
            if (ws.empty()) return fail(where(source, line) + "empty `{% %}` tag");
            if (ws[0] == "if") {
                tag.kind = Kind::If;
                tag.text = trim(body.substr(2));
            } else if (ws[0] == "else" && ws.size() == 1) {
                tag.kind = Kind::Else;
            } else if (ws[0] == "endif" && ws.size() == 1) {
                tag.kind = Kind::EndIf;
            } else if (ws.size() == 1 && is_identifier(ws[0])) {
                tag.kind = Kind::Var;
                tag.text = ws[0];
            } else {
                return fail(where(source, line) + "don't understand the tag `{% " + std::string(body) + " %}`");
            }
        }

        if (tag.kind != Kind::Var) {
            // Standalone tag: only whitespace before it on its line and only whitespace after it up to the newline.
            bool blank_before = true;
            size_t line_start = open;
            while (line_start > 0 && text[line_start - 1] != '\n') {
                if (!is_space(text[line_start - 1])) blank_before = false;
                --line_start;
            }
            size_t after = pos;
            while (after < text.size() && is_space(text[after])) ++after;
            bool blank_after = after == text.size() || text[after] == '\n';
            if (blank_before && blank_after) {
                // remove the indentation already emitted as text
                if (!tokens.empty() && tokens.back().kind == Kind::Text) {
                    auto& t = tokens.back().text;
                    size_t cut = t.size();
                    while (cut > 0 && is_space(t[cut - 1])) --cut;
                    if (cut == 0 || t[cut - 1] == '\n') {
                        t = t.substr(0, cut);
                        if (t.empty()) tokens.pop_back();
                    }
                }
                skip_rest_of_line = after < text.size();
                if (!skip_rest_of_line) pos = after;
            }
        }
        if (tag.kind != Kind::Comment) tokens.push_back(tag);
    }
    return tokens;
}

}  // namespace

const std::string* Variables::find(std::string_view name) const {
    auto it = values_.find(name);
    return it == values_.end() ? nullptr : &it->second;
}

bool Variables::truthy(std::string_view name) const {
    auto v = find(name);
    return v && !v->empty() && *v != "false" && *v != "0";
}

Result<std::string> render(std::string_view text, const Variables& vars, std::string_view source) {
    auto tokens = tokenize(text, source);
    if (!tokens) return fail(tokens.error());

    struct Frame {
        bool parent_active;
        bool condition;
        bool in_else;
        int line;
    };
    std::vector<Frame> stack;
    bool active = true;
    std::string out;
    out.reserve(text.size());

    for (const auto& t : *tokens) {
        switch (t.kind) {
            case Kind::Text:
                if (active) out += t.text;
                break;
            case Kind::Var: {
                auto v = vars.find(t.text);
                if (!v) return fail(where(source, t.line) + "unknown variable `" + std::string(t.text) + "`");
                if (active) out += *v;
                break;
            }
            case Kind::If: {
                auto c = evaluate(t.text, vars, source, t.line);
                if (!c) return fail(c.error());
                stack.push_back({active, *c, false, t.line});
                active = active && *c;
                break;
            }
            case Kind::Else:
                if (stack.empty()) return fail(where(source, t.line) + "`else` without `if`");
                if (stack.back().in_else) return fail(where(source, t.line) + "two `else` for one `if`");
                stack.back().in_else = true;
                active = stack.back().parent_active && !stack.back().condition;
                break;
            case Kind::EndIf:
                if (stack.empty()) return fail(where(source, t.line) + "`endif` without `if`");
                active = stack.back().parent_active;
                stack.pop_back();
                break;
            case Kind::Comment:
                break;
        }
    }
    if (!stack.empty()) {
        return fail(where(source, stack.back().line) + "`if` is never closed with `endif`");
    }
    return out;
}

std::string render_path(std::string_view path, const Variables& vars) {
    std::string out;
    size_t i = 0;
    while (i < path.size()) {
        if (path.compare(i, 2, "__") == 0) {
            size_t end = path.find("__", i + 2);
            if (end != std::string_view::npos) {
                std::string_view name = path.substr(i + 2, end - i - 2);
                if (is_identifier(name)) {
                    if (auto v = vars.find(name)) {
                        out += *v;
                        i = end + 2;
                        continue;
                    }
                }
            }
        }
        out += path[i++];
    }
    return out;
}

}  // namespace rangka
