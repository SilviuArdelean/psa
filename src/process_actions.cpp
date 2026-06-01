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

#ifdef _WIN32
using process_name_map = std::map<DWORD, ustring>;

process_name_map build_process_name_map(HANDLE snapshot_handle) {
  process_name_map names;

  PROCESSENTRY32 entry{};
  entry.dwSize = sizeof(entry);
  if (!Process32First(snapshot_handle, &entry)) {
    return names;
  }

  do {
    names.emplace(entry.th32ProcessID, entry.szExeFile);
  } while (Process32Next(snapshot_handle, &entry));

  return names;
}

std::optional<proc_info_details> find_process_in_snapshot(
    HANDLE snapshot_handle,
    DWORD pid) {
  PROCESSENTRY32 entry{};
  entry.dwSize = sizeof(entry);
  if (!Process32First(snapshot_handle, &entry)) {
    return std::nullopt;
  }

  do {
    if (entry.th32ProcessID == pid) {
      proc_info_details details;
      details.proc_pid = static_cast<int>(entry.th32ProcessID);
      details.parent_pid = static_cast<int>(entry.th32ParentProcessID);
      details.proc_name = entry.szExeFile;
      return details;
    }
  } while (Process32Next(snapshot_handle, &entry));

  return std::nullopt;
}

ustring format_system_time(const FILETIME& file_time) {
  FILETIME local_file_time{};
  SYSTEMTIME system_time{};

  if (!FileTimeToLocalFileTime(&file_time, &local_file_time) ||
      !FileTimeToSystemTime(&local_file_time, &system_time)) {
    return _T("<unavailable>");
  }

  return std::format(_T("{:04}-{:02}-{:02} {:02}:{:02}:{:02}"),
                     system_time.wYear, system_time.wMonth, system_time.wDay,
                     system_time.wHour, system_time.wMinute,
                     system_time.wSecond);
}

ustring query_process_architecture(HANDLE process_handle) {
  SYSTEM_INFO system_info{};
  GetNativeSystemInfo(&system_info);

  BOOL is_wow64 = FALSE;
  if (!IsWow64Process(process_handle, &is_wow64)) {
    return _T("<unavailable>");
  }

  if (is_wow64) {
    return _T("x86");
  }

  switch (system_info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
      return _T("x64");
    case PROCESSOR_ARCHITECTURE_INTEL:
      return _T("x86");
    case PROCESSOR_ARCHITECTURE_ARM64:
      return _T("ARM64");
    case PROCESSOR_ARCHITECTURE_ARM:
      return _T("ARM");
    default:
      return _T("<unavailable>");
  }
}

std::optional<ustring> query_token_user(HANDLE token_handle) {
  DWORD size = 0;
  GetTokenInformation(token_handle, TokenUser, nullptr, 0, &size);
  if (size == 0) {
    return std::nullopt;
  }

  std::vector<BYTE> buffer(size);
  if (!GetTokenInformation(token_handle, TokenUser, buffer.data(), size,
                           &size)) {
    return std::nullopt;
  }

  const auto* token_user = reinterpret_cast<const TOKEN_USER*>(buffer.data());

  DWORD name_size = 0;
  DWORD domain_size = 0;
  SID_NAME_USE sid_type = SidTypeUnknown;
  LookupAccountSidW(nullptr, token_user->User.Sid, nullptr, &name_size, nullptr,
                    &domain_size, &sid_type);
  if (name_size == 0) {
    return std::nullopt;
  }

  std::wstring account_name(name_size, L'\0');
  std::wstring domain_name(domain_size, L'\0');
  if (!LookupAccountSidW(nullptr, token_user->User.Sid, account_name.data(),
                         &name_size, domain_name.data(), &domain_size,
                         &sid_type)) {
    return std::nullopt;
  }

  account_name.resize(name_size);
  domain_name.resize(domain_size);

  if (!domain_name.empty()) {
    return domain_name + _T("\\") + account_name;
  }

  return account_name;
}

