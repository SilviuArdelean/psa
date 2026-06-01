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

// test_psa_cli.cpp — GTest unit tests for ProcessCommandLine()
//
// These tests exercise the command-line parsing logic in psa.cpp without
// hitting the OS.  A FakeProcessingOperations subclass intercepts every
// virtual call so we can assert on what was (or was not) invoked.

#include "pch.h"

#include <gtest/gtest.h>

#include "operations.h"

#ifdef _WIN32
int optind = 0;
#endif

// ---------------------------------------------------------------------------
// Platform-specific getopt state reset
// ---------------------------------------------------------------------------
#ifdef _WIN32
// No getopt header needed on Windows
extern int optind;
#else
#include <unistd.h>  // declares extern int optind
#endif

// ---------------------------------------------------------------------------
// Forward declaration — definition is compiled from src/psa.cpp
// ---------------------------------------------------------------------------
bool ProcessCommandLine(int argc, char* argv[], ProcessingOperations* pPO);

// ---------------------------------------------------------------------------
// Argv helper — builds a TCHAR* argv[] from an initializer list.
// Owns the string storage to keep pointers valid for the duration of the call.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// FakeProcessingOperations — records every virtual call for assertion
// ---------------------------------------------------------------------------
struct FakeProcessingOperations : ProcessingOperations {
  struct Call {
    std::string name;
    bool show_details = false;
    ustring str_arg;
    int int_arg = 0;
    ustring cmdline_arg;
    uint32_t pid_arg = 0;
  };

  std::vector<Call> calls;
  bool return_value = true;
  std::optional<proc_info_details> pid_lookup_result = std::nullopt;

  bool PrintAllProcessesInformation(bool const show_details = false) override {
    calls.push_back({"PrintAll", show_details});
    return return_value;
  }

  bool PrintProcessInformation(const ustring& process_name,
                               bool const show_details = false) override {
    calls.push_back({"PrintOne", show_details, process_name});
    return return_value;
  }

  void PrintTopExpensiveProcesses(const int top) override {
    calls.push_back({"TopExpensive", false, {}, top});
  }

  void KillProcesses(TCHAR const* argvProcessParam) override {
    calls.push_back({"KillProcesses", false, ustring(argvProcessParam)});
  }

  void KillProcessesWithCmdLine(const ustring& argvProcessParam,
                                const ustring& cmdlineFilter) override {
    calls.push_back({"KillProcessesWithCmdLine", false, argvProcessParam, 0,
                     cmdlineFilter});
  }

  std::optional<proc_info_details> GetProcessInfoByPid(
      const uint32_t process_pid) override {
    calls.push_back({"GetProcessInfoByPid", false, {}, 0, {}, process_pid});
    return pid_lookup_result;
  }

  void PrintProcessInfoReport(const proc_info_details& details) override {
    calls.push_back({"PrintProcessInfoReport",
                     false,
                     {},
                     0,
                     {},
                     static_cast<uint32_t>(details.proc_pid)});
  }

  void GenerateProcessesTree(int const proc_pid,
                             bool print_header = false) override {
    calls.push_back({"GenTree", print_header, {}, proc_pid});
  }
};

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------
class PsaCliTest : public ::testing::Test {
 protected:
  FakeProcessingOperations fake;

  void SetUp() override {
    fake.calls.clear();
    fake.return_value = true;
    fake.pid_lookup_result = std::nullopt;
    optind = 0;  // Reset getopt state between tests.
    // XGetopt (Windows) uses a static `next` pointer that is only cleared
    // when optind == 0 at the start of a getopt() call.  Setting optind = 1
    // does NOT clear it, causing dangling-pointer reads when tests share a
    // process.  Linux glibc getopt() also honours optind == 0 as a full
    // reinitialisation trigger.
  }

  bool run(Argv& av) { return ProcessCommandLine(av.argc(), av.argv(), &fake); }
};

// ===========================================================================
// -a / -A  (print all processes)
// ===========================================================================

