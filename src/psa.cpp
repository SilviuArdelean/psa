/*
*	Processes Status Analysis - psa
*/

#include "general.h"
#include "string_utils.h"
#include "ProcessingOperations.h"
#ifdef _WIN32
#include "XGetopt.h"
#include "tchar.h"
#endif

#ifdef __linux__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif

void showAvailableInformation()
{
	// a = list all processes information 
	// e = top [no] most expensive memory consuming processes | top 10 by default
	// k = kill by process PID
	// o = info only one process name criteria
	// t = tree snapshot of current processes
			// nice to have
			// chose stream (iostream / fstream) 
			// specify additional pid as root to build the tree


	ucout << _T("  Processes Status Analysis - version 0.1\n");
	ucout << _T("------------------------------------------- \n");
	ucout << _T("-a	: list all processes information \n");
	ucout << _T("-e [no]	: top [no] most expensive memory consuming processes | top 10 by default \n");
	ucout << _T("-k	: kill specific process by PID \n");
	ucout << _T("-o	: info only one process name criteria \n");
	ucout << _T("-t	: tree snapshot of current processes\n");
}

#ifdef _WIN32
bool processCommandLine(int argc, TCHAR *argv[], ProcessingOperations *pPO)
#else
bool processCommandLine(int argc, char *argv[], ProcessingOperations *pPO)
#endif
{	
	if (!pPO) 
	{
		ucout << _T("Internal error: ") << PSA_INTERNAL_ERRORS::invalid_processing_operations;
		return false;
	}

	int opt = 0;
	bool good_params = false;
	short loop_params = 0;
#ifdef _WIN32
	while ((opt = getopt(argc, argv, _T("aekotAEKOT"))) != EOF)
#elif __linux__
	while ((opt = getopt(argc, argv, _T("aekotAEKOT"))) != EOF)
#endif
	{
		switch (tolower(opt))
		{
			case _T('a'):
				{
					pPO->printAllProcessesInformation();
				}
			break;

			case _T('e'):
				{
					auto top = utoi(argv[argc - 1]);
					if (top == 0) top = 10;

					pPO->printTopExpensiveProcesses(top);
				}
			break;

			case _T('k'):
				{
					ucout << _T("-k: kill specific process by PID \n");
					ucout << _T("	(feature not implemented yet) \n");
				}
			break;

			case _T('o'):
				{
					ustring searchfor = (loop_params + 2 < argc) ? argv[loop_params+2] : argv[argc-1];
					pPO->printProcessInformation(searchfor);
				}
			break;


			case _T('t'):
				{
					auto proc_pid = 0;
					if (3 == argc && isdigit(*argv[argc - 1]))
						proc_pid = utoi(argv[argc - 1]);

					pPO->generateProcessesTree(proc_pid);
				}
			break;
			
			case _T('?'):
			default:
				showAvailableInformation();
				return false;
				break;
		}

		loop_params++;
		good_params = true;
	}

	if (!good_params)
	{
		showAvailableInformation();
		return false;
	}

	return true;
}

#ifdef _WIN32
int _tmain(int argc, TCHAR* argv[])
#else
int main(int argc, char **argv)
#endif
{
	ProcessingOperations po;
	processCommandLine(argc, argv, &po);

	return 0;
}