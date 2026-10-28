#pragma once
// The template language used by every file under templates/.
//
//   {% name %}                   the value of a variable
//   {% if react %} ... {% endif %}
//   {% if postgres and not spring %} ... {% else %} ... {% endif %}
//   {%# a comment %}
//
// Conditions are variable names combined with `not`, `and` and `or` (`and` binds tighter); a variable is true
// when it is set and not empty. A line holding nothing but an if/else/endif/comment tag disappears completely,
// so templates can be indented naturally without leaving blank lines behind.
//
// `{%` was chosen because nothing generated here uses it: Vue and GitHub Actions use `{{ }}`, C++ uses `[[ ]]`.
//
// Unknown variables are errors, not empty strings, so a typo in a template fails the tests instead of shipping.

#include <map>
#include <string>
#include <string_view>

#include "core/result.hpp"

namespace rangka {

class Variables {
public:
    void set(const std::string& name, std::string value) { values_[name] = std::move(value); }
    void set(const std::string& name, const char* value) { values_[name] = value; }
    void set(const std::string& name, bool value) { values_[name] = value ? "true" : ""; }

    bool has(std::string_view name) const { return values_.find(name) != values_.end(); }
    const std::string* find(std::string_view name) const;
    bool truthy(std::string_view name) const;
    const std::map<std::string, std::string, std::less<>>& all() const { return values_; }

private:
    std::map<std::string, std::string, std::less<>> values_;
};

/// Renders template text. `source` names the template in error messages ("backend/go/main.go").
Result<std::string> render(std::string_view text, const Variables& vars, std::string_view source = "template");

/// Replaces `__name__` in a template path with the variable `name`, when such a variable exists
/// ("src/main/java/__package_path__/App.java"). Unknown names such as Python's `__init__` are left alone.
std::string render_path(std::string_view path, const Variables& vars);

}  // namespace rangka
