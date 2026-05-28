
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
#include "string_utils.h"

void ShowParameters() {
  ucout << _T("    -a    : list all processes information") << std::endl;
  ucout << _T("    -e [no]    : top [no] most expensive memory consuming ")
           _T("processes ")
           _T("| top 10 by default ")
        << std::endl;
  ucout << _T("    -k <name|pid> : kill specific process by PID or name")
        << std::endl;
  ucout << _T("    -k <name|pid> --cmdline <text> : kill by name or PID, ")
           _T("filtered by command line substring")
        << std::endl;
  ucout << _T("    -o <name|pid> : info only one process name criteria ")
        << std::endl;
  ucout << _T("    -d <name|pid> : process details with command line for ")
           _T("matching process(es)")
        << std::endl;
  ucout << _T("    -t [pid] : tree snapshot of current processes") << std::endl;
  ucout << _T("    -h    : show available options") << std::endl;
}

void ShowAvailableInformation() {
  // a = list all processes information
  // e = top [no] most expensive memory consuming processes | top 10 by default
  // k = kill by process PID or name, with optional command line filter
  // o = info only one process name criteria
  // d = show command line details per process (modifier for -o)
  // t = tree snapshot of current processes
  // nice to have
  // chose stream (iostream / fstream)
  // specify additional pid as root to build the tree

  ucout << _T("----------------------------------------------------------")
        << std::endl;
  ucout << _T("     psa - Processes Status Analysis - version 0.4")
        << std::endl;
  ucout << _T("----------------------------------------------------------")
        << std::endl;

  ShowParameters();

  ucout << _T("----------------------------------------------------------")
        << std::endl;
  ucout << _T("    Author: Silviu-Marius Ardelean https://ardelean.ch ")
        << std::endl;
  ucout << _T("----------------------------------------------------------")
        << std::endl;
}

// Converts a narrow UTF-8 string (from cxxopts) to the project's native string
// type. On Windows Unicode builds this widens to UTF-16; on Linux it's a no-op.
static ustring to_ustring(const std::string& s) {
#ifdef _WIN32
  // argv is in the current code page (ACP), not UTF-8
  int wlen = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
  if (wlen > 1) {
    std::wstring ws(wlen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &ws[0], wlen);
    ws.resize(wlen - 1);  // remove null terminator
    return ws;
  } else {
    return std::wstring();
  }
#else
  return s;
#endif
}

bool ProcessCommandLine(int argc, char* argv[], ProcessingOperations* pPO) {
  // Backward compatibility: preserve support for space-separated forms (e.g.,
  // -e 5, -t 1234) and accept upper-case flags on Windows by mapping them to
  // lowercase.
  // Special-case: if any argument is exactly "-?", show help and return true
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "-?") == 0) {
      ShowAvailableInformation();
      return true;
    }
  }
  if (!pPO) {
    ucout << _T("Internal error: ")
          << PSA_INTERNAL_ERRORS::invalid_processing_operations;
    return false;
  }

  // cxxopts does not consume a space-separated token for implicit-value options
  // (e.g. "-e 5" or "-t 1234"). Pre-process argv to merge such pairs into the
  // "=" form ("-e=5", "-t=1234") so the original calling convention is
  // preserved. Additionally, on Windows, map upper-case flags to lower-case for
  // backward compatibility.
  static const auto is_all_digits = [](const char* s) {
    if (!s || !*s)
      return false;
    while (*s) {
      if (!isdigit(static_cast<unsigned char>(*s++)))
        return false;
    }
    return true;
  };
  std::vector<std::string> cooked_storage;
  std::vector<const char*> cooked_argv;
  cooked_storage.reserve(argc);
  cooked_argv.reserve(argc);
  for (int i = 0; i < argc; ++i) {
    std::string a = argv[i];
#ifdef _WIN32
    // Map upper-case flag to lower-case for aliased options
    if (a.size() == 2 && a[0] == '-' && std::strchr("ADEKOT", a[1])) {
      a[1] = static_cast<char>(tolower(a[1]));
    }
#endif
    if ((a == "-e" || a == "-t") && i + 1 < argc &&
        is_all_digits(argv[i + 1])) {
      // implicit_value options can't consume space-separated tokens as short
      // opts. Use the long-option "=" form which cxxopts handles correctly.
      std::string longform = (a == "-e") ? "--entries=" : "--tree=";
      cooked_storage.push_back(longform + argv[i + 1]);
      ++i;
    } else {
      cooked_storage.push_back(std::move(a));
    }
  }
  for (const auto& s : cooked_storage)
    cooked_argv.push_back(s.c_str());
  const int cooked_argc = static_cast<int>(cooked_argv.size());

  cxxopts::Options options("psa", "Processes Status Analysis");
  options.add_options()("a", "List all processes information")(
      "e,entries",
      "Top [no] most expensive memory consuming processes (default: 10)",
      cxxopts::value<int>()->implicit_value("10"),
      "[no]")("k", "Kill specific process by PID or name",
              cxxopts::value<std::string>(),
              "<name|pid>")("cmdline", "Command line substring filter for kill",
                            cxxopts::value<std::string>(), "<text>")(
      "o", "Info for one process matching name criteria",
      cxxopts::value<std::string>(), "<name|pid>")(
      "d", "Process details with command line for matching process(es)",
      cxxopts::value<std::string>(), "<name|pid>")
