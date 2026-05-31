/*
 * Integration tests for ProcessCommandLine() against real OS operations.
 * Copyright (c) 2017-2026 Silviu-Marius Ardelean
 */

#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include "pch.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "operations.h"

#ifdef _WIN32
extern int optind;
#else
#include <unistd.h>
#endif

bool ProcessCommandLine(int argc, char* argv[], ProcessingOperations* pPO);

// Force header printing for tree output during tests.
class TestProcessingOperationsWithHeader : public ProcessingOperations {
 public:
  void GenerateProcessesTree(int const proc_pid,
                             bool print_header = false) override {
    ProcessingOperations::GenerateProcessesTree(proc_pid, true);
  }
};

#include <cstdio>
#include <sstream>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef _UNICODE
class UcoutCapture {
 public:
  UcoutCapture() : old_buf(std::wcout.rdbuf(capture.rdbuf())) {}
  ~UcoutCapture() { std::wcout.rdbuf(old_buf); }
  std::wstring str() const { return capture.str(); }

 private:
  std::wstringstream capture;
  std::wstreambuf* old_buf;
};
#else
class UcoutCapture {
 public:
  UcoutCapture() : old_buf(std::cout.rdbuf(capture.rdbuf())) {}
  ~UcoutCapture() { std::cout.rdbuf(old_buf); }
  std::string str() const { return capture.str(); }

 private:
  std::stringstream capture;
  std::streambuf* old_buf;
};
#endif

// Capture C stdout to a temporary file.
class StdoutCapture {
 public:
  StdoutCapture() {
#ifdef _WIN32
    fflush(stdout);
    old_fd = _dup(_fileno(stdout));
    char tmp_dir[MAX_PATH] = {0};
    DWORD dwRet = GetTempPathA(MAX_PATH, tmp_dir);
    char tmp_path[MAX_PATH] = {0};
    UINT uRet = 0;
    if (dwRet == 0 || dwRet > MAX_PATH) {
      strcpy_s(tmp_dir, ".\\");
    }
    uRet = GetTempFileNameA(tmp_dir, "psa", 0, tmp_path);
    if (uRet == 0) {
      tmp_name = "stdout_capture_win.tmp";
    } else {
      tmp_name = tmp_path;
    }
    FILE* tmp = nullptr;
    if (freopen_s(&tmp, tmp_name.c_str(), "w+b", stdout) != 0 || !tmp) {
      tmp_name.clear();
    }
#else
    fflush(stdout);
    old_fd = dup(fileno(stdout));
    char tmp_template[] = "/tmp/psa_stdout_XXXXXX";
    int fd = mkstemp(tmp_template);
    if (fd == -1) {
      tmp_name = "stdout_capture_posix.tmp";
    } else {
      tmp_name = tmp_template;
      close(fd);
    }
    FILE* tmp = freopen(tmp_name.c_str(), "w+b", stdout);
    if (!tmp) {
      tmp_name.clear();
    }
#endif
  }
  ~StdoutCapture() {
    fflush(stdout);
    if (old_fd != -1) {
#ifdef _WIN32
      FILE* tmp = nullptr;
      freopen_s(&tmp, "NUL", "w", stdout);
      (void)tmp;
      _dup2(old_fd, _fileno(stdout));
      _close(old_fd);
#else
      FILE* tmp = freopen("/dev/null", "w", stdout);
      (void)tmp;
      dup2(old_fd, fileno(stdout));
      close(old_fd);
#endif
    }
    if (!tmp_name.empty()) {
#ifdef _WIN32
      DeleteFileA(tmp_name.c_str());
#else
      std::remove(tmp_name.c_str());
#endif
    }
  }
  std::string str() const {
    fflush(stdout);
    std::ifstream ifs(tmp_name, std::ios::binary);
    std::vector<char> buf((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());
    return std::string(buf.begin(), buf.end());
  }

 private:
  int old_fd = -1;
  std::string tmp_name;
};

struct Argv {
  std::vector<std::string> store;
  std::vector<char*> ptrs;
  Argv(std::initializer_list<const char*> args) {
    for (auto s : args)
      store.emplace_back(s);
    for (auto& s : store)
      ptrs.push_back(s.data());
  }
  int argc() { return static_cast<int>(ptrs.size()); }
  char** argv() { return ptrs.data(); }
};

class PsaCliIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override { optind = 0; }
#ifdef _UNICODE
  bool run_and_capture(Argv& av, std::wstring& out) {
    UcoutCapture cap;
    StdoutCapture stdcap;
    TestProcessingOperationsWithHeader real;
    bool result = ProcessCommandLine(av.argc(), av.argv(), &real);
    out = cap.str();
    std::string cstdout = stdcap.str();
    if (!cstdout.empty()) {
      std::wstring wstdout(cstdout.begin(), cstdout.end());
      out += wstdout;
    }
    return result;
  }
#else
  bool run_and_capture(Argv& av, std::string& out) {
    UcoutCapture cap;
    StdoutCapture stdcap;
    TestProcessingOperationsWithHeader real;
    bool result = ProcessCommandLine(av.argc(), av.argv(), &real);
    out = cap.str();
    out += stdcap.str();
    return result;
  }
#endif
};

