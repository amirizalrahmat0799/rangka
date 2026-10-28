#include "check.hpp"

int main() {
    int run = 0;
    for (const auto& c : check::cases()) {
        int before = check::failures();
        c.fn();
        ++run;
        if (check::failures() != before) std::cerr << "  in test: " << c.name << "\n";
    }
    std::cout << run << " tests, " << check::failures() << " failed checks\n";
    return check::failures() == 0 ? 0 : 1;
}
