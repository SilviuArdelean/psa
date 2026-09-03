
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
 *
 *    Processes Status Analysis - psa
 */

#include "cxxopts.hpp"
#include "operations.h"
#include "pch.h"
#include "psa_cli_parsing.h"

void ShowAvailableInformation();
void ShowParameters();
static ustring to_ustring(const std::string& s);

namespace {

bool try_parse_positive_pid(const std::string& value, uint32_t& pid) {
  if (value.empty()) {
    return false;
  }

  uint64_t parsed = 0;
  for (const unsigned char ch : value) {
    if (!std::isdigit(ch)) {
      return false;
    }

    parsed = (parsed * 10) + static_cast<uint64_t>(ch - '0');
    if (parsed > static_cast<uint64_t>(UINT32_MAX)) {
      return false;
    }
  }

  if (parsed == 0) {
    return false;
  }

  pid = static_cast<uint32_t>(parsed);
  return true;
}

bool try_get_cmdline_filter(const cxxopts::ParseResult& result,
                            ustring& cmdline_filter) {
  const std::string& cmdline_val = result["filter-param"].as<std::string>();
  if (cmdline_val.empty() || cli_parsing::is_flag_like_value(cmdline_val)) {
    ucout << _T("Error: option --filter-param requires a non-empty, ")
             _T("non-flag value.")
          << std::endl;
    ucout << _T("More information:") << std::endl;
    ShowParameters();
    return false;
  }

  cmdline_filter = to_ustring(cmdline_val);
  return true;
}

bool try_get_notification_title(const cxxopts::ParseResult& result,
                                std::string& title) {
  const std::string& title_value = result["notify-title"].as<std::string>();
  if (title_value.empty() || cli_parsing::is_flag_like_value(title_value)) {
    ucout << _T("Error: option --notify-title requires a non-empty, ")
             _T("non-flag value.")
          << std::endl;
    ucout << _T("More information:") << std::endl;
    ShowParameters();
    return false;
  }

  title = title_value;
  return true;
}

cxxopts::Options create_options() {
  cxxopts::Options options("psa", "Processes Status Analysis");
  options.add_options()("a", "List all processes information")(
      "e,entries",
      "Top [no] most expensive memory consuming processes (default: 10)",
      cxxopts::value<int>()->implicit_value("10"),
      "[no]")("k", "Kill specific process by PID or name",
              cxxopts::value<std::string>(),
              "<name|pid>")("o", "Info for one process matching name criteria",
                            cxxopts::value<std::string>(), "<name|pid>")(
      "pid", "Identify process information by PID",
      cxxopts::value<std::string>(),
      "<pid>")("d,details", "Show detailed process information (used with -o)")
#ifdef _WIN32
      ("t,tree", "Tree snapshot of current processes",
       cxxopts::value<int>()->implicit_value("0"), "[pid]")
#else
      ("t,tree", "Tree snapshot of current processes",
       cxxopts::value<int>()->implicit_value("1"), "[pid]")
#endif
          ("filter-param",
           "Filter processes by command line substring (used with -k or -o)",
           cxxopts::value<std::string>(), "<value>")(
      "notify",
      "Show a native system notification with an optional custom message",
      cxxopts::value<std::string>()->implicit_value("psa notification"),
      "[message]")(
      "notify-title",
      "Customize the notification title (requires --notify with a message)",
      cxxopts::value<std::string>(),
      "<title>")("h,help", "Show available options");
  return options;
}

bool apply_process_filter(const cxxopts::ParseResult& result,
                          const ustring& filter,
                          bool show_details,
                          ProcessingOperations* po) {
  if (result.count("filter-param")) {
    ustring cmdline_filter;
    if (!try_get_cmdline_filter(result, cmdline_filter)) {
      return false;
    }

    return show_details ? po->PrintProcessInformationWithCmdlineFilter(
                              filter, cmdline_filter, true)
                        : po->PrintProcessInformationWithCmdlineFilter(
                              filter, cmdline_filter);
  } else {
    return show_details ? po->PrintProcessInformation(filter, true)
                        : po->PrintProcessInformation(filter);
  }
}

bool handle_filter_option(const cxxopts::ParseResult& result,
                          const char* key,
                          bool show_details,
                          ProcessingOperations* po,
                          bool& good_params) {
  if (!result.count(key)) {
    return true;
  }

  const std::string& value = result[key].as<std::string>();
  if (cli_parsing::is_flag_like_value(value)) {
    ucout << _T("Error: option -") << to_ustring(key)
          << _T(" requires a non-flag value.") << std::endl
          << _T("More information:") << std::endl;
    ShowParameters();
    return false;
  }

  const auto filter = to_ustring(value);
  if (!apply_process_filter(result, filter, show_details, po)) {
    return false;
  }

  good_params = true;
  return true;
}

bool handle_kill_option(const cxxopts::ParseResult& result,
                        ProcessingOperations* processing_operations,
                        bool& good_params) {
  if (!result.count("k")) {
    return true;
  }

  const std::string& value = result["k"].as<std::string>();
  if (cli_parsing::is_flag_like_value(value)) {
    ucout << _T("Error: option -k requires a non-flag value.") << std::endl;
    ucout << _T("More information:") << std::endl;
    ShowParameters();
    return false;
  }

  const auto filter = to_ustring(value);
  if (result.count("filter-param")) {
    ustring cmdline_filter;
    if (!try_get_cmdline_filter(result, cmdline_filter)) {
      return false;
    }

    processing_operations->KillProcessesWithCmdLine(filter, cmdline_filter);
  } else {
    processing_operations->KillProcesses(filter.c_str());
  }
  good_params = true;
  return true;
}

void handle_entries_option(const cxxopts::ParseResult& result,
                           ProcessingOperations* processing_operations,
                           bool& good_params) {
  if (!result.count("e")) {
    return;
  }

  int top = result["e"].as<int>();
  if (top <= 0)
    top = 10;

  processing_operations->PrintTopExpensiveProcesses(top);
  good_params = true;
}

bool dispatch_requested_options(const cxxopts::ParseResult& result,
                                bool notify_has_explicit_message,
                                ProcessingOperations* processing_operations) {
  if (result.count("filter-param") && !result.count("k") &&
      !result.count("o")) {
    ucout << _T("Error: --filter-param can only be used with -k or -o.")
          << std::endl;
    ShowParameters();
    return false;
  }

  if (result.count("details") && !result.count("o")) {
    ucout << _T("Error: --details can only be used with -o.") << std::endl;
    ShowParameters();
    return false;
  }

  if (result.count("notify-title") && !result.count("notify")) {
    ucout << _T("Error: --notify-title can only be used with --notify.")
          << std::endl;
    ShowParameters();
    return false;
  }

  if (result.count("notify-title") && !notify_has_explicit_message) {
    ucout << _T("Error: --notify-title requires --notify to include an ")
             _T("explicit message.")
          << std::endl;
    ShowParameters();
    return false;
  }

  bool good_params = false;

  if (result.count("a")) {
    if (!processing_operations->PrintAllProcessesInformation())
      return false;
    good_params = true;
  }

  handle_entries_option(result, processing_operations, good_params);

  if (!handle_kill_option(result, processing_operations, good_params)) {
    return false;
  }

  const bool show_details = result.count("details") > 0;
  if (!handle_filter_option(result, "o", show_details, processing_operations,
                            good_params)) {
    return false;
  }

  if (result.count("pid")) {
    const std::string& pid_value = result["pid"].as<std::string>();
    uint32_t pid = 0;
    if (!try_parse_positive_pid(pid_value, pid)) {
      ucout << _T("Error: PID must be a positive integer.") << std::endl;
      return false;
    }

    auto details = processing_operations->GetProcessInfoByPid(pid);
    if (!details.has_value()) {
      ucout << _T("Error: Process with PID ") << pid << _T(" not found.")
            << std::endl;
      return false;
    }

    processing_operations->PrintProcessInfoReport(details.value());
    good_params = true;
  }

  if (result.count("t")) {
    processing_operations->GenerateProcessesTree(result["t"].as<int>());
    good_params = true;
  }
  if (result.count("notify")) {
    std::string notification_title = "psa";
    if (result.count("notify-title") &&
        !try_get_notification_title(result, notification_title)) {
      return false;
    }

    if (!processing_operations->ShowNotification(
            result["notify"].as<std::string>(), notification_title)) {
      ucout << _T("Warning: Failed to show notification.") << std::endl;
    }
    good_params = true;
  }
  if (!good_params) {
    ShowAvailableInformation();
    return false;
  }

  return true;
}

}  // namespace

