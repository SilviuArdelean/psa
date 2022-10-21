#include "operations.h"

#include <algorithm>
#include <iostream>
#include <mutex>
#include <vector>

#include "fixed_queue.h"
#include "general.h"
#include "process_operations.h"
#include "processes_tree_builder.h"
#include "string_utils.h"

#ifdef _WIN32
#include "Winternl.h"
#include "smart_handler.h"
#elif __linux__
#include <proc/readproc.h>
#endif

ProcessingOperations::ProcessingOperations(void) {}

bool ProcessingOperations::BuildProcessesMap() {
  std::mutex g_i_mutex;
  std::lock_guard<std::mutex> lock(g_i_mutex);

#ifdef _WIN32

  PROCESSENTRY32 pe32;

  // Take a snapshot of all processes in the system.
  smart_handle hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hProcessSnap == INVALID_HANDLE_VALUE) {
    printError(TEXT("CreateToolhelp32Snapshot (of processes)"));
    return FALSE;
  }

  // Set the size of the structure before using it.
  pe32.dwSize = sizeof(PROCESSENTRY32);

  // Retrieve information about the first process,
  // and exit if unsuccessful
  if (!Process32First(hProcessSnap, &pe32)) {
    printError(TEXT(
        "Retrieve information about the first process has failed"));  // show
                                                                      // cause
                                                                      // of
                                                                      // failure
    return FALSE;
  }

  map_processes_.clear();

  do {
    proc_info pi(pe32.th32ProcessID, pe32.th32ParentProcessID, pe32.szExeFile,
                 getProcessUsedMemory(pe32.th32ProcessID));

    map_processes_.insert(std::pair<DWORD, proc_info>(pe32.th32ProcessID, pi));

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
    proc_info pi(_process.tid, _process.ppid,
                 (_process.cmdline != NULL) ? *_process.cmdline : _process.cmd,
                 _process.vsize);  // !!! ??? make sure is the right attribute

    map_processes_.insert(std::pair<DWORD, proc_info>(_process.tid, pi));
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

void ProcessingOperations::printTopExpensiveProcesses(const int top) {
  if (map_processes_.empty()) {
    BuildProcessesMap();
  }

  if (map_processes_.empty())
    ucout << "Processes list is empty" << std::endl;

  struct data4sort {
    int pid;
    ustring proc_name;
    ULONG64 mem_usage;

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

  fixed_queue<data4sort, std::vector<data4sort>,
                  LessThanByFileSize> top_queue(top);

  for (auto& ob : map_processes_) {
    data4sort data;
    data.pid = ob.second.procPID;
    data.proc_name = ob.second.procName;
    data.mem_usage = ob.second.usedMemory;

    top_queue.push(data);
  }

  ULONG64 processesAllSize = 0;

  ucout << " Top " << top << " most consuming memory processes \n";
  ShowHeader();

  std::sort(top_queue.begin(), top_queue.end(),
            [](const auto& l, const auto& r) {
                 return l.mem_usage > r.mem_usage;
            });

  while (!top_queue.empty()) {
    auto ob = top_queue.top();

    uprintf_s(_T(" [%d] \t %.2lf MB \t %-15s \n"), ob.pid,
              (double)ob.mem_usage / MB_DIVIDER,
              ob.proc_name.c_str());

    processesAllSize += ob.mem_usage;

    top_queue.pop();
  }

  ucout << "-------------------------------------------" << std::endl;
  ucout << "   Total used memory: " << (double)processesAllSize / MB_DIVIDER
        << " MB" << std::endl;
}

bool ProcessingOperations::get_filter_results(const ustring& process_name,
                                              const ustring& filter) {
  return ((filter.empty() || std::string::npos != filter.find(_T("*"))
               ? true
               : string_utils::search_substring(filter, process_name)));
}

bool ProcessingOperations::printAllProcessesInformation(
    bool const show_details) {
  int processesCount = 0;
  ULONG64 processesAllSize = 0;

  if (map_processes_.empty())
    BuildProcessesMap();

  ShowHeader();

  for (auto& proc_obj : map_processes_) {
    ustring current_process(proc_obj.second.procName);

    auto procPID = proc_obj.second.procPID;

    {
      uprintf_s(_T("PID [%d] \t %.4lf MB \t %-15s \n"), procPID,
                (double)proc_obj.second.usedMemory / MB_DIVIDER,
                proc_obj.second.procName.c_str());

      if (show_details) {
        printProcessDetailedInfo(procPID);
      }

      processesAllSize += proc_obj.second.usedMemory;
      processesCount++;
    }
  }

  if (processesCount != 0) {
    ucout << "--------------------------------------------------------"
          << std::endl;
    ucout << "   Total used memory: " << (double)processesAllSize / MB_DIVIDER
          << " MB "
          << "  [ " << map_processes_.size() - 1  // skip PID 0
          << " processes ]" << std::endl;
  }

  return true;
}

bool ProcessingOperations::printProcessInformation(const ustring& filter,
                                                   bool const show_details) {
  int processesCount = 0;
  ULONG64 processesAllSize = 0;

  if (map_processes_.empty())
    BuildProcessesMap();

  ShowHeader();

  // https://msdn.microsoft.com/en-us/library/windows/desktop/ms682050(v=vs.85).aspx
  for (auto& proc_obj : map_processes_) {
    ustring current_process(proc_obj.second.procName);

    if (string_utils::search_substring(current_process, filter)) {
      auto proc_pid = proc_obj.second.procPID;
      auto process_path = process_operations::GetProcessPath(proc_pid);

      uprintf_s(_T("[PID: %d] \t %.4lf MB \t %-15s \n"), proc_pid,
                (double)proc_obj.second.usedMemory / MB_DIVIDER,
                proc_obj.second.procName.c_str());

      // if (!process_path.empty()) {
      //   uprintf_s(_T("   [ %s ]\n"), process_path.c_str());
      // }

      if (show_details) {
        printProcessDetailedInfo(proc_pid);
      }

      processesAllSize += proc_obj.second.usedMemory;
      processesCount++;
    }
  }

  if (processesCount != 0) {
    if (0 != processesAllSize) {
      ucout << "-----------------------------------" << std::endl;
      ucout << "   Total used memory: " << (double)processesAllSize / MB_DIVIDER
            << " MB  [ " << processesCount << " processes ]" << std::endl;
    } else {
#ifdef _WIN32
      ucout << "Seems psa.exe application runs under not enough privileges. "
               "Please launch it with administrator privileges."
            << std::endl;
#else
      ucout << "Seems psa application runs under not enough privileges. Please "
               "run it by root privileges."
            << std::endl;
#endif
    }
  } else {
    ucout << "Undetected process with \'" << filter << "' name." << std::endl;
  }

  return true;
}

bool ProcessingOperations::printProcessDetailedInfo(DWORD pid) {
  // TO DO
  // command line
  // started from
  // PEB address
  // parent information

  return true;
}

void ProcessingOperations::generateProcessesTree(int const proc_pid) {
  if (map_processes_.empty())
    BuildProcessesMap();

  ProcsTreeBuilder tree_builder(&map_processes_);

  tree_builder.mapBuilder();
  tree_builder.mapHandshake();
  tree_builder.buildTree();

  auto it = map_processes_.find(proc_pid);
  if (it != map_processes_.end()) {
    tree_builder.printTree(proc_pid);
  } else {
    ucout << "Invalid Process ID | PID " << proc_pid
          << " not detected in memory" << std::endl;
  }
}

void ProcessingOperations::killProcesses(TCHAR const* argvProcessParam) {
  if (map_processes_.empty())
    BuildProcessesMap();

  if (string_utils::is_number(argvProcessParam)) {
    process_operations::kill_process_by_pid_optimized(utoi(argvProcessParam),
                                                      map_processes_);
  } else {
    process_operations::kill_process_by_name_optimized(argvProcessParam,
                                                       map_processes_);
  }
}

#ifdef _WIN32
SIZE_T ProcessingOperations::getProcessUsedMemory(DWORD const processID) const {
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
    printf("LookupPrivilegeValue error: %u\n", GetLastError());
    return FALSE;
  }

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;

  // Enable the privilege or disable all privileges.

  if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
                             (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) {
    printf("AdjustTokenPrivileges error: %u\n", GetLastError());
    return FALSE;
  }

  if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
    printf("The token does not have the specified privilege. \n");
    return FALSE;
  }

  CloseHandle(hToken);

  return TRUE;
}

void ProcessingOperations::printError(TCHAR* msg) {
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
  uprintf(TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg);
}
#endif