TEST_F(PsaCliTest, FlagA_PrintsAll) {
  Argv av{"psa", "-a"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("PrintAll", fake.calls[0].name);
  EXPECT_FALSE(fake.calls[0].show_details);
}

#ifdef _WIN32
TEST_F(PsaCliTest, FlagAUpper_SameAsFlagA) {
  // On Windows getopt tolowers the option; on Linux uppercase is a separate
  // option in the optstring.
  Argv av{"psa", "-A"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("PrintAll", fake.calls[0].name);
}
#endif

// ===========================================================================
// --details  (print process details)
// ===========================================================================

TEST_F(PsaCliTest, FlagDetails_PrintsWithDetails) {
  Argv av{"psa", "-o", "chrome", "--details"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("PrintOne", fake.calls[0].name);
  EXPECT_TRUE(fake.calls[0].show_details);
  EXPECT_EQ(ustring(_T("chrome")), fake.calls[0].str_arg);
}

TEST_F(PsaCliTest, FlagO_ThenDetails_ThenValue_NormalizesAndSucceeds) {
  Argv av{"psa", "-o", "--details", "chrome"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("PrintOne", fake.calls[0].name);
  EXPECT_TRUE(fake.calls[0].show_details);
  EXPECT_EQ(ustring(_T("chrome")), fake.calls[0].str_arg);
}

TEST_F(PsaCliTest, DetailsWithoutO_ReturnsFalse) {
  // --details is a boolean flag; without -o it must be rejected.
  Argv av{"psa", "--details"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, FlagD_IsUnknownOption_ReturnsFalse) {
  Argv av{"psa", "-d", "chrome"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, FlagO_WithDetails_AndFlagA_InvokesBoth) {
  // -a and -o --details are independent; both operations should be dispatched.
  Argv av{"psa", "-a", "-o", "chrome", "--details"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(2u, fake.calls.size());
  EXPECT_EQ("PrintAll", fake.calls[0].name);
  EXPECT_EQ("PrintOne", fake.calls[1].name);
  EXPECT_TRUE(fake.calls[1].show_details);
  EXPECT_EQ(ustring(_T("chrome")), fake.calls[1].str_arg);
}

// ===========================================================================
// -e / -E  (top N expensive processes)
// ===========================================================================

TEST_F(PsaCliTest, FlagE_DefaultTop10) {
  Argv av{"psa", "-e"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("TopExpensive", fake.calls[0].name);
  EXPECT_EQ(10, fake.calls[0].int_arg);
}

TEST_F(PsaCliTest, FlagE_WithExplicitCount) {
  Argv av{"psa", "-e", "5"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("TopExpensive", fake.calls[0].name);
  EXPECT_EQ(5, fake.calls[0].int_arg);
}

TEST_F(PsaCliTest, FlagE_ZeroFallsBackToDefault) {
  Argv av{"psa", "-e", "0"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("TopExpensive", fake.calls[0].name);
  EXPECT_EQ(10, fake.calls[0].int_arg);
}

TEST_F(PsaCliTest, FlagEUpper_SameAsFlagE) {
  Argv av{"psa", "-E"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("TopExpensive", fake.calls[0].name);
  EXPECT_EQ(10, fake.calls[0].int_arg);
}

// ===========================================================================
// -k / -K  (kill processes)
// ===========================================================================

TEST_F(PsaCliTest, FlagK_KillByName) {
  Argv av{"psa", "-k", "chrome"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("KillProcesses", fake.calls[0].name);
  EXPECT_EQ(ustring(_T("chrome")), fake.calls[0].str_arg);
}

TEST_F(PsaCliTest, FlagKUpper_SameAsFlagK) {
  Argv av{"psa", "-K", "notepad"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("KillProcesses", fake.calls[0].name);
  EXPECT_EQ(ustring(_T("notepad")), fake.calls[0].str_arg);
}

TEST_F(PsaCliTest, FlagK_WithFilterParam) {
  Argv av{"psa", "-k", "chrome", "--filter-param", "network"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("KillProcessesWithCmdLine", fake.calls[0].name);
  EXPECT_EQ(ustring(_T("chrome")), fake.calls[0].str_arg);
  EXPECT_EQ(ustring(_T("network")), fake.calls[0].cmdline_arg);
}

TEST_F(PsaCliTest, FlagK_WithFilterParam_ByPid) {
  Argv av{"psa", "-k", "1234", "--filter-param", "network"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("KillProcessesWithCmdLine", fake.calls[0].name);
  EXPECT_EQ(ustring(_T("1234")), fake.calls[0].str_arg);
  EXPECT_EQ(ustring(_T("network")), fake.calls[0].cmdline_arg);
}

TEST_F(PsaCliTest, FlagK_WithEmptyFilterParam_ReturnsFalse) {
  Argv av{"psa", "-k", "chrome", "--filter-param", ""};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, FilterParamWithoutK_ReturnsFalse) {
  Argv av{"psa", "--filter-param", "network"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

// ===========================================================================
// -o / -O  (print one process)
// ===========================================================================

TEST_F(PsaCliTest, FlagO_PrintsByName) {
  Argv av{"psa", "-o", "explorer"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("PrintOne", fake.calls[0].name);
  EXPECT_FALSE(fake.calls[0].show_details);
  EXPECT_EQ(ustring(_T("explorer")), fake.calls[0].str_arg);
}

TEST_F(PsaCliTest, FlagOUpper_SameAsFlagO) {
  Argv av{"psa", "-O", "svchost"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("PrintOne", fake.calls[0].name);
  EXPECT_FALSE(fake.calls[0].show_details);
  EXPECT_EQ(ustring(_T("svchost")), fake.calls[0].str_arg);
}

// Combined: -o <name> --details alongside -e still works; both operations run.
TEST_F(PsaCliTest, FlagO_WithDetails_AndFlagE_BothRun) {
  Argv av{"psa", "-o", "chrome", "--details", "-e", "5"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(2u, fake.calls.size());
  // -e runs first (handle_entries_option order), then -o.
  const auto& entries_call = fake.calls[0];
  EXPECT_EQ("TopExpensive", entries_call.name);
  EXPECT_EQ(5, entries_call.int_arg);
  const auto& print_call = fake.calls[1];
  EXPECT_EQ("PrintOne", print_call.name);
  EXPECT_TRUE(print_call.show_details);
  EXPECT_EQ(ustring(_T("chrome")), print_call.str_arg);
}

// ===========================================================================
// -t / -T  (process tree)
// ===========================================================================

TEST_F(PsaCliTest, FlagT_DefaultRootPid) {
  Argv av{"psa", "-t"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("GenTree", fake.calls[0].name);
#ifdef _WIN32
  EXPECT_EQ(0, fake.calls[0].int_arg);  // Windows root PID = 0
#else
  EXPECT_EQ(1, fake.calls[0].int_arg);  // Linux root PID = 1
#endif
}

TEST_F(PsaCliTest, FlagT_WithExplicitPid) {
  // The -t implementation reads argv[argc-1] when argc == 3.
  Argv av{"psa", "-t", "1234"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("GenTree", fake.calls[0].name);
  EXPECT_EQ(1234, fake.calls[0].int_arg);
}

TEST_F(PsaCliTest, FlagTUpper_SameAsFlagT) {
  Argv av{"psa", "-T"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("GenTree", fake.calls[0].name);
}

// ===========================================================================
// Error / edge cases
// ===========================================================================

TEST_F(PsaCliTest, NoArgs_ReturnsFalse) {
  Argv av{"psa"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, PrintAll_FailurePropagated) {
  fake.return_value = false;
  Argv av{"psa", "-a"};
  EXPECT_FALSE(run(av));
}

TEST_F(PsaCliTest, PrintOne_FailurePropagated) {
  fake.return_value = false;
  Argv av{"psa", "-o", "chrome"};
  EXPECT_FALSE(run(av));
}

TEST_F(PsaCliTest, PrintDetails_FailurePropagated) {
  fake.return_value = false;
  Argv av{"psa", "-o", "chrome", "--details"};
  EXPECT_FALSE(run(av));
}

// Unknown flag: cxxopts treats unknown options as errors, so ProcessCommandLine
// returns false.
TEST_F(PsaCliTest, UnknownFlag_ReturnsFalse) {
  Argv av{"psa", "-z"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

// "-?" is literally passed as an argument.  getopt returns '?' (not in
// The code now explicitly checks for "-?" before invoking cxxopts,
// so ProcessCommandLine returns true (show-help path).
TEST_F(PsaCliTest, HelpFlag_ReturnsTrue) {
  Argv av{"psa", "-?"};
  EXPECT_TRUE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

// The parser rejects values that begin with '-' (looks like a flag).
TEST_F(PsaCliTest, FlagO_FlagAsArg_ReturnsFalse) {
  Argv av{"psa", "-o", "-a"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, FlagK_FlagAsArg_ReturnsFalse) {
  Argv av{"psa", "-k", "-a"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

// ===========================================================================
// --pid  (process information by PID)
// ===========================================================================

TEST_F(PsaCliTest, FlagPid_ValidPositiveValue_CallsLookupAndPrintReport) {
  proc_info_details details;
  details.proc_pid = 1234;
  details.proc_name = _T("dummy");
  fake.pid_lookup_result = details;

  Argv av{"psa", "--pid", "1234"};
  EXPECT_TRUE(run(av));
  ASSERT_EQ(2u, fake.calls.size());
  EXPECT_EQ("GetProcessInfoByPid", fake.calls[0].name);
  EXPECT_EQ(1234u, fake.calls[0].pid_arg);
  EXPECT_EQ("PrintProcessInfoReport", fake.calls[1].name);
  EXPECT_EQ(1234u, fake.calls[1].pid_arg);
}

TEST_F(PsaCliTest, FlagPid_NotFound_ReturnsFalse) {
  fake.pid_lookup_result = std::nullopt;

  Argv av{"psa", "--pid", "999999"};
  EXPECT_FALSE(run(av));
  ASSERT_EQ(1u, fake.calls.size());
  EXPECT_EQ("GetProcessInfoByPid", fake.calls[0].name);
  EXPECT_EQ(999999u, fake.calls[0].pid_arg);
}

TEST_F(PsaCliTest, FlagPid_InvalidAlpha_ReturnsFalse) {
  Argv av{"psa", "--pid", "abc"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, FlagPid_InvalidNegative_ReturnsFalse) {
  Argv av{"psa", "--pid", "-1"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, FlagPid_InvalidZero_ReturnsFalse) {
  Argv av{"psa", "--pid", "0"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, FlagPid_InvalidTrailingSpace_ReturnsFalse) {
  Argv av{"psa", "--pid", "1234 "};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, FlagPid_InvalidOverflow_ReturnsFalse) {
  Argv av{"psa", "--pid", "4294967296"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, FlagPid_MissingArgument_ReturnsFalse) {
  Argv av{"psa", "--pid"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, FlagDetails_FlagAsArg_ReturnsFalse) {
  Argv av{"psa", "-o", "-a", "--details"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}

TEST_F(PsaCliTest, FlagK_OptionAsArg_ReturnsFalse) {
  Argv av{"psa", "-k", "--filter-param", "micro_av"};
  EXPECT_FALSE(run(av));
  EXPECT_TRUE(fake.calls.empty());
}
