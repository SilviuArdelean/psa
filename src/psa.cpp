/*
 *    Processes Status Analysis - psa
 */

#include "general.h"
#include "operations.h"
#include "string_utils.h"

#ifdef _WIN32
#include "psa-win/XGetopt.h"
#include "tchar.h"
#endif

#ifdef __linux__
#include <cstdio>
#include <unistd.h>
#endif

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
  ucout << _T("    -t    : tree snapshot of current processes") << std::endl;
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

  ucout << _T("-----------------------------------------------------------")
        << std::endl;
  ucout << _T("       Processes Status Analysis - version 0.3") << std::endl;
  ucout << _T("-----------------------------------------------------------")
        << std::endl;

  ShowParameters();

  ucout << _T("-----------------------------------------------------------")
        << std::endl;
  ucout << _T("  Author: Silviu-Marius Ardelean http://silviuardelean.ro ")
        << std::endl;
  ucout << _T("-----------------------------------------------------------")
        << std::endl;
}

// Validates that optarg is not itself a flag (e.g. psa -o -d avast).
// Returns the search term on success, or empty string + prints an error on
// failure.
static ustring resolve_process_arg(const TCHAR* flag, const TCHAR* optarg_val) {
  ustring searchfor = (optarg_val != nullptr) ? optarg_val : _T("");
  if (searchfor.empty()) {
    ucout << _T("Error: missing argument for -") << flag << _T(".")
          << std::endl;
    return _T("");
  }
  if (searchfor[0] == _T('-')) {
    ucout << _T("Error: missing argument for -") << flag << _T(".")
          << std::endl;
    ucout << _T("       Did you mean: psa ") << searchfor << _T(" <name|pid> ?")
          << std::endl;
    return _T("");
  }
  return searchfor;
}

bool ProcessCommandLine(int argc, TCHAR* argv[], ProcessingOperations* pPO) {
  if (!pPO) {
    ucout << _T("Internal error: ")
          << PSA_INTERNAL_ERRORS::invalid_processing_operations;
    return false;
  }

  int opt = 0;
  bool good_params = false;

  // optstring: 'k', 'o' and 'd' require arguments.
  // 'e' has an optional number — handled manually by peeking at argv[optind].
  while ((opt = getopt(argc, argv, _T("aek:o:d:tAEK:O:D:T"))) != EOF) {
#ifdef _WIN32
    auto option = tolower(opt);
#else
    auto option = opt;
#endif

    switch (option) {
      case _T('a'): {
        if (!pPO->PrintAllProcessesInformation())
          return false;
      } break;

      case _T('e'): {
        // -e is optional: peek at argv[optind] — consume it only if it's a
        // number.
        int top = 10;
        if (optind < argc && string_utils::is_number(argv[optind]))
          top = utoi(argv[optind++]);
        if (top == 0)
          top = 10;

        pPO->PrintTopExpensiveProcesses(top);
      } break;

      case _T('k'): {
        const auto searchfor = resolve_process_arg(_T("k"), optarg);
        if (searchfor.empty())
          return false;
        pPO->KillProcesses(searchfor.c_str());
      } break;

      case _T('o'): {
        const auto searchfor = resolve_process_arg(_T("o"), optarg);
        if (searchfor.empty())
          return false;
        if (!pPO->PrintProcessInformation(searchfor))
          return false;
      } break;

      case _T('d'): {
        const auto searchfor = resolve_process_arg(_T("d"), optarg);
        if (searchfor.empty())
          return false;
        if (!pPO->PrintProcessInformation(searchfor, true))
          return false;
      } break;

      case _T('t'): {
#ifdef _WIN32
        auto proc_pid = 0;  // Windows ROOT PID = 0
#else
        auto proc_pid = 1;  // Linux   ROOT PID = 1
#endif
        if (3 == argc && isdigit(*argv[argc - 1]))
          proc_pid = utoi(argv[argc - 1]);

        pPO->GenerateProcessesTree(proc_pid);
      } break;

      case _T('?'):
        ShowAvailableInformation();
        return (optind > 1 && argv[optind - 1][0] == _T('-') &&
                argv[optind - 1][1] == _T('?'));
      default:
        ucout << _T(" psa: invalid option ...") << std::endl;
        ucout << _T(" Please check the available list of parameters : \n");
        ShowParameters();
        return false;
        break;
    }

    good_params = true;
  }

  if (!good_params) {
    ShowAvailableInformation();
    return false;
  }

  return true;
}

#ifdef _WIN32
int _tmain(int argc, TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{
  ProcessingOperations po;
  return ProcessCommandLine(argc, argv, &po) ? 0 : 1;
}
