// pch.h: Shared/common header for the psa project.
// This file groups stable, frequently used includes, but it is a normal
// header unless the build is explicitly configured to generate/use a PCH.

#pragma once

// Standard C++ headers
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ranges>
#include <string>
#include <vector>

// Format library - use std::format if available, otherwise use fmt library.
#if defined(__has_include)
    #if __has_include(<format>)
        #include <format>
    #endif
#endif

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
namespace psa {
using std::format;
}
#else
    #include <fmt/format.h>
    #include <fmt/xchar.h>
namespace psa {
using fmt::format;
}
#endif

// Windows-specific headers
#ifdef _WIN32
#include <windows.h>
#include <Psapi.h>
#include <Shlwapi.h>
#include <TlHelp32.h>
#include <tchar.h>
#endif

// Linux-specific headers
#ifdef __linux__
#include <cstdio>
#include <unistd.h>
#include <proc/readproc.h>
#endif

// Project headers that are used across multiple files
#include "general.h"
