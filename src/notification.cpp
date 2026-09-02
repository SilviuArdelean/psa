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

#include "notification.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <spawn.h>
#include <sys/wait.h>

extern char** environ;
#endif

namespace psa::system::detail {

std::string sanitize_text(const std::string& input, size_t max_len) {
  static constexpr const char* kAllowedPunctuation = ".,!?-_:'()@ ";

  std::string sanitized;
  sanitized.reserve((std::min)(input.size(), max_len));

  for (const unsigned char ch : input) {
    if (sanitized.size() >= max_len) {
      break;
    }

    const bool is_allowed =
        std::isalnum(ch) ||
        std::strchr(kAllowedPunctuation, static_cast<char>(ch)) != nullptr;
    if (is_allowed) {
      sanitized.push_back(static_cast<char>(ch));
    }
  }

  return sanitized;
}

namespace {

std::string escape_single_quoted(const std::string& text, bool powershell_style) {
  std::string escaped;
  escaped.reserve(text.size());

  for (const char ch : text) {
    if (ch == '\'') {
      escaped += powershell_style ? "''" : "'\\''";
    } else {
      escaped.push_back(ch);
    }
  }

  return escaped;
}

}  // namespace

#if defined(_WIN32)

std::wstring widen_ascii(const std::string& text) {
  return std::wstring(text.begin(), text.end());
}

bool file_exists(const std::wstring& path) {
  const DWORD attributes = GetFileAttributesW(path.c_str());
  return attributes != INVALID_FILE_ATTRIBUTES &&
         (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring read_environment_path(const wchar_t* variable_name) {
  const DWORD required_size = GetEnvironmentVariableW(variable_name, nullptr, 0);
  if (required_size == 0) {
    return L"";
  }

  std::wstring value(required_size - 1, L'\0');
  GetEnvironmentVariableW(variable_name, value.data(), required_size);
  return value;
}

std::wstring join_windows_path(const std::wstring& base,
                               const std::wstring& suffix) {
  if (base.empty()) {
    return suffix;
  }

  if (base.back() == L'\\' || base.back() == L'/') {
    return base + suffix;
  }

  return base + L"\\" + suffix;
}

std::wstring quote_windows_argument(const std::wstring& argument) {
  if (argument.empty()) {
    return L"\"\"";
  }

  const bool requires_quotes =
      argument.find_first_of(L" \t\n\v\"") != std::wstring::npos;
  if (!requires_quotes) {
    return argument;
  }

  std::wstring quoted;
  quoted.push_back(L'\"');

  size_t backslash_count = 0;
  for (const wchar_t ch : argument) {
    if (ch == L'\\') {
      ++backslash_count;
      continue;
    }

    if (ch == L'\"') {
      quoted.append(backslash_count * 2 + 1, L'\\');
      quoted.push_back(L'\"');
      backslash_count = 0;
      continue;
    }

    if (backslash_count != 0) {
      quoted.append(backslash_count, L'\\');
      backslash_count = 0;
    }

    quoted.push_back(ch);
  }

  if (backslash_count != 0) {
    quoted.append(backslash_count * 2, L'\\');
  }

  quoted.push_back(L'\"');
  return quoted;
}

std::wstring build_windows_command_line(
    const std::wstring& executable,
    const std::vector<std::wstring>& arguments) {
  std::wstring command_line = quote_windows_argument(executable);
  for (const std::wstring& argument : arguments) {
    command_line.push_back(L' ');
    command_line += quote_windows_argument(argument);
  }

  return command_line;
}

std::wstring resolve_powershell_path() {
  const std::wstring system_root = read_environment_path(L"SystemRoot");
  const std::wstring program_files = read_environment_path(L"ProgramFiles");

  const std::vector<std::wstring> candidates = {
      join_windows_path(system_root,
                        L"System32\\WindowsPowerShell\\v1.0\\powershell.exe"),
      join_windows_path(program_files, L"PowerShell\\7\\pwsh.exe"),
      join_windows_path(program_files, L"PowerShell\\6\\pwsh.exe")};

  for (const std::wstring& candidate : candidates) {
    if (file_exists(candidate)) {
      return candidate;
    }
  }

  return L"";
}

bool spawn_windows_process(const std::wstring& executable,
                           const std::vector<std::wstring>& arguments) {
  std::wstring command_line = build_windows_command_line(executable, arguments);
  std::vector<wchar_t> mutable_command_line(command_line.begin(),
                                            command_line.end());
  mutable_command_line.push_back(L'\0');

  STARTUPINFOW startup_info{};
  startup_info.cb = sizeof(startup_info);
  PROCESS_INFORMATION process_info{};

  if (!CreateProcessW(executable.c_str(), mutable_command_line.data(), nullptr,
                      nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr,
                      &startup_info, &process_info)) {
    return false;
  }

  const HANDLE process_handle = process_info.hProcess;
  const HANDLE thread_handle = process_info.hThread;

  WaitForSingleObject(process_handle, INFINITE);

  DWORD exit_code = 1;
  const bool has_exit_code =
      GetExitCodeProcess(process_handle, &exit_code) != FALSE;

  CloseHandle(thread_handle);
  CloseHandle(process_handle);
  return has_exit_code && exit_code == 0;
}

#endif

std::string build_windows_script(const std::string& title,
                                 const std::string& message) {
  const std::string safe_title =
      escape_single_quoted(sanitize_text(title), /*powershell_style=*/true);
  const std::string safe_message =
      escape_single_quoted(sanitize_text(message), /*powershell_style=*/true);

  return "Add-Type -AssemblyName PresentationFramework; "
         "[System.Windows.MessageBox]::Show('" +
         safe_message + "','" + safe_title +
         "',[System.Windows.MessageBoxButton]::OK," \
         "[System.Windows.MessageBoxImage]::Warning)";
}

std::vector<std::string> build_linux_arguments(const std::string& title,
                                               const std::string& message) {
  return {"--icon=dialog-warning", sanitize_text(title),
          sanitize_text(message)};
}

}  // namespace psa::system::detail

namespace psa::system {

#if defined(_WIN32)

bool show_notification(const std::string& title, const std::string& message) {
  const std::wstring executable = detail::resolve_powershell_path();
  if (executable.empty()) {
    return false;
  }

  const std::string script = detail::build_windows_script(title, message);
  const std::vector<std::wstring> arguments = {
      L"-NoProfile", L"-NonInteractive", L"-WindowStyle", L"Hidden",
      L"-Command", detail::widen_ascii(script)};

  return detail::spawn_windows_process(executable, arguments);
}

#elif defined(__linux__)

bool show_notification(const std::string& title, const std::string& message) {
  const std::vector<std::string> arguments =
      detail::build_linux_arguments(title, message);
  std::vector<char*> argv;
  argv.reserve(arguments.size() + 2);
  argv.push_back(const_cast<char*>("notify-send"));

  for (const std::string& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  argv.push_back(nullptr);

  pid_t child_pid = 0;
  const int spawn_result =
      posix_spawnp(&child_pid, "notify-send", nullptr, nullptr, argv.data(),
                   environ);
  if (spawn_result != 0) {
    return false;
  }

  int status = 0;
  if (waitpid(child_pid, &status, 0) < 0) {
    return false;
  }

  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

#elif defined(__APPLE__)

// osascript -e 'display notification "..." with title "..."' — not implemented yet.
bool show_notification(const std::string& /*title*/,
                       const std::string& /*message*/) {
  return false;
}

#else

bool show_notification(const std::string& /*title*/,
                       const std::string& /*message*/) {
  return false;
  }

#endif

}  // namespace psa::system