#ifdef _WIN32
      ("t,tree", "Tree snapshot of current processes",
       cxxopts::value<int>()->implicit_value("0"),
       "[pid]")  // Windows ROOT PID = 0
#else
      ("t,tree", "Tree snapshot of current processes",
       cxxopts::value<int>()->implicit_value("1"),
       "[pid]")  // Linux ROOT PID = 1
#endif
      ("h,help", "Show available options");

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

  bool good_params = false;

  if (result.count("a")) {
    if (!pPO->PrintAllProcessesInformation())
      return false;
    good_params = true;
  }

  if (result.count("e")) {
    int top = result["e"].as<int>();
    if (top <= 0)
      top = 10;
    pPO->PrintTopExpensiveProcesses(top);
    good_params = true;
  }

  if (result.count("k")) {
    const std::string& val = result["k"].as<std::string>();
    if (!val.empty() && val[0] == '-') {
      // Reject flag-looking argument as value
      return false;
    }
    const auto searchfor = to_ustring(val);
    ustring cmdline_filter;
    if (result.count("cmdline")) {
      cmdline_filter = to_ustring(result["cmdline"].as<std::string>());
    }
    // TODO: Update KillProcesses to accept cmdline_filter in next step
    pPO->KillProcesses(searchfor.c_str(), cmdline_filter);
    good_params = true;
  }

  if (result.count("o")) {
    const std::string& val = result["o"].as<std::string>();
    if (!val.empty() && val[0] == '-') {
      // Reject flag-looking argument as value
      return false;
    }
    const auto searchfor = to_ustring(val);
    if (!pPO->PrintProcessInformation(searchfor))
      return false;
    good_params = true;
  }

  if (result.count("d")) {
    const std::string& val = result["d"].as<std::string>();
    if (!val.empty() && val[0] == '-') {
      // Reject flag-looking argument as value
      return false;
    }
    const auto searchfor = to_ustring(val);
    if (!pPO->PrintProcessInformation(searchfor, true))
      return false;
    good_params = true;
  }

  if (result.count("t")) {
    pPO->GenerateProcessesTree(result["t"].as<int>());
    good_params = true;
  }

  if (!good_params) {
    ShowAvailableInformation();
    return false;
  }

  return true;
}

#ifndef PSA_TEST_BUILD
int main(int argc, char** argv) {
  ProcessingOperations po;
  return ProcessCommandLine(argc, argv, &po) ? 0 : 1;
}
#endif  // PSA_TEST_BUILD