void ShowParameters() {
  const auto print_parameter_line = [](const ustring& label,
                                       const ustring& description) {
    ucout << psa::format(_T("    {:<36} : {}"), label, description)
          << std::endl;
  };

  print_parameter_line(_T("-a"), _T("list all processes information"));
  print_parameter_line(
      _T("-e [no]"),
      _T("top [no] most expensive memory consuming processes | top 10 by ")
      _T("default"));
  print_parameter_line(
      _T("-k <name|pid> [--filter-param <value>]"),
      _T("kill specific process by PID or name, optionally filtering by ")
      _T("command line parameter"));
  print_parameter_line(
      _T("-o <name|pid> [-d|--details] [--filter-param <value>]"),
      _T("info for process(es) matching name/PID, optionally with details ")
      _T("and command line filter"));
  print_parameter_line(_T("--pid <pid>"),
                       _T("identify process information by PID (full process ")
                       _T("report)"));
  print_parameter_line(_T("-d, --details"),
                       _T("show detailed process information (used with -o)"));
  print_parameter_line(
      _T("--filter-param <value>"),
      _T("filter by command line substring (used with -k or -o)"));
  print_parameter_line(
      _T("-t [pid]"),
      _T("tree snapshot of current processes or of the subprocesses of a ")
      _T("specified process PID"));
  print_parameter_line(
      _T("--notify [message]"),
      _T("show a native system notification with an optional custom ")
      _T("message"));
  print_parameter_line(_T("--notify-title <title>"),
                       _T("customize the notification title (used with ")
                       _T("--notify <message>)"));
  print_parameter_line(_T("-h"), _T("show available options"));
}

