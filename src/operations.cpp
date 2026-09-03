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

#include "operations.h"
#include "pch.h"

#include <cstdlib>
#include <sstream>
#include "fixed_queue.h"
#include "notification.h"
#include "process_actions.h"
#include "processes_tree_builder.h"
#include "string_utils.h"
#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef _WIN32
#include "smart_handler.h"
#endif

namespace {

ustring format_memory(ULONG64 bytes, bool no_decimals = false) {
  if (bytes >= GB_DIVIDER)
    return no_decimals ? psa::format(_T("{:.0f} GB"), ToGb(bytes))
                       : psa::format(_T("{:.2f} GB"), ToGb(bytes));
  if (bytes >= MB_DIVIDER)
    return no_decimals ? psa::format(_T("{:.0f} MB"), ToMb(bytes))
                       : psa::format(_T("{:.2f} MB"), ToMb(bytes));
  return no_decimals ? psa::format(_T("{:.0f} KB"), ToKb(bytes))
                     : psa::format(_T("{:.2f} KB"), ToKb(bytes));
}

void print_process_row(const proc_info& process, bool bracket_pid) {
  const auto mem = format_memory(static_cast<ULONG64>(process.used_memory));
#ifdef _WIN32
  if (bracket_pid) {
    uprintf_s(_T("[PID: %d] \t %s \t %-15s \n"), process.proc_pid, mem.c_str(),
              process.proc_name.c_str());
    return;
  }
  uprintf_s(_T("PID [%d] \t %s \t %-15s \n"), process.proc_pid, mem.c_str(),
            process.proc_name.c_str());
#else
  if (bracket_pid) {
    uprintf_s("[PID: %d] \t %s \t %-15s \n", process.proc_pid, mem.c_str(),
              process.proc_name.c_str());
    return;
  }
  uprintf_s("PID [%d] \t %s \t %-15s \n", process.proc_pid, mem.c_str(),
            process.proc_name.c_str());
#endif
}

void print_total_memory_summary(ULONG64 total_memory, int process_count) {
  ucout << _T("-----------------------------------") << std::endl;
  ucout << _T("   Total used memory: ") << format_memory(total_memory, true)
        << _T("  [ ") << process_count << _T(" processes ]") << std::endl;
}

void print_all_processes_summary(ULONG64 total_memory, int process_count) {
  ucout << _T("--------------------------------------------------------")
        << std::endl;
  ucout << _T("   Total used memory: ") << format_memory(total_memory, true)
        << _T("  [ ") << process_count << _T(" processes ]") << std::endl;
}
void print_insufficient_privileges_message() {
#ifdef _WIN32
  ucout << _T("Seems psa.exe application runs under not enough privileges. ")
           _T("Please launch it with administrator privileges.")
        << std::endl;
#else
  ucout << _T("Seems psa application runs under not enough privileges. ")
           _T("Please ")
           _T("run it by root privileges.")
        << std::endl;
#endif
}

bool process_matches_filter(const proc_info& process,
                            const ustring& filter,
                            bool filter_is_pid) {
  if (filter_is_pid) {
    return process.proc_pid == utoi(filter.c_str());
  }

  return string_utils::search_substring(process.proc_name, filter);
}

ustring resolve_process_details(const proc_info& process) {
  if (!process.cmdline_args.empty()) {
    return process.cmdline_args;
  }

  return process_actions::get_process_cmdline(process.proc_pid);
}

void print_process_details(const proc_info& process) {
  const auto cmdline = resolve_process_details(process);
  if (!cmdline.empty()) {
    ucout << _T("[ Command Line: ") << cmdline.c_str() << _T(" ]") << std::endl;
    return;
  }

  const auto path = process_actions::get_process_path(process.proc_pid);
  if (!path.empty()) {
    ucout << _T("   path: [ ") << path.c_str() << _T(" ]") << std::endl;
  }
}

}  // namespace

ProcessingOperations::ProcessingOperations(void) {}

bool ProcessingOperations::EnsureProcessesMap() {
  if (map_processes_.empty())
    return BuildProcessesMap();
  return true;
}

