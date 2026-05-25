# ---------------------------------------------------------------------------
# testGenericTree — unit-test executable
# ---------------------------------------------------------------------------

add_executable(testGenericTree
    testGenericTree.cpp
    # stdafx.cpp is a legacy PCH stub; its content is just the PCH create step.
    # CMake uses target_precompile_headers instead — no need to compile it separately.
)

target_include_directories(testGenericTree PRIVATE
    "${PSA_INCLUDE_DIR}"                       # include/  (generic_tree.h, general.h, etc.)
    "${CMAKE_CURRENT_SOURCE_DIR}"              # testing/testGenericTree/ (stdafx.h, targetver.h)
)

# ---------------------------------------------------------------------------
# Precompiled header — modern CMake equivalent of the old <PrecompiledHeader>
# Use / Create pattern from testGenericTree.vcxproj
# stdafx.h is Windows-specific, so only enable it for WIN32 builds.
# ---------------------------------------------------------------------------
if(WIN32)
    target_precompile_headers(testGenericTree PRIVATE stdafx.h)
endif()

# ---------------------------------------------------------------------------
# Platform flags  (test project mirrors psa-win defines; no procps needed)
# ---------------------------------------------------------------------------
if(WIN32)
    target_compile_definitions(testGenericTree PRIVATE
        _WIN32
        _CONSOLE
        UNICODE
        _UNICODE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
    if(MSVC)
        target_compile_options(testGenericTree PRIVATE
            /W3
            /sdl
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:Release>:/Oi>
            $<$<CONFIG:Release>:/Gy>
            $<$<CONFIG:Debug>:/Od>
            $<$<CONFIG:Debug>:/Zi>
        )
        target_link_options(testGenericTree PRIVATE
            $<$<CONFIG:Release>:/OPT:REF>
            $<$<CONFIG:Release>:/OPT:ICF>
        )
    endif()
elseif(UNIX)
    target_compile_options(testGenericTree PRIVATE
        -Wall
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O2>
    )
    target_compile_definitions(testGenericTree PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
endif()

# ---------------------------------------------------------------------------
# CTest integration — enable_testing() is called in the root CMakeLists.txt
# ---------------------------------------------------------------------------
# Use $<TARGET_FILE:...> so CTest resolves the exact binary path at build time,
# including multi-config generators (Visual Studio / Ninja Multi-Config) that
# place outputs under bin/<Config>/ rather than bin/.
add_test(NAME testGenericTree COMMAND $<TARGET_FILE:testGenericTree>)
