cmake_minimum_required(VERSION 3.20)
project(rangka VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# The version shown by `rangka --version` and written into generated READMEs. Release builds pass the tag.
set(RANGKA_VERSION "${PROJECT_VERSION}" CACHE STRING "Version string")
option(RANGKA_BUILD_TESTS "Build the tests" ON)

# One exe with nothing to install: link the C++ runtime statically on Windows and Linux release builds.
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  add_compile_options(/W4 /permissive- /utf-8 /Zc:__cplusplus)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- templates/ compiled into the program ----------------------------------------------------------------
add_executable(rangka_embed tools/embed.cpp)
file(GLOB_RECURSE RANGKA_TEMPLATES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/templates/*")
set(RANGKA_EMBEDDED "${CMAKE_BINARY_DIR}/generated/templates.cpp")
add_custom_command(
  OUTPUT "${RANGKA_EMBEDDED}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/generated"
  COMMAND rangka_embed "${CMAKE_SOURCE_DIR}/templates" "${RANGKA_EMBEDDED}"
  DEPENDS rangka_embed ${RANGKA_TEMPLATES}
  COMMENT "Embedding templates"
  VERBATIM)

# --- core: everything except the terminal, shared with the WebAssembly build -----------------------------
add_library(rangka_core STATIC
  src/core/catalog.cpp
  src/core/generator.cpp
  src/core/names.cpp
  src/core/template.cpp
  src/core/zip.cpp
  "${RANGKA_EMBEDDED}")
target_include_directories(rangka_core PUBLIC src)
target_compile_definitions(rangka_core PUBLIC RANGKA_VERSION="${RANGKA_VERSION}")

# --- the command-line tool -------------------------------------------------------------------------------
add_executable(rangka src/cli/main.cpp src/cli/terminal.cpp src/cli/writer.cpp)
target_link_libraries(rangka PRIVATE rangka_core)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_link_options(rangka PRIVATE -static-libstdc++ -static-libgcc)
endif()
install(TARGETS rangka RUNTIME DESTINATION bin)

if(RANGKA_BUILD_TESTS)
  enable_testing()
  add_executable(rangka_tests
    tests/main.cpp
    tests/names_test.cpp
    tests/template_test.cpp
    tests/generator_test.cpp
    tests/zip_test.cpp)
  target_link_libraries(rangka_tests PRIVATE rangka_core)
  add_test(NAME rangka_tests COMMAND rangka_tests)
endif()