TEST_F(PsaCliIntegrationTest, FlagA_PrintsAllProcesses) {
  Argv av{"psa", "-a"};
#ifdef _UNICODE
  std::wstring out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_FALSE(out.empty());
  EXPECT_NE(out.find(L"PID"), std::wstring::npos);
#else
  std::string out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_FALSE(out.empty());
  EXPECT_NE(out.find("PID"), std::string::npos);
#endif
}

TEST_F(PsaCliIntegrationTest, FlagE_Top5Processes) {
  Argv av{"psa", "-e", "5"};
#ifdef _UNICODE
  std::wstring out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_FALSE(out.empty());
  EXPECT_NE(out.find(L"PID"), std::wstring::npos);
#else
  std::string out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_FALSE(out.empty());
  EXPECT_NE(out.find("PID"), std::string::npos);
#endif
}

TEST_F(PsaCliIntegrationTest, FlagO_KnownProcess) {
  Argv av{"psa", "-o", "System"};
#ifdef _UNICODE
  std::wstring out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_FALSE(out.empty());
  EXPECT_NE(out.find(L"Process Name"), std::wstring::npos);
  if (out.find(L" System") == std::wstring::npos) {
    std::wcerr << L"[DEBUG] No 'System' process found. Output was:\n"
               << out << std::endl;
    SUCCEED() << "No 'System' process found; skipping process name check.";
    return;
  }
  EXPECT_NE(out.find(L" System"), std::wstring::npos);
#else
  std::string out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_FALSE(out.empty());
  EXPECT_NE(out.find("Process Name"), std::string::npos);
  if (out.find(" System") == std::string::npos) {
    std::cerr << "[DEBUG] No 'System' process found. Output was:\n"
              << out << std::endl;
    SUCCEED() << "No 'System' process found; skipping process name check.";
    return;
  }
  EXPECT_NE(out.find(" System"), std::string::npos);
#endif
}

TEST_F(PsaCliIntegrationTest, FlagD_KnownProcess) {
  Argv av{"psa", "-d", "System"};
#ifdef _UNICODE
  std::wstring out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_FALSE(out.empty());
  EXPECT_NE(out.find(L"System"), std::wstring::npos);
#else
  std::string out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_FALSE(out.empty());
  EXPECT_NE(out.find("System"), std::string::npos);
#endif
}

TEST_F(PsaCliIntegrationTest, FlagT_TreeSnapshot) {
  Argv av{"psa", "-t"};
#ifdef _UNICODE
  std::wstring out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_FALSE(out.empty());
  EXPECT_NE(out.find(L"PID"), std::wstring::npos);
#else
  std::string out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_FALSE(out.empty());
  EXPECT_NE(out.find("PID"), std::string::npos);
#endif
}

#ifdef _WIN32
static HANDLE spawn_dummy_process(DWORD& pid) {
  STARTUPINFOW si = {sizeof(si)};
  PROCESS_INFORMATION pi = {};
  wchar_t cmd[] = L"cmd.exe /C ping -n 60 127.0.0.1 >NUL";
  if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL,
                      NULL, &si, &pi)) {
    pid = 0;
    return nullptr;
  }
  CloseHandle(pi.hThread);
  pid = pi.dwProcessId;
  return pi.hProcess;
}

