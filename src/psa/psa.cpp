/*
*	Processes Status Analysis - psa
*/

#include "general.h"
#include "..\XGetopt.h"
#include "stdlib.h"
#include "string_utils.h"
#include "..\ProcessingOperations.h"
#include "tchar.h"

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

	std::wcout << _T("  Processes Status Analysis - version 0.1\n");
	std::wcout << _T("------------------------------------------- \n");
	std::wcout << _T("-a	: list all processes information \n");
	std::wcout << _T("-e [no]	: top [no] most expensive memory consuming processes | top 10 by default \n");
	std::wcout << _T("-k	: kill specific process by PID \n");
	std::wcout << _T("-o	: info only one process name criteria \n");
	std::wcout << _T("-t	: tree snapshot of current processes\n");
}

bool processCommandLine(int argc, TCHAR *argv[], ProcessingOperations *pPO)
{	
	if (!pPO) 
	{
		std::wcout << _T("Internal error: ") << PSA_INTERNAL_ERRORS::invalid_processing_operations;

		return false;
	}

	int opt = 0;
	bool good_params = false;
	short loop_params = 0;

	while ((opt = getopt(argc, argv, _T("aekotAEKOT"))) != EOF)
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
					short top = _ttoi(argv[argc - 1]);
					if (top == 0) top = 10;

					pPO->printTopExpensiveProcesses(top);
				}
			break;

			case _T('k'):
				{
					std::wcout << _T("-k: kill specific process by PID \n");
					std::wcout << _T("	(feature not implemented yet) \n");
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
					DWORD proc_pid = 0;
					if (3 == argc && isdigit(*argv[argc - 1]))
						proc_pid = _ttoi(argv[argc - 1]);

					pPO->generateProcessesTree(proc_pid);
				}
			break;
			
			case _T('?'):
			default:
				showAvailableInformation();
				return FALSE;
				break;
		}

		loop_params++;
		good_params = true;
	}

	if (!good_params)
	{
		showAvailableInformation();
	}

	return TRUE;
}

int _tmain(int argc, TCHAR* argv[])
{
	ProcessingOperations po;
	processCommandLine(argc, argv, &po);

	return 0;
}