std::optional<bool> query_token_elevation(HANDLE token_handle) {
  TOKEN_ELEVATION elevation{};
  DWORD size = 0;
  if (!GetTokenInformation(token_handle, TokenElevation, &elevation,
                           sizeof(elevation), &size)) {
    return std::nullopt;
  }

  return elevation.TokenIsElevated != 0;
}

std::optional<ustring> query_token_integrity_level(HANDLE token_handle) {
  DWORD size = 0;
  GetTokenInformation(token_handle, TokenIntegrityLevel, nullptr, 0, &size);
  if (size == 0) {
    return std::nullopt;
  }

  std::vector<BYTE> buffer(size);
  if (!GetTokenInformation(token_handle, TokenIntegrityLevel, buffer.data(),
                           size, &size)) {
    return std::nullopt;
  }

  const auto* mandatory_label =
      reinterpret_cast<const TOKEN_MANDATORY_LABEL*>(buffer.data());
  const DWORD rid = *GetSidSubAuthority(
      mandatory_label->Label.Sid,
      static_cast<DWORD>(*GetSidSubAuthorityCount(mandatory_label->Label.Sid) -
                         1));

  switch (rid) {
    case SECURITY_MANDATORY_UNTRUSTED_RID:
      return _T("Untrusted");
    case SECURITY_MANDATORY_LOW_RID:
      return _T("Low");
    case SECURITY_MANDATORY_MEDIUM_RID:
      return _T("Medium");
    case SECURITY_MANDATORY_HIGH_RID:
      return _T("High");
    case SECURITY_MANDATORY_SYSTEM_RID:
      return _T("System");
    default:
      return _T("<unavailable>");
  }
}

void set_access_denied_fields(proc_info_details& details) {
  details.executable_path = _T("<access denied>");
  details.cmdline_args = _T("<access denied>");
  details.architecture = _T("<access denied>");
  details.start_time = _T("<access denied>");
  details.integrity_level = _T("<access denied>");
  details.user = _T("<access denied>");
  details.session_id = std::nullopt;
  details.elevated = std::nullopt;
}

std::optional<proc_info_details> get_process_details_info_win32(
    const uint32_t pid) {
  smart_handle snapshot_handle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
  if (!snapshot_handle.get_handle() ||
      snapshot_handle.get_handle() == INVALID_HANDLE_VALUE) {
    return std::nullopt;
  }

  const auto names = build_process_name_map(snapshot_handle.get_handle());
  auto details = find_process_in_snapshot(snapshot_handle.get_handle(),
                                          static_cast<DWORD>(pid));
  if (!details.has_value()) {
    return std::nullopt;
  }

  const auto parent_it = names.find(static_cast<DWORD>(details->parent_pid));
  if (parent_it != names.end()) {
    details->parent_name = parent_it->second;
  }

  smart_handle process_handle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                          FALSE, static_cast<DWORD>(pid)));
  if (!process_handle.get_handle()) {
    if (GetLastError() == ERROR_ACCESS_DENIED) {
      set_access_denied_fields(*details);
    }
    return details;
  }

  DWORD path_size = MAX_PATH;
  std::vector<WCHAR> path_buffer(path_size + 1, L'\0');
  if (QueryFullProcessImageNameW(process_handle.get_handle(), 0,
                                 path_buffer.data(), &path_size)) {
    details->executable_path.assign(path_buffer.data(), path_size);
  } else {
    details->executable_path = _T("<unavailable>");
  }

  details->cmdline_args = get_process_cmdline(static_cast<int>(pid));
  if (details->cmdline_args.empty()) {
    details->cmdline_args = _T("<unavailable>");
  }

  FILETIME create_time{};
  FILETIME exit_time{};
  FILETIME kernel_time{};
  FILETIME user_time{};
  if (GetProcessTimes(process_handle.get_handle(), &create_time, &exit_time,
                      &kernel_time, &user_time)) {
    details->start_time = format_system_time(create_time);
  } else {
    details->start_time = _T("<unavailable>");
  }

  DWORD session_id = 0;
  if (ProcessIdToSessionId(static_cast<DWORD>(pid), &session_id)) {
    details->session_id = session_id;
  }

  details->architecture =
      query_process_architecture(process_handle.get_handle());

  smart_handle token_handle(nullptr);
  HANDLE token_raw = nullptr;
  if (OpenProcessToken(process_handle.get_handle(), TOKEN_QUERY, &token_raw)) {
    token_handle = smart_handle(token_raw);
    details->user = query_token_user(token_handle.get_handle());
    if (!details->user.has_value()) {
      details->user = _T("<unavailable>");
    }

    details->elevated = query_token_elevation(token_handle.get_handle());
    details->integrity_level =
        query_token_integrity_level(token_handle.get_handle());
  } else {
    details->user = _T("<unavailable>");
    details->elevated = std::nullopt;
    details->integrity_level = _T("<unavailable>");
  }

  return details;
}
#endif  // _WIN32

