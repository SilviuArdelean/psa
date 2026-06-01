/*
 * Copyright (c) 2017-2026 Silviu-Marius Ardelean
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <cctype>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include "Psapi.h"
#include "Shlwapi.h"
#include "TlHelp32.h"
#include "tchar.h"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Shlwapi.lib")
#endif

#ifdef _UNICODE
#define ustrcmp wcscmp
#define ustrcpy wcscpy
#define ustring std::wstring
#define usnprintf _snwprintf
#define ufopen _wfopen
#define ufopen_s _wfopen_s
#define ufprintf fwprintf
#define uprintf wprintf
#define ufputs fputws
#define ustrncat wcsncat
#define ustrlen wcslen
#define uvsnprintf _vsntprintf
#define ustrchr wcschr
#define ustrrchr wcsrchr
#define ustricmp _wcsicmp
#define ustrtol wcstol
#define uostream std::wostream
#define ufscanf_s fwscanf_s
#define ustringstream std::wstringstream
#define itou _itow_s
#define utok wcstok_s
#define SEPARATOR _T("]|[")
#define uprintf_s wprintf
#ifndef __T
#define __T(x) L##x
#endif
#ifndef _T
#define _T(x) __T(x)
#endif
#ifndef TCHAR
#define TCHAR wchar_t
#endif
#define ucout std::wcout
#else
#define ustrcmp strcmp
#define ustring std::string
#define ustrcpy strcpy
#define usnprintf snprintf
#define ufopen fopen
#define ufopen_s _fopen_s
#define ufprintf fprintf
#define uprintf printf
#define ufputs fputs
#define ustrncat strncat
#define ustrlen strlen
#define uvsnprintf _vsntprintf
#define ustrchr strchr
#define ustrrchr strrchr
#define ustrtol strtol
#define ustricmp stricmp
#define uostream std::ostream
#define ufscanf_s fscanf_s
#define ustringstream std::stringstream
#define itou _itoa_s
#define utok strtok_s
#define SEPARATOR "]|["
#define uprintf_s printf
#ifndef _T
#define _T(x) x
#endif
#ifndef TCHAR
#define TCHAR char
#endif
#define ucout std::cout
#endif

#ifdef __linux__
using DWORD = unsigned long;
using ULONG64 = int64_t;
using SIZE_T = std::uint64_t;
#define utoi atoi
#elif _WIN32
#define utoi _ttoi
#endif

struct proc_info {
  int proc_pid = 0;
  int parent_pid = 0;
  ustring proc_name;
  int64_t used_memory = 0;
  ustring cmdline_args;

  proc_info() = default;

  proc_info(int procPID,
            int parentPID,
            ustring procName,
            int64_t usedMemory,
            ustring cmdlineArgs)
      : proc_pid(procPID),
        parent_pid(parentPID),
        proc_name(std::move(procName)),
        used_memory(usedMemory),
        cmdline_args(std::move(cmdlineArgs)) {}

  proc_info(const proc_info& rhs) = default;

  proc_info& operator=(const proc_info& rhs) = default;

  proc_info(proc_info&& rhs) noexcept = default;

  proc_info& operator=(proc_info&& rhs) noexcept = default;
};

struct proc_info_details : public proc_info {
  ustring executable_path;
  std::optional<ustring> parent_name;
  std::optional<ustring> user;
  std::optional<uint32_t> session_id;
  std::optional<ustring> architecture;
  std::optional<ustring> start_time;
  std::optional<ustring> integrity_level;
  std::optional<bool> elevated;

  proc_info_details() = default;
  explicit proc_info_details(const proc_info& base) : proc_info(base) {}
};

using procs_map = std::multimap<DWORD, proc_info>;

constexpr ULONG64 KB_DIVIDER = 1024;
constexpr ULONG64 MB_DIVIDER = 1024 * 1024;
constexpr ULONG64 GB_DIVIDER = 1024 * 1024 * 1024;

constexpr double ToKb(ULONG64 bytes) noexcept {
  return static_cast<double>(bytes) / KB_DIVIDER;
}
constexpr double ToMb(ULONG64 bytes) noexcept {
  return static_cast<double>(bytes) / MB_DIVIDER;
}
constexpr double ToGb(ULONG64 bytes) noexcept {
  return static_cast<double>(bytes) / GB_DIVIDER;
}

constexpr int kFakeRootPID = 999999;
constexpr int kFakeRootParentPID = 1000000;

enum class PSA_INTERNAL_ERRORS : int { invalid_processing_operations = 10001 };
