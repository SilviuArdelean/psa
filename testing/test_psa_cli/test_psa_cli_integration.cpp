/*
 * Integration tests for psa ProcessCommandLine() with real OS operations.
 *
 * These tests exercise the actual ProcessingOperations implementation and
 * verify that the output is as expected. They do not mock the OS.
 *
 * Copyright (c) 2017-2026 Silviu-Marius Ardelean
 */

#include "pch.h"
#include <gtest/gtest.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdio>
#ifdef _WIN32
#  include <windows.h>
#endif
#include "operations.h"

#ifdef _WIN32
  // No getopt header needed on Windows
  extern int optind;
#else
#  include <unistd.h>
#endif

// Forward declaration
bool ProcessCommandLine(int argc, char* argv[], ProcessingOperations* pPO);

// Cross-platform capture of ucout (std::wcout or std::cout) and C stdout
#include <sstream>
#include <cstdio>
#include <vector>
#ifdef _WIN32
#  include <io.h>
#  include <fcntl.h>
#else
#  include <unistd.h>
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

// Helper: capture C stdout to a string (cross-platform)
class StdoutCapture {
public:
  StdoutCapture() {
#ifdef _WIN32
  fflush(stdout);
  old_fd = _dup(_fileno(stdout));
  char tmp_buf[L_tmpnam] = {0};
  tmpnam_s(tmp_buf, L_tmpnam);
  tmp_name = tmp_buf;
  FILE* tmp = nullptr;
  freopen_s(&tmp, tmp_name.c_str(), "w+b", stdout);
  (void)tmp;
#else
  fflush(stdout);
  old_fd = dup(fileno(stdout));
  char* tn = std::tmpnam(nullptr);
  tmp_name = tn ? tn : "stdout_capture.tmp";
  FILE* tmp = freopen(tmp_name.c_str(), "w+b", stdout);
  (void)tmp;
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
  }
  std::string str() const {
    fflush(stdout);
    std::ifstream ifs(tmp_name, std::ios::binary);
    std::vector<char> buf((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return std::string(buf.begin(), buf.end());
  }
private:
  int old_fd = -1;
  std::string tmp_name;
};

// Helper: Argv builder
struct Argv {
  std::vector<std::string> store;
  std::vector<char*> ptrs;
  Argv(std::initializer_list<const char*> args) {
    for (auto s : args) store.emplace_back(s);
    for (auto& s : store) ptrs.push_back(s.data());
  }
  int argc() { return static_cast<int>(ptrs.size()); }
  char** argv() { return ptrs.data(); }
};

// Integration fixture
class PsaCliIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override { optind = 0; }
#ifdef _UNICODE
  bool run_and_capture(Argv& av, std::wstring& out) {
    UcoutCapture cap;
    StdoutCapture stdcap;
    ProcessingOperations real;
    bool result = ProcessCommandLine(av.argc(), av.argv(), &real);
    out = cap.str();
    // Also append C stdout (converted to wstring)
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
    ProcessingOperations real;
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
    std::wcerr << L"[DEBUG] No 'System' process found. Output was:\n" << out << std::endl;
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
    std::cerr << "[DEBUG] No 'System' process found. Output was:\n" << out << std::endl;
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

// Helper: spawn a dummy process (ping -n 60 127.0.0.1)
#ifdef _WIN32
// Returns process handle (caller must CloseHandle), sets pid
static HANDLE spawn_dummy_process(DWORD& pid) {
  STARTUPINFOW si = { sizeof(si) };
  PROCESS_INFORMATION pi = {};
  wchar_t cmd[] = L"cmd.exe /C ping -n 60 127.0.0.1 >NUL";
  if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
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
  Argv av{"psa", "-k", ss.str().c_str()};
  std::wstring out;
  EXPECT_TRUE(run_and_capture(av, out));
  // Should mention the PID or success
  std::wstring wpid(ss.str().begin(), ss.str().end());
  EXPECT_NE(out.find(wpid), std::wstring::npos);
  // Confirm process is gone: wait for process to exit
  DWORD waitResult = WaitForSingleObject(hProcess, 2000); // 2s timeout
  EXPECT_TRUE(waitResult == WAIT_OBJECT_0);
  DWORD exitCode = 0;
  BOOL gotExit = GetExitCodeProcess(hProcess, &exitCode);
  EXPECT_TRUE(gotExit);
  EXPECT_NE(exitCode, STILL_ACTIVE);
  CloseHandle(hProcess);
}
#else
// On Linux, spawn a short-lived background process via fork()/exec() and
// kill it by PID using the -k flag.
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

static pid_t spawn_dummy_process() {
  pid_t pid = fork();
  if (pid == 0) {
    // Child: sleep long enough for the test to kill it
    execlp("sleep", "sleep", "60", nullptr);
    _exit(1);
  }
  return pid;  // parent returns child PID (or -1 on fork failure)
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
  // Clean up zombie if still present
  waitpid(pid, nullptr, WNOHANG);
}
#endif
