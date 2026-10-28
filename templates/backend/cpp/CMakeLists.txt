cmake_minimum_required(VERSION 3.24)
project({% snake %} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Dependencies: use installed packages when there are some (vcpkg, Homebrew, apt), download them otherwise.
include(FetchContent)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(httplib
  URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v{% v_httplib %}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  FIND_PACKAGE_ARGS CONFIG)
FetchContent_Declare(nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v{% v_json %}/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  FIND_PACKAGE_ARGS CONFIG)
FetchContent_MakeAvailable(httplib nlohmann_json)
find_package(Threads REQUIRED)
{% if postgres %}
find_package(PostgreSQL REQUIRED)
{% endif %}

# everything except main(), so the tests can link it too
add_library(app_core STATIC
  src/api.cpp
  src/memory_store.cpp
{% if postgres %}
  src/postgres_store.cpp
{% endif %}
)
target_include_directories(app_core PUBLIC src)
target_link_libraries(app_core PUBLIC httplib::httplib nlohmann_json::nlohmann_json Threads::Threads)
{% if postgres %}
target_link_libraries(app_core PUBLIC PostgreSQL::PostgreSQL)
{% endif %}
if(MSVC)
  target_compile_options(app_core PUBLIC /W4 /utf-8)
else()
  target_compile_options(app_core PUBLIC -Wall -Wextra)
endif()

add_executable({% snake %} src/main.cpp)
target_link_libraries({% snake %} PRIVATE app_core)

enable_testing()
add_executable(api_test tests/api_test.cpp)
target_link_libraries(api_test PRIVATE app_core)
add_test(NAME api_test COMMAND api_test)
