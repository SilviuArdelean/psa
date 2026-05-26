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

#include "fixed_queue.h"
#include "process_operations.h"
#include "processes_tree_builder.h"
#include "string_utils.h"

#ifdef _WIN32
#include "smart_handler.h"
#endif

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

  // Take a snapshot of all processes in the system.
  smart_handle hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hProcessSnap == INVALID_HANDLE_VALUE) {
    PrintError(TEXT("CreateToolhelp32Snapshot (of processes)"));
    return FALSE;
  }

  // Set the size of the structure before using it.
  pe32.dwSize = sizeof(PROCESSENTRY32);

  // Retrieve information about the first process,
  // and exit if unsuccessful
  if (!Process32First(hProcessSnap, &pe32)) {
    PrintError(TEXT(
        "Retrieve information about the first process has failed"));  // show
                                                                      // cause
                                                                      // of
                                                                      // failure
    return FALSE;
  }

  map_processes_.clear();

  do {
    map_processes_.emplace(
        pe32.th32ProcessID,
        proc_info(pe32.th32ProcessID, pe32.th32ParentProcessID, pe32.szExeFile,
                  GetProcessUsedMemory(pe32.th32ProcessID)));

  } while (Process32Next(hProcessSnap, &pe32));

#else

  PROCTAB* proc = openproc(PROC_FILLARG       // fillarg used for cmdline
                           | PROC_FILLSTAT);  // fillstat used for cmd

  static proc_t _process;  // needs static because of readproc() throws
                           // segmantation fault unleast on Ubuntu 16
                           // https://gitlab.com/procps-ng/procps/issues/33

  // zero out the allocated proc_info memory
  memset(&_process, 0, sizeof(_process));
  map_processes_.clear();

  while (readproc(proc, &_process) != NULL) {
    map_processes_.emplace(
        _process.tid,
        proc_info(_process.tid, _process.ppid,
                  (_process.cmdline != NULL) ? *_process.cmdline : _process.cmd,
                  _process.vsize));
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
    top_queue.push({.pid = proc.procPID,
                    .proc_name = proc.procName,
                    .mem_usage = static_cast<ULONG64>(proc.usedMemory)});
  }

  ULONG64 processesAllSize = 0;

  ucout << _T(" Top ") << top << _T(" most consuming memory processes \n");
  ShowHeader();

  // Copy out to a vector and sort - avoids breaking the heap invariant in
  // fixed_queue by never calling pop() on a sorted container.
  std::vector<data4sort> sorted(top_queue.begin(), top_queue.end());
  std::sort(sorted.begin(), sorted.end(), [](const auto& l, const auto& r) {
    return l.mem_usage > r.mem_usage;
  });

  for (const auto& ob : sorted) {
    uprintf_s(_T(" [%d] \t %.2lf MB \t %-15s \n"), ob.pid, ToMb(ob.mem_usage),
              ob.proc_name.c_str());

    processesAllSize += ob.mem_usage;
  }

  ucout << "-------------------------------------------" << std::endl;
  ucout << "   Total used memory: " << ToMb(processesAllSize) << " MB"
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
  int processesCount = 0;
  ULONG64 processesAllSize = 0;

  if (!EnsureProcessesMap())
    return false;

  ShowHeader();

  for (const auto& proc : map_processes_ | std::views::values) {
    auto procPID = proc.procPID;

#ifdef _WIN32
    uprintf_s(_T("PID [%d] \t %.4lf MB \t %-15s \n"), procPID,
              ToMb(proc.usedMemory), proc.procName.c_str());
#else
    uprintf_s("PID [%d] \t %.4lf MB \t %-15s \n", procPID,
              ToMb(proc.usedMemory), proc.procName.c_str());
#endif

    if (show_details)
      static_cast<void>(PrintProcessDetailedInfo(procPID));

    processesAllSize += proc.usedMemory;
    processesCount++;
  }

  if (processesCount != 0) {
    ucout << _T("--------------------------------------------------------")
          << std::endl;
    uprintf_s(_T("   Total used memory: %.4lf MB  [ %d processes ]\n"),
              ToMb(processesAllSize), processesCount);
  }

  return true;
}

