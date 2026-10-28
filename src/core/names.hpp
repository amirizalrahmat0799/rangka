#pragma once
// Turning what the user typed ("My Shop", "my_shop") into the names each language wants.

#include <string>
#include <string_view>
#include <vector>

#include "core/result.hpp"

namespace rangka {

/// Splits a name into lowercase words: "My Shop-API v2" -> {"my", "shop", "api", "v2"}.
std::vector<std::string> split_words(std::string_view text);

/// "my-shop": the project folder, npm package, Docker image and compose project name.
/// Fails if nothing usable is left, or the name doesn't start with a letter.
Result<std::string> project_name(std::string_view text);

std::string to_kebab(std::string_view text);   // my-shop
std::string to_snake(std::string_view text);   // my_shop
std::string to_pascal(std::string_view text);  // MyShop
std::string to_title(std::string_view text);   // My Shop
std::string to_flat(std::string_view text);    // myshop (Java package segment)

/// Checks a Java package such as "com.example.shop": dot-separated lowercase identifiers, no Java keywords.
Result<std::string> java_package(std::string_view text);

}  // namespace rangka