bool ProcessingOperations::BuildProcessesMap() {
  std::lock_guard<std::mutex> lock(map_mutex_);

#ifdef _WIN32
  PROCESSENTRY32 pe32;

  smart_handle hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hProcessSnap == INVALID_HANDLE_VALUE) {
    PrintError(TEXT("CreateToolhelp32Snapshot (of processes)"));
    return FALSE;
  }

  pe32.dwSize = sizeof(PROCESSENTRY32);

  // Start iteration from the first process entry.
  if (!Process32First(hProcessSnap, &pe32)) {
    PrintError(TEXT("Retrieve information about the first process has failed"));
    return FALSE;
  }

  map_processes_.clear();

  do {
    map_processes_.emplace(
        pe32.th32ProcessID,
        proc_info(pe32.th32ProcessID, pe32.th32ParentProcessID, pe32.szExeFile,
                  GetProcessUsedMemory(pe32.th32ProcessID),
                  process_actions::get_process_cmdline(pe32.th32ProcessID)));

  } while (Process32Next(hProcessSnap, &pe32));

#else

  PROCTAB* proc = openproc(PROC_FILLARG | PROC_FILLSTAT);

  // Keep this static to avoid procps readproc crashes seen on older Ubuntu.
  static proc_t _process;

  memset(&_process, 0, sizeof(_process));
  map_processes_.clear();

  while (readproc(proc, &_process) != NULL) {
    map_processes_.emplace(
        _process.tid,
        proc_info(_process.tid, _process.ppid,
                  (_process.cmdline != NULL) ? *_process.cmdline : _process.cmd,
                  _process.vsize,
                  process_actions::get_process_cmdline(_process.tid)));
  }

  closeproc(proc);

#endif

  return true;
}

void ShowHeader() {
  ucout << "------------------------------------------------\n";
  ucout << "  PID \t\t RAM Usage \t Process Name \n";
  ucout << "------------------------------------------------\n";
}

void ProcessingOperations::PrintTopExpensiveProcesses(const int top) {
  if (!EnsureProcessesMap())
    return;

  if (map_processes_.empty())
    ucout << "Processes list is empty" << std::endl;

  struct data4sort {
    int pid = 0;
    ustring proc_name;
    ULONG64 mem_usage = 0;

    bool operator>(const data4sort& rhs) const {
      return mem_usage > rhs.mem_usage;
    }

    bool operator<(const data4sort& rhs) const {
      return mem_usage < rhs.mem_usage;
    }
  };

  struct LessThanByFileSize {
    bool operator()(const data4sort& lhs, const data4sort& rhs) const {
      return lhs.mem_usage < rhs.mem_usage;
    }
  };

  fixed_queue<data4sort, std::vector<data4sort>, LessThanByFileSize> top_queue(
      top);

  for (const auto& proc : map_processes_ | std::views::values) {
    top_queue.push({.pid = proc.proc_pid,
                    .proc_name = proc.proc_name,
                    .mem_usage = static_cast<ULONG64>(proc.used_memory)});
  }

  ULONG64 processesAllSize = 0;

  ucout << _T(" Top ") << top << _T(" most consuming memory processes \n");
  ShowHeader();

  // Copy to a vector, then sort it for display without mutating fixed_queue.
  std::vector<data4sort> sorted(top_queue.begin(), top_queue.end());
  std::sort(sorted.begin(), sorted.end(), [](const auto& l, const auto& r) {
    return l.mem_usage > r.mem_usage;
  });

  for (const auto& ob : sorted) {
    uprintf_s(_T(" [%d] \t %s \t %-15s \n"), ob.pid,
              format_memory(ob.mem_usage).c_str(), ob.proc_name.c_str());

    processesAllSize += ob.mem_usage;
  }

  ucout << _T("-------------------------------------------") << std::endl;
  ucout << _T("   Total used memory: ") << format_memory(processesAllSize, true)
        << std::endl;
}

bool ProcessingOperations::get_filter_results(const ustring& process_name,
                                              const ustring& filter) {
  return ((filter.empty() || std::string::npos != filter.find(_T("*"))
               ? true
               : string_utils::search_substring(filter, process_name)));
}