bool ProcessingOperations::PrintProcessInformation(const ustring& filter,
                                                   bool const show_details) {
  int processesCount = 0;
  ULONG64 processesAllSize = 0;

  if (!EnsureProcessesMap())
    return false;

  ShowHeader();

  // https://msdn.microsoft.com/en-us/library/windows/desktop/ms682050(v=vs.85).aspx
  const bool filter_is_pid = string_utils::is_number(filter);
  auto matching = map_processes_ | std::views::values |
                  std::views::filter([&](const proc_info& p) {
                    if (filter_is_pid)
                      return p.procPID == utoi(filter.c_str());
                    return string_utils::search_substring(p.procName, filter);
                  });

  for (const auto& proc : matching) {
    auto proc_pid = proc.procPID;

#ifdef _WIN32
    uprintf_s(_T("[PID: %d] \t %.4lf MB \t %-15s \n"), proc_pid,
              ToMb(proc.usedMemory), proc.procName.c_str());
#else
    uprintf_s("[PID: %d] \t %.4lf MB \t %-15s \n", proc_pid,
              ToMb(proc.usedMemory), proc.procName.c_str());
#endif

    if (show_details) {
      const auto cmdline = process_operations::GetProcessCmdLine(proc_pid);
      if (!cmdline.empty()) {
        ucout << _T("   cmdl: [ ") << cmdline.c_str() << _T(" ]") << std::endl;
      } else {
        const auto path = process_operations::GetProcessPath(proc_pid);
        if (!path.empty())
          ucout << _T("   path: [ ") << path.c_str() << _T(" ]") << std::endl;
      }
    }

    processesAllSize += proc.usedMemory;
    processesCount++;
  }

  if (processesCount != 0) {
    if (0 != processesAllSize) {
      ucout << _T("-----------------------------------") << std::endl;
      uprintf_s(_T("   Total used memory: %.4lf MB  [ %d processes ]\n"),
                ToMb(processesAllSize), processesCount);
    } else {
#ifdef _WIN32
      ucout
          << _T("Seems psa.exe application runs under not enough privileges. ")
             _T("Please launch it with administrator privileges.")
          << std::endl;
#else
      ucout << _T("Seems psa application runs under not enough privileges. ")
               _T("Please ")
               _T("run it by root privileges.")
            << std::endl;
#endif
    }
  } else {
    ucout << _T("Undetected process with '") << filter << _T("' name.")
          << std::endl;
  }

  return true;
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

  if (proc_pid == root_pid || proc_pid == FAKE_ROOT_PID ||
      map_processes_.find(proc_pid) != map_processes_.end()) {
    tree_builder.PrintTree(proc_pid, print_header);
  } else {
    ucout << "Invalid Process ID | PID " << proc_pid
          << " not detected in memory" << std::endl;
  }
}

void ProcessingOperations::KillProcesses(TCHAR const* argvProcessParam) {
  if (!EnsureProcessesMap())
    return;

  if (string_utils::is_number(argvProcessParam)) {
    int pid = utoi(argvProcessParam);
    process_operations::kill_process_by_pid_optimized(pid, map_processes_);
  } else {
    process_operations::kill_process_by_name_optimized(argvProcessParam,
                                                       map_processes_);
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

  if (K32GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc,
                              sizeof(pmc))) {
    /*
    printf("\tPageFaultCount: 0x%08X\n", pmc.PageFaultCount);
    printf("\tPeakWorkingSetSize: 0x%08X\n",
    pmc.PeakWorkingSetSize);
    printf("\tWorkingSetSize: 0x%08X\n", pmc.WorkingSetSize);
    printf("\tQuotaPeakPagedPoolUsage: 0x%08X\n",
    pmc.QuotaPeakPagedPoolUsage);
    printf("\tQuotaPagedPoolUsage: 0x%08X\n",
    pmc.QuotaPagedPoolUsage);
    printf("\tQuotaPeakNonPagedPoolUsage: 0x%08X\n",
    pmc.QuotaPeakNonPagedPoolUsage);
    printf("\tQuotaNonPagedPoolUsage: 0x%08X\n",
    pmc.QuotaNonPagedPoolUsage);
    printf("\tPagefileUsage: 0x%08X\n", pmc.PagefileUsage);
    printf("\tPeakPagefileUsage: 0x%08X\n",
    pmc.PeakPagefileUsage)
    */
  }

  return pmc.PrivateUsage;
}

BOOL ProcessingOperations::SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
) {
  TOKEN_PRIVILEGES tp;
  LUID luid;

  if (!LookupPrivilegeValue(NULL,           // lookup privilege on local system
                            lpszPrivilege,  // privilege to lookup
                            &luid))         // receives LUID of privilege
  {
    std::cerr << std::format("LookupPrivilegeValue error: {}\n",
                             GetLastError());
    return FALSE;
  }

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;

  // Enable the privilege or disable all privileges.

  if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
                             (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) {
    std::cerr << std::format("AdjustTokenPrivileges error: {}\n",
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
  DWORD eNum;
  TCHAR sysMsg[256];
  TCHAR* p;

  eNum = GetLastError();
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, eNum,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  // Default language
                sysMsg, 256, NULL);

  // Trim the end of the line and terminate it with a null
  p = sysMsg;
  while ((*p > 31) || (*p == 9))
    ++p;
  do {
    *p-- = 0;
  } while ((p >= sysMsg) && ((*p == '.') || (*p < 33)));

  // Display the message
  ucout << std::format(_T("\n  WARNING: {} failed with error {} ({})"), msg,
                       eNum, sysMsg);
}
#endif
