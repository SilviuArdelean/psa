/*
*	Processes Status Analysis - psa
*/

#include "general.h"
#include "string_utils.h"
#include "ProcessingOperations.h"

#ifdef _WIN32
	#include "psa-win/XGetopt.h"
	#include "tchar.h"
#endif

#ifdef __linux__
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
#endif

void showParameters()
{
	ucout << _T("	-a	: list all processes information") << std::endl;
	ucout << _T("	-e [no]	: top [no] most expensive memory consuming processes | top 10 by default ") << std::endl;
	ucout << _T("	-k  : kill specific process by PID or name") << std::endl;
	ucout << _T("	-o	: info only one process name criteria ") << std::endl;
	ucout << _T("	-t	: tree snapshot of current processes") << std::endl;
}

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

	ucout << _T("-----------------------------------------------------------") << std::endl;
	ucout << _T("       Processes Status Analysis - version 0.3") << std::endl;
	ucout << _T("-----------------------------------------------------------") << std::endl;
	
	showParameters();

	ucout << _T("-----------------------------------------------------------") << std::endl;
	ucout << _T("  Author: Silviu-Marius Ardelean http://silviuardelean.ro ") << std::endl;
	ucout << _T("-----------------------------------------------------------") << std::endl;
}

bool processCommandLine(int argc, TCHAR *argv[], ProcessingOperations *pPO)
{	
	if (!pPO) 
	{
		ucout << _T("Internal error: ") << PSA_INTERNAL_ERRORS::invalid_processing_operations;
		return false;
	}

	int opt = 0;
	bool good_params = false;
	short loop_params = 0;

	while ((opt = getopt(argc, argv, _T("aekotAEKOT"))) != EOF)
	{
#ifdef _WIN32
		auto option = tolower(opt);
#else
		auto option = opt;
#endif

		switch (option)
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
					if (argc < 3)
					{
						ucout << _T(" Invalid set of parameters. Please specify the PID or the name of the process to be killed. \n");
						return false;
					}

					TCHAR *secondParam = argv[argc - 1];
					pPO->killProcesses(secondParam);
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
#ifdef _WIN32
					auto proc_pid = 0;  // Windows ROOT PID = 0
#else	
					auto proc_pid = 1;	// Linux   ROOT PID = 1
#endif
					if (3 == argc && isdigit(*argv[argc - 1]))
						proc_pid = utoi(argv[argc - 1]);

					pPO->generateProcessesTree(proc_pid);
				}
			break;
			
			case _T('?'):
				showAvailableInformation();
				return true;
				break;	
			default:
				ucout << _T(" psa: invalid option ...") << std::endl;
				ucout << _T(" Please check the available list of parameters : \n");
				showParameters();
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