bool ProcessingOperations::PrintAllProcessesInformation(
    bool const show_details) {
  int process_count = 0;
  ULONG64 all_process_size = 0;

  if (!EnsureProcessesMap())
    return false;

  ShowHeader();

  for (const auto& proc : map_processes_ | std::views::values) {
    print_process_row(proc, false);

    if (show_details)
      static_cast<void>(PrintProcessDetailedInfo(proc.proc_pid));

    all_process_size += proc.used_memory;
    process_count++;
  }

  if (process_count != 0) {
    print_all_processes_summary(all_process_size, process_count);
  }

  return true;
}

bool ProcessingOperations::PrintProcessInformation(const ustring& filter,
                                                   bool const show_details) {
  int process_count = 0;
  ULONG64 all_processes_size = 0;

  if (!EnsureProcessesMap())
    return false;

  ShowHeader();

  const bool filter_is_pid = string_utils::is_number(filter);
  for (const auto& proc : map_processes_ | std::views::values) {
    if (!process_matches_filter(proc, filter, filter_is_pid)) {
      continue;
    }

    print_process_row(proc, true);

    if (show_details) {
      print_process_details(proc);
    }

    all_processes_size += proc.used_memory;
    process_count++;
  }

  if (process_count != 0) {
    if (0 != all_processes_size) {
      print_total_memory_summary(all_processes_size, process_count);
    } else {
      print_insufficient_privileges_message();
    }
  } else {
    ucout << _T("Undetected process with '") << filter << _T("' name.")
          << std::endl;
  }

  return true;
}

bool ProcessingOperations::PrintProcessInformationWithCmdlineFilter(
    const ustring& process_name,
    const ustring& cmdline_filter,
    bool const show_details) {
  int process_count = 0;
  ULONG64 all_processes_size = 0;

  if (!EnsureProcessesMap())
    return false;

  if (process_name.empty() || cmdline_filter.empty())
    return false;

  ShowHeader();

  const bool filter_is_pid = string_utils::is_number(process_name);
  for (const auto& proc : map_processes_ | std::views::values) {
    if (!process_matches_filter(proc, process_name, filter_is_pid)) {
      continue;
    }

    if (!string_utils::search_substring(proc.cmdline_args, cmdline_filter)) {
      continue;
    }

    print_process_row(proc, true);

    if (show_details) {
      print_process_details(proc);
    }

    all_processes_size += proc.used_memory;
    process_count++;
  }

  if (process_count != 0) {
    if (0 != all_processes_size) {
      print_total_memory_summary(all_processes_size, process_count);
    } else {
      print_insufficient_privileges_message();
    }
  } else {
    ucout << _T("No processes matched '") << process_name
          << _T("' with command line containing '") << cmdline_filter
          << _T("'.") << std::endl;
  }

  return true;
}

std::optional<proc_info_details> ProcessingOperations::GetProcessInfoByPid(
    const uint32_t process_pid) {
  return process_actions::get_process_details_info(process_pid);
}

void ProcessingOperations::PrintProcessInfoReport(
    const proc_info_details& details) {
  process_actions::print_process_info_report(details);
}

bool ProcessingOperations::PrintProcessDetailedInfo(DWORD pid) {
  // TO DO
  // command line
  // started from
  // PEB address
  // parent information

  return true;
}

void ProcessingOperations::GenerateProcessesTree(int const proc_pid,
                                                 bool print_header) {
  if (!EnsureProcessesMap())
    return;

  ProcsTreeBuilder tree_builder(&map_processes_);

  tree_builder.MapBuilder();
  tree_builder.MapHandshake();
  tree_builder.BuildTree();

  // Root sentinels (0 on Windows, 1 on Linux) are never in map_processes_
  // they resolve to the fake root inside PrintTree.  Any other PID must exist.
#ifdef _WIN32
  constexpr int root_pid = 0;
#else
  constexpr int root_pid = 1;
#endif

  if (proc_pid == root_pid || proc_pid == kFakeRootPID ||
      map_processes_.find(proc_pid) != map_processes_.end()) {
    tree_builder.PrintTree(proc_pid, print_header);
  } else {
    ucout << "Invalid Process ID | PID " << proc_pid
          << " not detected in memory" << std::endl;
  }
}