#ifndef _WIN32
std::optional<proc_info_details> get_process_details_info_linux(
    const uint32_t pid) {
  const std::string proc_dir = std::format("/proc/{}", pid);
  struct stat sb{};
  if (stat(proc_dir.c_str(), &sb) != 0) {
    return std::nullopt;
  }

  proc_info_details details;
  details.proc_pid = static_cast<int>(pid);
  details.executable_path = get_process_path(static_cast<int>(pid));

  std::string comm_path = std::format("{}/comm", proc_dir);
  std::ifstream comm_file(comm_path);
  if (comm_file) {
    std::string comm;
    std::getline(comm_file, comm);
    details.proc_name = comm;
  }

  details.cmdline_args = get_process_cmdline(static_cast<int>(pid));
  if (details.cmdline_args.empty()) {
    details.cmdline_args = _T("<unavailable>");
  }

  std::string status_path = std::format("{}/status", proc_dir);
  std::ifstream status_file(status_path);
  if (status_file) {
    std::string line;
    uint32_t uid = 0;
    while (std::getline(status_file, line)) {
      if (line.find("PPid:") == 0) {
        std::sscanf(line.c_str(), "PPid:\t%d", &details.parent_pid);
      } else if (line.find("Uid:") == 0) {
        std::sscanf(line.c_str(), "Uid:\t%u", &uid);
        details.user = std::to_string(uid);

        struct passwd* pw = getpwuid(uid);
        if (pw != nullptr) {
          details.user = pw->pw_name;
        }

        details.elevated = (uid == 0);
      }
    }
  }

  std::string stat_path = std::format("{}/stat", proc_dir);
  std::ifstream stat_file(stat_path);
  if (stat_file) {
    std::string line;
    std::getline(stat_file, line);

    size_t last_paren = line.rfind(')');
    if (last_paren != std::string::npos) {
      std::istringstream iss(line.substr(last_paren + 1));

      char state = '\0';
      int ppid = 0;
      int pgrp = 0;
      int session = 0;
      int tty_nr = 0;
      int tpgid = 0;
      unsigned long flags = 0;
      unsigned long minflt = 0;
      unsigned long cminflt = 0;
      unsigned long majflt = 0;
      unsigned long cmajflt = 0;
      unsigned long utime = 0;
      unsigned long stime = 0;
      long cutime = 0;
      long cstime = 0;
      long priority = 0;
      long nice = 0;
      long num_threads = 0;
      long itrealvalue = 0;
      unsigned long long starttime = 0;

        if (!(iss >> state >> ppid >> pgrp >> session >> tty_nr >> tpgid >>
          flags >> minflt >> cminflt >> majflt >> cmajflt >> utime >>
          stime >> cutime >> cstime >> priority >> nice >> num_threads >>
          itrealvalue >> starttime)) {
        return details;
        }

      details.session_id = session;
        details.parent_pid = ppid;

      long clk_tck = sysconf(_SC_CLK_TCK);
      if (clk_tck <= 0)
        clk_tck = 100;

      std::ifstream stat_info("/proc/stat");
      unsigned long btime = 0;
      if (stat_info) {
        std::string stat_line;
        while (std::getline(stat_info, stat_line)) {
          if (stat_line.find("btime") == 0) {
            std::sscanf(stat_line.c_str(), "btime %lu", &btime);
            break;
          }
        }
      }

      if (btime > 0) {
        time_t start_seconds = btime + (starttime / clk_tck);
        struct tm* tm_info = localtime(&start_seconds);
        if (tm_info != nullptr) {
          char buffer[64];
          strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
          details.start_time = buffer;
        }
      }
    }
  }

  int exe_fd = open(std::format("{}/exe", proc_dir).c_str(), O_RDONLY);
  if (exe_fd >= 0) {
    unsigned char elf_header[20];
    if (read(exe_fd, elf_header, sizeof(elf_header)) == sizeof(elf_header)) {
      if (elf_header[0] == 0x7f && elf_header[1] == 'E' &&
          elf_header[2] == 'L' && elf_header[3] == 'F') {
        unsigned char ei_class = elf_header[4];

        if (ei_class == 1) {
          details.architecture = _T("x86");
        } else if (ei_class == 2) {
          details.architecture = _T("x64");
        } else {
          details.architecture = _T("<unavailable>");
        }
      }
    }
    close(exe_fd);
  }

  details.integrity_level = std::nullopt;

  return details;
}
#endif  // !_WIN32

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
        << _T("' name or including this name pattern was found.") << std::endl;
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

