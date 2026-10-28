#include "cli/terminal.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace term {
namespace {

bool g_color = false;

std::string wrap(std::string_view s, const char* code) {
    if (!g_color) return std::string(s);
    return std::string("\x1b[") + code + "m" + std::string(s) + "\x1b[0m";
}

bool stdin_tty() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

bool stdout_tty() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

enum class Key { Up, Down, Enter, Cancel, Digit, Other };

struct KeyPress {
    Key key;
    int digit = 0;
};

#ifdef _WIN32
KeyPress read_key() {
    int c = _getch();
    if (c == 0 || c == 224) {
        int c2 = _getch();
        if (c2 == 72) return {Key::Up};
        if (c2 == 80) return {Key::Down};
        return {Key::Other};
    }
    if (c == 13) return {Key::Enter};
    if (c == 3 || c == 27) return {Key::Cancel};
    if (c == 'k' || c == 'K') return {Key::Up};
    if (c == 'j' || c == 'J') return {Key::Down};
    if (c >= '1' && c <= '9') return {Key::Digit, c - '0'};
    return {Key::Other};
}

class RawMode {
public:
    RawMode() {}
    ~RawMode() {}
};
#else
class RawMode {
public:
    RawMode() {
        tcgetattr(STDIN_FILENO, &saved_);
        termios raw = saved_;
        raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO | ISIG);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }
    ~RawMode() { tcsetattr(STDIN_FILENO, TCSANOW, &saved_); }

private:
    termios saved_{};
};

int read_byte(bool wait) {
    termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_cc[VMIN] = wait ? 1 : 0;
    t.c_cc[VTIME] = wait ? 0 : 1;  // 0.1 s to tell a lone Esc from an escape sequence
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    unsigned char c;
    return read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
}

KeyPress read_key() {
    int c = read_byte(true);
    if (c == 27) {
        int c2 = read_byte(false);
        if (c2 == -1) return {Key::Cancel};
        if (c2 == '[' || c2 == 'O') {
            int c3 = read_byte(false);
            if (c3 == 'A') return {Key::Up};
            if (c3 == 'B') return {Key::Down};
        }
        return {Key::Other};
    }
    if (c == '\r' || c == '\n') return {Key::Enter};
    if (c == 3 || c == 4 || c == -1) return {Key::Cancel};
    if (c == 'k') return {Key::Up};
    if (c == 'j') return {Key::Down};
    if (c >= '1' && c <= '9') return {Key::Digit, c - '0'};
    return {Key::Other};
}
#endif

void flush() { std::cout.flush(); }

std::string pad(std::string_view s, size_t width) {
    std::string out(s);
    if (out.size() < width) out.append(width - out.size(), ' ');
    return out;
}

const char* pointer() { return g_color ? "\xE2\x9D\xAF" : ">"; }   // ❯
const char* tick() { return g_color ? "\xE2\x9C\x94" : "*"; }      // ✔

}  // namespace

void init() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    bool vt = false;
    if (GetConsoleMode(out, &mode)) {
        vt = SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    }
    g_color = vt && stdout_tty() && std::getenv("NO_COLOR") == nullptr;
#else
    const char* t = std::getenv("TERM");
    g_color = stdout_tty() && std::getenv("NO_COLOR") == nullptr && !(t && std::string_view(t) == "dumb");
#endif
}

bool interactive() { return stdin_tty() && stdout_tty(); }

std::string bold(std::string_view s) { return wrap(s, "1"); }
std::string dim(std::string_view s) { return wrap(s, "2"); }
std::string cyan(std::string_view s) { return wrap(s, "36"); }
std::string green(std::string_view s) { return wrap(s, "32"); }
std::string red(std::string_view s) { return wrap(s, "31"); }
std::string yellow(std::string_view s) { return wrap(s, "33"); }