bool ProcessingOperations::ShowNotification(const std::string& message,
                                            const std::string& title) {
  return psa::system::show_notification(title.empty() ? "psa" : title,
                                        message);
}

void ProcessingOperations::KillProcesses(TCHAR const* argvProcessParam) {
  if (!EnsureProcessesMap())
    return;

  if (string_utils::is_number(argvProcessParam)) {
    int pid = utoi(argvProcessParam);
    process_actions::kill_process_by_pid_optimized(pid, map_processes_);
  } else {
    process_actions::kill_process_by_name_optimized(argvProcessParam,
                                                    map_processes_);
  }
}

void ProcessingOperations::KillProcessesWithCmdLine(
    const ustring& argv_process_param,
    const ustring& cmdline_filter) {
  if (argv_process_param.empty()) {
    ucout << _T("Error: Process name or PID cannot be empty.") << std::endl;
    return;
  }
  if (cmdline_filter.empty()) {
    ucout << _T("Error: Command line filter cannot be empty.") << std::endl;
    return;
  }

  if (!EnsureProcessesMap())
    return;

  std::vector<proc_info> candidates;
  const bool filter_is_pid = string_utils::is_number(argv_process_param);

  if (filter_is_pid) {
    DWORD pid = static_cast<DWORD>(utoi(argv_process_param.c_str()));
    auto range = map_processes_.equal_range(pid);
    for (auto it = range.first; it != range.second; ++it) {
      candidates.push_back(it->second);
    }
  } else {
    for (const auto& [pid, proc] : map_processes_) {
      if (string_utils::search_substring(proc.proc_name, argv_process_param)) {
        candidates.push_back(proc);
      }
    }
  }

  std::vector<int> pids_to_kill;
  for (const auto& proc : candidates) {
    if (string_utils::search_substring(proc.cmdline_args, cmdline_filter)) {
      pids_to_kill.push_back(proc.proc_pid);
    }
  }

  if (pids_to_kill.empty()) {
    ucout << _T("No processes matching '") << argv_process_param
          << _T("' with command line containing '") << cmdline_filter
          << _T("' were found.") << std::endl;
    return;
  }

  for (int pid : pids_to_kill) {
    process_actions::kill_process_by_pid_optimized(pid, map_processes_);
  }
}

#ifdef _WIN32
SIZE_T ProcessingOperations::GetProcessUsedMemory(DWORD const processID) const {
  smart_handle hProcess = OpenProcess(
      PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
  if (NULL == hProcess)
    return 0;

  PROCESS_MEMORY_COUNTERS_EX pmc;
  ::ZeroMemory(&pmc, sizeof(PROCESS_MEMORY_COUNTERS_EX));

  K32GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc,
                          sizeof(pmc));

  return pmc.PrivateUsage;
}

BOOL ProcessingOperations::SetPrivilege(HANDLE hToken,
                                        LPCTSTR lpszPrivilege,
                                        BOOL bEnablePrivilege) {
  TOKEN_PRIVILEGES tp;
  LUID luid;

  if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) {
    std::cerr << psa::format("LookupPrivilegeValue error: {}\n",
                             GetLastError());
    return FALSE;
  }

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;

  if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
                             (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) {
    std::cerr << psa::format("AdjustTokenPrivileges error: {}\n",
                             GetLastError());
    return FALSE;
  }

  if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
    std::cerr << "The token does not have the specified privilege.\n";
    return FALSE;
  }

  CloseHandle(hToken);

  return TRUE;
}

void ProcessingOperations::PrintError(const TCHAR* msg) {
  DWORD num;
  TCHAR sys_msg[256];
  TCHAR* p;

  num = GetLastError();
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, num, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), sys_msg,
                256, NULL);

  // Trim trailing punctuation/newline from the system message.
  p = sys_msg;
  while ((*p > 31) || (*p == 9))
    ++p;
  do {
    *p-- = 0;
  } while ((p >= sys_msg) && ((*p == '.') || (*p < 33)));

  ucout << psa::format(_T("\n  WARNING: {} failed with error {} ({})"), msg,
                       num, sys_msg);
}
#endif
