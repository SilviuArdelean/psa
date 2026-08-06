cmake_minimum_required(VERSION 3.16)  # 3.16+ required for target_precompile_headers()

project(psa
    VERSION 1.0.0
    DESCRIPTION "Processes Status Analysis"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# fmt library for std::format fallback on older compilers
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    fmt
    URL      https://github.com/fmtlib/fmt/archive/refs/tags/10.2.1.zip
    URL_HASH SHA256=312151a2d13c8327f5c9c586ac6cf7cddc1658e8f53edae0ec56509c8fa516c9
)
FetchContent_MakeAvailable(fmt)

# ---------------------------------------------------------------------------
# Output directories  (mirrors the old Build/<Config>/<Platform>/ layout)
# ---------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ---------------------------------------------------------------------------
# Shared include path (../../include relative to every sub-project)
# ---------------------------------------------------------------------------
set(PSA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# ---------------------------------------------------------------------------
# Testing — enable_testing() MUST be called in the root scope so that
# ctest can discover tests registered in subdirectories.
# ---------------------------------------------------------------------------
enable_testing()

# Always print test output and summary on failure
set(CTEST_OUTPUT_ON_FAILURE ON)

# ---------------------------------------------------------------------------
# Sub-projects
# ---------------------------------------------------------------------------
add_subdirectory(src)
add_subdirectory(testing/test_generic_tree)
add_subdirectory(testing/test_psa_cli)