ustring get_process_path(const int process_pid) {
  ustring process_path;
#ifdef _WIN32
  smart_handle hnd(
      OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_pid));
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

ustring get_process_cmdline(const int process_pid) {
  ustring cmdline;
#ifdef _WIN32
  smart_handle hnd(
      OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_pid));
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

std::optional<proc_info_details> get_process_details_info(const uint32_t pid) {
#ifdef _WIN32
  return get_process_details_info_win32(pid);
#else
  return get_process_details_info_linux(pid);
#endif
}

void print_process_info_report(const proc_info_details& details) {
  const auto format_optional_value =
      [](const auto& opt,
         const ustring& fallback =
             _T(
                                            "<unavailable>"))
      -> ustring {
    if (!opt.has_value()) {
      return fallback;
    }

    using opt_type = std::remove_cvref_t<decltype(opt)>;
    using value_t = typename opt_type::value_type;
    if constexpr (std::is_same_v<value_t, bool>) {
      return opt.value() ? _T("Yes") : _T("No");
    } else if constexpr (std::is_integral_v<value_t>) {
      return std::format(_T("{}"), opt.value());
    } else {
      return opt.value();
    }
  };

  ucout << std::format(_T("{:<14}{}\n"), _T("PID:"), details.proc_pid);
  ucout << std::format(_T("{:<14}{}\n"), _T("Name:"), details.proc_name);
  ucout << std::format(_T("{:<14}{}\n"), _T("Parent PID:"), details.parent_pid);
  ucout << std::format(_T("{:<14}{}\n"), _T("Parent Name:"),
                       format_optional_value(details.parent_name));
  ucout << std::format(_T("{:<14}{}\n"), _T("User:"),
                       format_optional_value(details.user));
  ucout << std::format(_T("{:<14}{}\n"), _T("Session ID:"),
                       format_optional_value(details.session_id, _T("N/A")));
  ucout << std::format(_T("{:<14}{}\n"), _T("Architecture:"),
                       format_optional_value(details.architecture));
  ucout << std::format(_T("{:<14}{}\n"), _T("Start Time:"),
                       format_optional_value(details.start_time));
  ucout << std::format(
      _T("{:<14}{}\n"), _T("Integrity:"),
      format_optional_value(details.integrity_level, _T("N/A")));
  ucout << std::format(_T("{:<14}{}\n"), _T("Elevated:"),
                       format_optional_value(details.elevated, _T("N/A")));
  ucout << std::format(_T("{:<14}{}\n"), _T("Path:"), details.executable_path);
  ucout << std::format(_T("{:<14}{}\n"), _T("Command Line:"),
                       details.cmdline_args);
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
