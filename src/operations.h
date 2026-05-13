
#pragma once
#include <map>
#include <mutex>
#include <stack>
#include <vector>
#include "general.h"

class ProcessingOperations {
 public:
  ProcessingOperations(void);
  ~ProcessingOperations() {};

  [[nodiscard]] bool BuildProcessesMap();

  std::multimap<DWORD, proc_info>* GetProcessesMap() { return &map_processes_; }

  [[nodiscard]] bool PrintAllProcessesInformation(
      bool const show_details = false);
  [[nodiscard]] bool PrintProcessInformation(const ustring& process_name,
                                             bool const show_details = false);
  void PrintTopExpensiveProcesses(const int top);
  void KillProcesses(TCHAR const* argvProcessParam);
  void GenerateProcessesTree(int const proc_pid);

 protected:
  [[nodiscard]] bool PrintProcessDetailedInfo(DWORD pid);
  bool get_filter_results(const ustring& process_name,
                          const ustring& current_process);

#ifdef _WIN32
  void PrintError(const TCHAR* msg);
  SIZE_T GetProcessUsedMemory(DWORD const processID) const;
  BOOL SetPrivilege(
      HANDLE hToken,           // access token handle
      LPCTSTR lpszPrivilege,   // name of privilege to enable/disable
      BOOL bEnablePrivilege);  // to enable or disable privilege
#endif

 private:
  [[nodiscard]] bool EnsureProcessesMap();

 protected:
  procs_map map_processes_;
  std::mutex map_mutex_;
};