void ShowAvailableInformation() {
  ucout
      << _T("----------------------------------------------------------------")
      << std::endl;
  ucout << _T("        psa - Processes Status Analysis - version 0.4")
        << std::endl;
  ucout
      << _T("----------------------------------------------------------------")
      << std::endl;

  ShowParameters();

  ucout
      << _T("----------------------------------------------------------------")
      << std::endl;
  ucout << _T("       Author: Silviu-Marius Ardelean https://ardelean.ch ")
        << std::endl;
  ucout
      << _T("----------------------------------------------------------------")
      << std::endl;
}

// Convert parsed option values to the project's string type.
static ustring to_ustring(const std::string& s) {
#ifdef _WIN32
  int wlen = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
  if (wlen > 1) {
    std::wstring ws(wlen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &ws[0], wlen);
    ws.resize(wlen - 1);
    return ws;
  } else {
    return std::wstring();
  }
#else
  return s;
#endif
}

bool ProcessCommandLine(int argc, char* argv[], ProcessingOperations* pPO) {
  // Handle legacy help switch before parsing.
  if (cli_parsing::has_legacy_help_switch(argc, argv)) {
    ShowAvailableInformation();
    return true;
  }

  if (!pPO) {
    ucout << _T("Internal error: ")
          << static_cast<int>(
                 PSA_INTERNAL_ERRORS::invalid_processing_operations);
    return false;
  }

  auto cooked_storage = cli_parsing::normalize_arguments(argc, argv);
  auto cooked_argv = cli_parsing::build_argv_view(cooked_storage);
  const int cooked_argc = static_cast<int>(cooked_argv.size());
  const bool notify_has_explicit_message =
      cli_parsing::has_explicit_option_value(cooked_storage, "--notify");

  auto options = create_options();

  cxxopts::ParseResult result;
  try {
    result = options.parse(cooked_argc, cooked_argv.data());
  } catch (const cxxopts::exceptions::exception& e) {
    ucout << _T("Error: ") << to_ustring(e.what()) << std::endl;
    ShowParameters();
    return false;
  }

  if (result.count("help")) {
    ShowAvailableInformation();
    return true;
  }

  return dispatch_requested_options(result, notify_has_explicit_message, pPO);
}

#ifndef PSA_TEST_BUILD
int main(int argc, char** argv) {
  ProcessingOperations po;
  return ProcessCommandLine(argc, argv, &po) ? 0 : 1;
}
#endif  // PSA_TEST_BUILD
