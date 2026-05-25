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

# ---------------------------------------------------------------------------
# Sub-projects
# ---------------------------------------------------------------------------
add_subdirectory(src)
if(WIN32)
    add_subdirectory(testing/testGenericTree)
endif()