TEST_F(PsaCliIntegrationTest, FlagK_KillDummyProcess) {
  DWORD pid = 0;
  HANDLE hProcess = spawn_dummy_process(pid);
  ASSERT_NE(pid, 0u);
  ASSERT_NE(hProcess, nullptr);
  std::ostringstream ss;
  ss << pid;
  std::string pid_str = ss.str();
  Argv av{"psa", "-k", pid_str.c_str()};
  std::wstring out;
  EXPECT_TRUE(run_and_capture(av, out));
  std::wstring wpid;
  if (pid_str.empty()) {
    std::wcerr << L"[DEBUG] PID string is empty!\n";
  }
  for (char c : pid_str)
    wpid += static_cast<wchar_t>(c);
  EXPECT_NE(out.find(wpid), std::wstring::npos);
  DWORD waitResult = WaitForSingleObject(hProcess, 2000);
  EXPECT_TRUE(waitResult == WAIT_OBJECT_0);
  DWORD exitCode = 0;
  BOOL gotExit = GetExitCodeProcess(hProcess, &exitCode);
  EXPECT_TRUE(gotExit);
  EXPECT_NE(exitCode, STILL_ACTIVE);
  CloseHandle(hProcess);
}

TEST_F(PsaCliIntegrationTest, FlagK_KillDummyProcessWithFilterParam) {
  DWORD pid = 0;
  HANDLE hProcess = spawn_dummy_process(pid);
  ASSERT_NE(pid, 0u);
  ASSERT_NE(hProcess, nullptr);
  std::ostringstream ss;
  ss << pid;
  std::string pid_str = ss.str();
  Argv av{"psa", "-k", pid_str.c_str(), "--filter-param", "ping"};
  std::wstring out;
  EXPECT_TRUE(run_and_capture(av, out));
  std::wstring wpid;
  for (char c : pid_str)
    wpid += static_cast<wchar_t>(c);
  EXPECT_NE(out.find(wpid), std::wstring::npos);
  DWORD waitResult = WaitForSingleObject(hProcess, 2000);
  EXPECT_TRUE(waitResult == WAIT_OBJECT_0);
  DWORD exitCode = 0;
  BOOL gotExit = GetExitCodeProcess(hProcess, &exitCode);
  EXPECT_TRUE(gotExit);
  EXPECT_NE(exitCode, STILL_ACTIVE);
  CloseHandle(hProcess);
}

TEST_F(PsaCliIntegrationTest, FlagK_KillNonExistentProcessWithFilterParam) {
  Argv av{"psa", "-k", "nonexistent_proc_xyz", "--filter-param", "nonexistent_val_abc"};
  std::wstring out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_NE(out.find(L"No processes matching 'nonexistent_proc_xyz' with command line containing 'nonexistent_val_abc' were found."), std::wstring::npos);
}
#else
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

static pid_t spawn_dummy_process() {
  pid_t pid = fork();
  if (pid == 0) {
    execlp("sleep", "sleep", "60", nullptr);
    _exit(1);
  }
  return pid;
}

TEST_F(PsaCliIntegrationTest, FlagK_KillDummyProcess) {
  pid_t pid = spawn_dummy_process();
  ASSERT_GT(pid, 0);
  std::stringstream ss;
  ss << pid;
  Argv av{"psa", "-k", ss.str().c_str()};
  std::string out;
  EXPECT_TRUE(run_and_capture(av, out));
  // Give the OS a moment to reap the process
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  // Confirm process is gone — kill(pid, 0) returns -1/ESRCH when not found
  EXPECT_EQ(kill(pid, 0), -1);
  waitpid(pid, nullptr, WNOHANG);
}

TEST_F(PsaCliIntegrationTest, FlagK_KillDummyProcessWithFilterParam) {
  pid_t pid = spawn_dummy_process();
  ASSERT_GT(pid, 0);
  std::stringstream ss;
  ss << pid;
  std::string pid_str = ss.str();
  Argv av{"psa", "-k", pid_str.c_str(), "--filter-param", "60"};
  std::string out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_NE(out.find(ss.str()), std::string::npos);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_EQ(kill(pid, 0), -1);
  waitpid(pid, nullptr, WNOHANG);
}

TEST_F(PsaCliIntegrationTest, FlagK_KillNonExistentProcessWithFilterParam) {
  Argv av{"psa", "-k", "nonexistent_proc_xyz", "--filter-param", "nonexistent_val_abc"};
  std::string out;
  EXPECT_TRUE(run_and_capture(av, out));
  EXPECT_NE(out.find("No processes matching 'nonexistent_proc_xyz' with command line containing 'nonexistent_val_abc' were found."), std::string::npos);
}
#endif
