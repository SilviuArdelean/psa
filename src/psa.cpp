/*
 *    Processes Status Analysis - psa
 */

#include "pch.h"
#include "operations.h"
#include "string_utils.h"
#include "cxxopts.hpp"

void ShowParameters() {
  ucout << _T("    -a    : list all processes information") << std::endl;
  ucout << _T("    -e [no]    : top [no] most expensive memory consuming ")
           _T("processes ")
           _T("| top 10 by default ")
        << std::endl;
  ucout << _T("    -k <name|pid> : kill specific process by PID or name")
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
  // k = kill by process PID
  // o = info only one process name criteria
  // d = show command line details per process (modifier for -o)
  // t = tree snapshot of current processes
  // nice to have
  // chose stream (iostream / fstream)
  // specify additional pid as root to build the tree

  ucout << _T("----------------------------------------------------------")
        << std::endl;
  ucout << _T("     psa - Processes Status Analysis - version 0.4") << std::endl;
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
  return string_utils::utf8ToUtf16(s);
#else
  return s;
#endif
}

bool ProcessCommandLine(int argc, char* argv[], ProcessingOperations* pPO) {
    // Special-case: if any argument is exactly "-?", show help and return true (backward compatibility)
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
  // "=" form ("-e=5", "-t=1234") so the original calling convention is preserved.
  // Additionally, on Windows, map upper-case flags to lower-case for backward compatibility.
  static const auto is_all_digits = [](const char* s) {
    if (!s || !*s) return false;
    while (*s) { if (!isdigit(static_cast<unsigned char>(*s++))) return false; }
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
    if ((a == "-e" || a == "-t") && i + 1 < argc && is_all_digits(argv[i + 1])) {
      // implicit_value options can't consume space-separated tokens as short opts.
      // Use the long-option "=" form which cxxopts handles correctly.
      std::string longform = (a == "-e") ? "--entries=" : "--tree=";
      cooked_storage.push_back(longform + argv[i + 1]);
      ++i;
    } else {
      cooked_storage.push_back(std::move(a));
    }
  }
  for (const auto& s : cooked_storage) cooked_argv.push_back(s.c_str());
  const int cooked_argc = static_cast<int>(cooked_argv.size());

  cxxopts::Options options("psa", "Processes Status Analysis");
  options.add_options()
    ("a", "List all processes information")
    ("e,entries", "Top [no] most expensive memory consuming processes (default: 10)",
      cxxopts::value<int>()->implicit_value("10"), "[no]")
    ("k", "Kill specific process by PID or name",
      cxxopts::value<std::string>(), "<name|pid>")
    ("o", "Info for one process matching name criteria",
      cxxopts::value<std::string>(), "<name|pid>")
    ("d", "Process details with command line for matching process(es)",
      cxxopts::value<std::string>(), "<name|pid>")
#ifdef _WIN32
    ("t,tree", "Tree snapshot of current processes",
      cxxopts::value<int>()->implicit_value("0"), "[pid]")  // Windows ROOT PID = 0
#else
    ("t,tree", "Tree snapshot of current processes",
      cxxopts::value<int>()->implicit_value("1"), "[pid]")  // Linux ROOT PID = 1
#endif
    ("h,help", "Show available options")
  ;

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
    if (top == 0)
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
    pPO->KillProcesses(searchfor.c_str());
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