std::optional<size_t> select(std::string_view question, const std::vector<MenuItem>& items, size_t initial) {
    size_t width = 0;
    for (const auto& item : items) width = std::max(width, item.label.size());

    if (!interactive() || !g_color) {
        // numbered list, answered by typing a number
        std::cout << cyan("?") << " " << bold(question) << "\n";
        for (size_t i = 0; i < items.size(); ++i) {
            std::cout << "  " << (i + 1) << ") " << pad(items[i].label, width) << "  " << dim(items[i].hint) << "\n";
        }
        while (true) {
            std::cout << "  choose 1-" << items.size() << " [" << (initial + 1) << "]: ";
            flush();
            std::string line;
            if (!std::getline(std::cin, line)) return std::nullopt;
            if (line.empty()) return initial;
            char* end = nullptr;
            long n = std::strtol(line.c_str(), &end, 10);
            if (end && *end == '\0' && n >= 1 && static_cast<size_t>(n) <= items.size()) return n - 1;
        }
    }

    size_t current = initial;
    auto draw = [&](bool first) {
        if (!first) std::cout << "\x1b[" << items.size() << "A";
        for (size_t i = 0; i < items.size(); ++i) {
            std::cout << "\r\x1b[2K";
            if (i == current) {
                std::cout << cyan(pointer()) << " " << cyan(pad(items[i].label, width));
            } else {
                std::cout << "  " << pad(items[i].label, width);
            }
            std::cout << "  " << dim(items[i].hint) << "\n";
        }
        flush();
    };

    std::cout << cyan("?") << " " << bold(question) << dim("  (arrow keys, Enter)") << "\n\x1b[?25l";
    draw(true);
    std::optional<size_t> result;
    {
        RawMode raw;
        while (true) {
            KeyPress k = read_key();
            if (k.key == Key::Up) current = current == 0 ? items.size() - 1 : current - 1;
            else if (k.key == Key::Down) current = (current + 1) % items.size();
            else if (k.key == Key::Digit && static_cast<size_t>(k.digit) <= items.size()) current = k.digit - 1;
            else if (k.key == Key::Enter) { result = current; break; }
            else if (k.key == Key::Cancel) break;
            draw(false);
        }
    }
    // collapse the menu into one answered line
    std::cout << "\x1b[" << (items.size() + 1) << "A\r\x1b[J\x1b[?25h";
    if (result) {
        std::cout << green(tick()) << " " << bold(question) << " " << cyan(items[*result].label) << "\n";
    } else {
        std::cout << red("x") << " " << bold(question) << "\n";
    }
    flush();
    return result;
}

namespace {

// prints "? question (hint) " and reads a trimmed line
std::optional<std::string> read_answer(std::string_view question, std::string_view hint) {
    std::cout << cyan("?") << " " << bold(question);
    if (!hint.empty()) std::cout << " " << dim("(" + std::string(hint) + ")");
    std::cout << " ";
    flush();
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cout << "\n";
        return std::nullopt;
    }
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
    size_t start = line.find_first_not_of(' ');
    return start == std::string::npos ? std::string() : line.substr(start);
}

// replaces the prompt line with "✔ question answer"
void show_answer(std::string_view question, std::string_view answer) {
    if (interactive() && g_color) {
        std::cout << "\x1b[1A\r\x1b[2K" << green(tick()) << " " << bold(question) << " " << cyan(answer) << "\n";
    }
}

}  // namespace

std::optional<std::string> ask(std::string_view question, std::string_view fallback) {
    auto line = read_answer(question, fallback);
    if (!line) return std::nullopt;
    std::string answer = line->empty() ? std::string(fallback) : *line;
    show_answer(question, answer);
    return answer;
}

std::optional<bool> confirm(std::string_view question, bool fallback) {
    while (true) {
        auto a = read_answer(question, fallback ? "Y/n" : "y/N");
        if (!a) return std::nullopt;
        std::optional<bool> yes;
        if (a->empty()) yes = fallback;
        else if (*a == "y" || *a == "Y" || *a == "yes") yes = true;
        else if (*a == "n" || *a == "N" || *a == "no") yes = false;
        if (yes) {
            show_answer(question, *yes ? "Yes" : "No");
            return yes;
        }
    }
}

}  // namespace term
