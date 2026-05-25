# ---------------------------------------------------------------------------
# psa — main executable
# ---------------------------------------------------------------------------

# --- Source files shared across all platforms ---
set(PSA_SOURCES
    psa.cpp
    operations.cpp
    processes_tree_builder.cpp
)

set(PSA_HEADERS
    operations.h
    processes_tree_builder.h
    pch.h
)

# --- Windows-only helper (XGetopt — replaces POSIX getopt) ---
if(WIN32)
    list(APPEND PSA_SOURCES psa-win/XGetopt.cpp)
    list(APPEND PSA_HEADERS psa-win/XGetopt.h psa-win/targetver.h)
endif()

add_executable(psa ${PSA_SOURCES} ${PSA_HEADERS})

# --- Include directories ---
target_include_directories(psa PRIVATE
    "${PSA_INCLUDE_DIR}"   # root include/
    "${CMAKE_CURRENT_SOURCE_DIR}"  # src/ (for operations.h, processes_tree_builder.h)
)

# ---------------------------------------------------------------------------
# Precompiled header — improves build performance
# ---------------------------------------------------------------------------
target_precompile_headers(psa PRIVATE pch.h)

# ---------------------------------------------------------------------------
# Platform-specific configuration
# ---------------------------------------------------------------------------

# ---- Windows ---------------------------------------------------------------
if(WIN32)
    target_compile_definitions(psa PRIVATE
        _WIN32
        _CONSOLE
        UNICODE
        _UNICODE
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )

    # SDL security checks (mirrors <SDLCheck>true</SDLCheck>)
    if(MSVC)
        target_compile_options(psa PRIVATE
            /W3          # Warning level 3
            /sdl         # SDL checks
            $<$<CONFIG:Release>:/O2>   # MaxSpeed
            $<$<CONFIG:Release>:/Oi>   # Intrinsics
            $<$<CONFIG:Release>:/Gy>   # FunctionLevelLinking
            $<$<CONFIG:Debug>:/Od>
            $<$<CONFIG:Debug>:/Zi>
        )
        target_link_options(psa PRIVATE
            $<$<CONFIG:Release>:/OPT:REF>   # Remove unreferenced functions/data
            $<$<CONFIG:Release>:/OPT:ICF>   # COMDAT folding
        )
    endif()

    # Windows system libraries (replaces #pragma comment(lib,...) for external consumers)
    target_link_libraries(psa PRIVATE psapi shlwapi)

    # Debug binary is named "psad", Release is "psa" — matches TargetName in .vcxproj
    set_target_properties(psa PROPERTIES
        OUTPUT_NAME_DEBUG   "psad"
        OUTPUT_NAME_RELEASE "psa"
    )

# ---- Linux -----------------------------------------------------------------
elseif(UNIX AND NOT APPLE)
    # libprocps-dev  (sudo apt install libprocps-dev)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PROCPS REQUIRED libprocps)

    target_include_directories(psa PRIVATE ${PROCPS_INCLUDE_DIRS})
    target_link_libraries(psa PRIVATE ${PROCPS_LIBRARIES})
    target_compile_options(psa PRIVATE
        ${PROCPS_CFLAGS_OTHER}
        -Wall
        $<$<CONFIG:Debug>:-fpermissive>   # matches AdditionalOptions in psa-lin.vcxproj Debug|x64
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O2>
    )
    target_link_options(psa PRIVATE
        -Wl,-z,relro
        -Wl,-z,noexecstack
        -Wl,-z,now
    )

    target_compile_definitions(psa PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )

# ---- macOS -----------------------------------------------------------------
elseif(APPLE)
    # procps is not available on macOS; if you port process listing to macOS
    # APIs (libproc / sysctl), add the framework here, e.g.:
    # find_library(CORE_FOUNDATION CoreFoundation REQUIRED)
    # target_link_libraries(psa PRIVATE ${CORE_FOUNDATION})
    target_compile_options(psa PRIVATE
        -Wall
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O2>
    )
    target_compile_definitions(psa PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
    message(STATUS "macOS build: process-listing features may require additional porting.")
endif()
