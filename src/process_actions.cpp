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

#include "process_actions.h"

namespace process_actions {

namespace {

void print_termination_failure(const proc_info& process) {
  ucout << _T("Process '") << process.proc_name << _T("' PID [")
        << process.proc_pid << _T("] cannot be terminated.") << std::endl;
}

void print_termination_success(const proc_info& process) {
  ucout << _T("Process '") << process.proc_name << _T("' PID [")
        << process.proc_pid << _T("] was terminated.") << std::endl;
}

bool try_terminate_process(const proc_info& process) {
#ifdef _WIN32
  HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, (DWORD)process.proc_pid);
  if (NULL == hProcess) {
    return false;
  }

  const auto result = TerminateProcess(hProcess, 9);
  CloseHandle(hProcess);
  return result != 0;
#else
  return 0 == kill(process.proc_pid, SIGKILL);
#endif
}

bool terminate_process_and_report(const proc_info& process) {
  if (!try_terminate_process(process)) {
    print_termination_failure(process);
    return false;
  }

  print_termination_success(process);
  return true;
}

bool is_pid_lookup(const TCHAR* process_name) {
  return string_utils::compare_case_sensitive(process_name, process_fake_name);
}

void print_missing_pid_message(int process_pid) {
  ucout << _T("No process PID ") << process_pid << _T(" was found.")
        << std::endl;
}

void print_missing_name_message(const TCHAR* process_name) {
  ucout << _T("No '") << process_name
        << _T("' name or including this name pattern was found.")
        << std::endl;
}

void kill_process_by_pid_optimized_impl(int process_pid,
                                        const procs_map& map_processes) {
  if (is_essential_proccess(process_pid)) {
    proc_info dummy(process_pid, 0, process_fake_name, 0, _T(""));
    print_termination_failure(dummy);
    return;
  }
  const auto it_process = map_processes.find(process_pid);
  if (it_process == map_processes.end()) {
    print_missing_pid_message(process_pid);
    return;
  }

  terminate_process_and_report(it_process->second);
}

void kill_processes_by_name_optimized_impl(const TCHAR* process_name,
                                           const procs_map& map_processes) {
  bool process_found = false;
  for (const auto& process_entry : map_processes) {
    const auto& process = process_entry.second;
    if (!string_utils::search_substring(process.proc_name, process_name)) {
      continue;
    }

    process_found = true;
    terminate_process_and_report(process);
  }

  if (!process_found) {
    print_missing_name_message(process_name);
  }
}

}  // namespace

void kill_process_by_name(const TCHAR* process_name) {
  execute_kill_process(0, process_name);
}

void kill_process_by_pid(const int process_pid) {
  execute_kill_process(process_pid, process_fake_name);
}

void kill_process_by_name_optimized(const TCHAR* process_name,
                                    const procs_map& map_processes) {
  execute_kill_process_optimized(0, process_name, map_processes);
}

void kill_process_by_pid_optimized(const int process_pid,
                                   const procs_map& map_processes) {
  execute_kill_process_optimized(process_pid, process_fake_name, map_processes);
}

ustring GetProcessPath(int process_pid) {
  ustring process_path;
#ifdef _WIN32
  smart_handle hnd(
      OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                  FALSE, process_pid));
  if (hnd.get_handle()) {
    DWORD dwSize = MAX_PATH;
    WCHAR szFileName[MAX_PATH] = {0};
    QueryFullProcessImageName(hnd.get_handle(), 0, szFileName, &dwSize);
    if (dwSize != 0) {
      process_path = szFileName;
    }
  }
#else
  std::array<char, PATH_MAX> path_buff{};
  const auto len = get_exe_for_pid(process_pid, path_buff);
  if (len > 0) {
    path_buff[len] = '\0';
    process_path = path_buff.data();
  }
#endif
  return process_path;
}

ustring GetProcessCmdLine(int process_pid) {
  ustring cmdline;
#ifdef _WIN32
  smart_handle hnd(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                               FALSE, process_pid));
  if (!hnd.get_handle())
    return cmdline;
  using NtQueryInformationProcess_t =
      NTSTATUS(WINAPI*)(HANDLE, UINT, PVOID, ULONG, PULONG);
  static const auto NtQIP =
      reinterpret_cast<NtQueryInformationProcess_t>(GetProcAddress(
          GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess"));
  if (!NtQIP)
    return cmdline;
  ULONG size = 0;
  NtQIP(hnd.get_handle(), 60, nullptr, 0, &size);
  if (size == 0)
    return cmdline;
  std::vector<BYTE> buf(size);
  const NTSTATUS status = NtQIP(hnd.get_handle(), 60, buf.data(), size, &size);
  if (status < 0)
    return cmdline;
  const auto* us = reinterpret_cast<const UNICODE_STRING*>(buf.data());
  if (us->Buffer && us->Length > 0)
    cmdline = ustring(us->Buffer, us->Length / sizeof(WCHAR));
#else
  const auto path = std::format("/proc/{}/cmdline", process_pid);
  std::ifstream f(path, std::ios::binary);
  if (!f)
    return cmdline;
  std::string raw(std::istreambuf_iterator<char>(f), {});
  for (auto& c : raw)
    if (c == '\0')
      c = ' ';
  while (!raw.empty() && raw.back() == ' ')
    raw.pop_back();
  cmdline = raw;
#endif
  return cmdline;
}

bool is_essential_proccess(const int process_pid) {
  return (process_pid == 0 || process_pid == 1);
}

void execute_kill_process(const int process_pid, const TCHAR* process_name) {
#ifdef _WIN32
  PROCESSENTRY32 pEntry;
  pEntry.dwSize = sizeof(pEntry);
  HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
  BOOL hRes = Process32First(hSnapShot, &pEntry);
  while (hRes) {
    if ((!is_essential_proccess(process_pid) &&
         pEntry.th32ProcessID == process_pid) ||
        string_utils::search_substring(pEntry.szExeFile, process_name)) {
      HANDLE hProcess =
          OpenProcess(PROCESS_TERMINATE, 0, (DWORD)pEntry.th32ProcessID);
      if (NULL != hProcess) {
        TerminateProcess(hProcess, 9);
        CloseHandle(hProcess);
      }
    }
    hRes = Process32Next(hSnapShot, &pEntry);
  }
  CloseHandle(hSnapShot);
#else
  PROCTAB* proc = openproc(PROC_FILLARG | PROC_FILLSTAT);
  static proc_t _process;
  memset(&_process, 0, sizeof(_process));
  while (readproc(proc, &_process) != NULL) {
    ustring strProcessCmd =
        (_process.cmdline != NULL) ? *_process.cmdline : _process.cmd;
    if ((!is_essential_proccess(process_pid) && _process.tid == process_pid) ||
        string_utils::search_substring(strProcessCmd, process_name)) {
      kill(_process.tid, SIGKILL);
      if (_process.tid == process_pid)
        break;
    }
  }
  closeproc(proc);
#endif
}

void execute_kill_process_optimized(const int process_pid,
                                    const TCHAR* process_name,
                                    const procs_map& map_processes) {
  if (is_pid_lookup(process_name)) {
    kill_process_by_pid_optimized_impl(process_pid, map_processes);
    return;
  }

  kill_processes_by_name_optimized_impl(process_name, map_processes);
}

#ifdef __linux__
ssize_t get_exe_for_pid(int pid, std::span<char> buf) {
  const auto path = std::format("/proc/{}/exe", pid);
  return readlink(path.c_str(), buf.data(), buf.size() - 1);
}
#endif

}  // namespace process_actions
