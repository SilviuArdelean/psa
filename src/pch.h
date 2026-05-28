// pch.h: Shared/common header for the psa project.
// This file groups stable, frequently used includes, but it is a normal
// header unless the build is explicitly configured to generate/use a PCH.

#pragma once

// Standard C++ headers
#include <algorithm>
#include <cctype>
#include <cstring>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ranges>
#include <string>
#include <vector>

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
