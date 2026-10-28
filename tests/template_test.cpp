#include "check.hpp"
#include "core/template.hpp"

using namespace rangka;

namespace {
Variables vars() {
    Variables v;
    v.set("name", "shop");
    v.set("react", true);
    v.set("vue", false);
    v.set("postgres", true);
    return v;
}
std::string r(std::string_view text) {
    auto out = render(text, vars(), "t");
    if (!out) return "ERROR: " + out.error();
    return *out;
}
}  // namespace

TEST("variables are substituted") {
    CHECK_EQ(r("hello {% name %}!"), "hello shop!");
    CHECK_EQ(r("{%name%}-{% name %}"), "shop-shop");
    CHECK_EQ(r("no tags {{ vue }} ${{ github }}"), "no tags {{ vue }} ${{ github }}");
}

TEST("inline conditions") {
    CHECK_EQ(r("a{% if react %}R{% endif %}b"), "aRb");
    CHECK_EQ(r("a{% if vue %}V{% else %}X{% endif %}b"), "aXb");
    CHECK_EQ(r("{% if not vue %}1{% endif %}"), "1");
    CHECK_EQ(r("{% if react and postgres %}1{% endif %}"), "1");
    CHECK_EQ(r("{% if vue and postgres %}1{% endif %}"), "");
    CHECK_EQ(r("{% if vue or postgres %}1{% endif %}"), "1");
    CHECK_EQ(r("{% if vue or react and not postgres %}1{% else %}0{% endif %}"), "0");
    CHECK_EQ(r("{% if not not react %}1{% endif %}"), "1");
}

TEST("nested conditions") {
    CHECK_EQ(r("{% if react %}a{% if vue %}b{% else %}c{% endif %}d{% endif %}"), "acd");
    CHECK_EQ(r("{% if vue %}a{% if react %}b{% endif %}{% else %}e{% endif %}"), "e");
}

TEST("lines holding only a block tag disappear") {
    CHECK_EQ(r("one\n{% if react %}\ntwo\n{% endif %}\nthree\n"), "one\ntwo\nthree\n");
    CHECK_EQ(r("one\n    {% if vue %}\n    two\n    {% else %}\n    zwei\n    {% endif %}\nthree\n"),
             "one\n    zwei\nthree\n");
    CHECK_EQ(r("{% if react %}\nfirst line\n{% endif %}"), "first line\n");
    CHECK_EQ(r("a\n{%# a comment %}\nb\n"), "a\nb\n");
    CHECK_EQ(r("a\n{% if vue %}\nb\n{% endif %}"), "a\n");
}

TEST("tags sharing a line with text keep the line") {
    CHECK_EQ(r("x {% if react %}y{% endif %}\nz\n"), "x y\nz\n");
    CHECK_EQ(r("  {% name %}\n"), "  shop\n");
}

TEST("mistakes are reported with the line") {
    auto bad = [](std::string_view text) { return !render(text, vars(), "t").ok(); };
    CHECK(bad("{% nope %}"));
    CHECK(bad("{% if nope %}x{% endif %}"));
    CHECK(bad("{% if react %}x"));
    CHECK(bad("x{% endif %}"));
    CHECK(bad("{% else %}"));
    CHECK(bad("{% if react %}{% else %}{% else %}{% endif %}"));
    CHECK(bad("{% if react vue %}{% endif %}"));
    CHECK(bad("{% if react and %}{% endif %}"));
    CHECK(bad("{% name"));
    CHECK(bad("{% two words %}"));
    auto out = render("a\nb\n{% missing %}", vars(), "file.txt");
    CHECK(!out.ok());
    CHECK(out.error().find("file.txt:3") == 0);
}

TEST("paths") {
    Variables v;
    v.set("package_path", "com/example/shop");
    v.set("app_class", "ShopApplication");
    CHECK_EQ(render_path("src/main/java/__package_path__/__app_class__.java", v),
             "src/main/java/com/example/shop/ShopApplication.java");
    CHECK_EQ(render_path("app/__init__.py", v), "app/__init__.py");
    CHECK_EQ(render_path("a__b", v), "a__b");
}
