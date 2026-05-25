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
#include <map>
#include <mutex>
#include "general.h"

class ProcessingOperations {
 public:
  ProcessingOperations(void);
  virtual ~ProcessingOperations() {};

  [[nodiscard]] bool BuildProcessesMap();

  std::multimap<DWORD, proc_info>* GetProcessesMap() { return &map_processes_; }

  [[nodiscard]] virtual bool PrintAllProcessesInformation(
      bool const show_details = false);
  [[nodiscard]] virtual bool PrintProcessInformation(const ustring& process_name,
                                             bool const show_details = false);
  virtual void PrintTopExpensiveProcesses(const int top);
  virtual void KillProcesses(TCHAR const* argvProcessParam);
  virtual void GenerateProcessesTree(int const proc_pid);

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
