# ---------------------------------------------------------------------------
# testPsaCli — GTest unit tests for ProcessCommandLine()
# ---------------------------------------------------------------------------

include(FetchContent)

# ---------------------------------------------------------------------------
# Google Test v1.14.0 — fetched at configure time
# ---------------------------------------------------------------------------
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    URL      https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    URL_HASH SHA256=1f357c27ca988c3f7c6b4bf68a9395005ac6761f034046e9dde0896e3aba00e4
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# ---------------------------------------------------------------------------
# Sources — reuse psa.cpp + operations.cpp + processes_tree_builder.cpp
# The PSA_TEST_BUILD define guards out _tmain / main so GTest can provide
# its own entry point.
# ---------------------------------------------------------------------------
set(TEST_PSA_SOURCES
    test_psa_cli.cpp
    test_psa_cli_integration.cpp
    test_psa_cli_main.cpp
    "${CMAKE_SOURCE_DIR}/src/psa.cpp"
    "${CMAKE_SOURCE_DIR}/src/operations.cpp"
    "${CMAKE_SOURCE_DIR}/src/processes_tree_builder.cpp"
)

if(WIN32)
    list(APPEND TEST_PSA_SOURCES
        "${CMAKE_SOURCE_DIR}/src/psa-win/XGetopt.cpp"
    )
endif()

add_executable(test_psa_cli ${TEST_PSA_SOURCES})

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(test_psa_cli PRIVATE
    "${PSA_INCLUDE_DIR}"                       # include/  (general.h, etc.)
    "${CMAKE_SOURCE_DIR}/src"                  # src/ (operations.h, pch.h, etc.)
    "${CMAKE_CURRENT_SOURCE_DIR}"              # testing/testPsaCli/ (stdafx.h, targetver.h)
)

# ---------------------------------------------------------------------------
# Precompiled header — pch.h is #include "pch.h" line 1 in every .cpp.
# Must be unconditional (same as src/CMakeLists.txt) so Linux also gets
# the /FI (MSVC) or -include (GCC) injection that satisfies the first include.
# ---------------------------------------------------------------------------
target_precompile_headers(test_psa_cli PRIVATE
    "${CMAKE_SOURCE_DIR}/src/pch.h"
)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
target_compile_definitions(test_psa_cli PRIVATE
    PSA_TEST_BUILD          # suppresses _tmain / main so GTest owns the entry point
)

if(WIN32)
    target_compile_definitions(test_psa_cli PRIVATE
        _WIN32
        _CONSOLE
        UNICODE
        _UNICODE
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
    if(MSVC)
        target_compile_options(test_psa_cli PRIVATE
            /W3
            /sdl
            $<$<CONFIG:Debug>:/Od>
            $<$<CONFIG:Debug>:/Zi>
            $<$<CONFIG:Release>:/O2>
        )
    endif()
    target_link_libraries(test_psa_cli PRIVATE psapi shlwapi GTest::gtest)
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PROCPS REQUIRED libprocps)

    target_include_directories(test_psa_cli PRIVATE ${PROCPS_INCLUDE_DIRS})
    target_link_libraries(test_psa_cli PRIVATE ${PROCPS_LIBRARIES} GTest::gtest)
    target_compile_options(test_psa_cli PRIVATE
        ${PROCPS_CFLAGS_OTHER}
        -Wswitch
        -Wno-deprecated-declarations
        -Wempty-body
        -Wconversion
        -Wreturn-type
        -Wparentheses
        -Wno-format
        -Wuninitialized
        -Wunused-function
        -Wunused-value
        -Wunused-variable
        -fpermissive
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:RelWithDebInfo>:-O2>
    )
    target_compile_definitions(test_psa_cli PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:RelWithDebInfo>:NDEBUG>
        $<$<CONFIG:MinSizeRel>:NDEBUG>
    )
    target_link_options(test_psa_cli PRIVATE
        -Wl,-z,relro
        -Wl,-z,noexecstack
        -Wl,-z,now
    )
endif()

# ---------------------------------------------------------------------------
# CTest integration — each TEST/TEST_F becomes a separate CTest case
# ---------------------------------------------------------------------------
include(GoogleTest)
gtest_discover_tests(test_psa_cli)
