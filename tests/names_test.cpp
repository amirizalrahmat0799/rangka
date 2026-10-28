#include "check.hpp"
#include "core/names.hpp"

using namespace rangka;

TEST("words are split on punctuation and camelCase") {
    auto w = split_words("My Shop-API v2");
    CHECK_EQ(w.size(), 4u);
    CHECK_EQ(w[0], "my");
    CHECK_EQ(w[2], "api");
    CHECK_EQ(split_words("myShopApi").size(), 3u);
    CHECK_EQ(split_words("__init__").size(), 1u);
}

TEST("names in each style") {
    CHECK_EQ(to_kebab("My Shop"), "my-shop");
    CHECK_EQ(to_snake("my-shop"), "my_shop");
    CHECK_EQ(to_pascal("my_shop"), "MyShop");
    CHECK_EQ(to_title("my-shop"), "My Shop");
    CHECK_EQ(to_flat("My Shop 2"), "myshop2");
}

TEST("project names") {
    CHECK_EQ(*project_name("  Kedai Runcit  "), "kedai-runcit");
    CHECK(!project_name("!!!").ok());
    CHECK(!project_name("2fast").ok());
    CHECK(!project_name(std::string(70, 'a')).ok());
}

TEST("java packages") {
    CHECK(java_package("com.example.shop").ok());
    CHECK(java_package("my.app_2").ok());
    CHECK(!java_package("com.Example").ok());
    CHECK(!java_package("com..shop").ok());
    CHECK(!java_package("com.example.class").ok());
    CHECK(!java_package("").ok());
    CHECK(!java_package("com.my-shop").ok());
}
