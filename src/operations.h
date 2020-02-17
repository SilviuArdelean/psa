
#pragma once
#include <map>
#include <stack>
#include <vector>
#include "general.h"

class ProcessingOperations {
 public:
  ProcessingOperations(void);
  ~ProcessingOperations(){};

  bool BuildProcessesMap();

  std::multimap<DWORD, proc_info>* GetProcessesMap() { return &map_processes_; }

  bool printAllProcessesInformation(bool const show_details = false);
  bool printProcessInformation(const ustring& process_name,
                               bool const show_details = false);
  void printTopExpensiveProcesses(const int top);
  void killProcesses(TCHAR const* argvProcessParam);
  void generateProcessesTree(int const proc_pid);

 protected:
  bool printProcessDetailedInfo(DWORD pid);
  bool get_filter_results(const ustring& process_name,
                          const ustring& current_process);

#ifdef _WIN32
  void printError(TCHAR* msg);
  SIZE_T getProcessUsedMemory(DWORD const processID) const;
  BOOL SetPrivilege(
      HANDLE hToken,           // access token handle
      LPCTSTR lpszPrivilege,   // name of privilege to enable/disable
      BOOL bEnablePrivilege);  // to enable or disable privilege
#endif

 protected:
  procs_map map_processes_;
};
