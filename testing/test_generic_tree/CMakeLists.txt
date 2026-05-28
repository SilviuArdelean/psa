# ---------------------------------------------------------------------------
# test_generic_tree — GTest unit tests for generic_tree
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

add_executable(test_generic_tree
    test_generic_tree.cpp
)

target_include_directories(test_generic_tree PRIVATE
    "${PSA_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(WIN32)
    target_compile_definitions(test_generic_tree PRIVATE
        _WIN32
        _CONSOLE
        UNICODE
        _UNICODE
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
    if(MSVC)
        target_compile_options(test_generic_tree PRIVATE
            /W3
            /sdl
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:Release>:/Oi>
            $<$<CONFIG:Release>:/Gy>
            $<$<CONFIG:Debug>:/Od>
            $<$<CONFIG:Debug>:/Zi>
        )
        target_link_options(test_generic_tree PRIVATE
            $<$<CONFIG:Release>:/OPT:REF>
            $<$<CONFIG:Release>:/OPT:ICF>
        )
    endif()
    target_link_libraries(test_generic_tree PRIVATE psapi shlwapi GTest::gtest GTest::gtest_main)
elseif(UNIX)
    target_compile_options(test_generic_tree PRIVATE
        -Wall
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O2>
    )
    target_compile_definitions(test_generic_tree PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
    target_link_libraries(test_generic_tree PRIVATE GTest::gtest GTest::gtest_main)
endif()

# ---------------------------------------------------------------------------
# CTest integration — each TEST/TEST_F becomes a separate CTest case
# ---------------------------------------------------------------------------
include(GoogleTest)
gtest_discover_tests(test_generic_tree)
