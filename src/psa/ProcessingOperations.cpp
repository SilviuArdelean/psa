#include "stdafx.h"
#include "general.h"
#include "ProcessingOperations.h"
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include "string_utils.h"
#include "ProcsTreeBuilder.h"
#include "Winternl.h"

using namespace std;

ProcessingOperations::ProcessingOperations(void)
{
}

ProcessingOperations::~ProcessingOperations(void)
{
}

BOOL ProcessingOperations::SetPrivilege(
	   HANDLE hToken,          // access token handle
	   LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	   BOOL bEnablePrivilege   // to enable or disable privilege
	   ) 
{
   TOKEN_PRIVILEGES tp;
   LUID luid;

   if ( !LookupPrivilegeValue(  NULL,            // lookup privilege on local system
								  lpszPrivilege,   // privilege to lookup 
								  &luid ) )        // receives LUID of privilege
   {
      printf("LookupPrivilegeValue error: %u\n", GetLastError() ); 
      return FALSE; 
   }

   tp.PrivilegeCount = 1;
   tp.Privileges[0].Luid = luid;
   tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;

   // Enable the privilege or disable all privileges.

   if ( !AdjustTokenPrivileges(
      hToken, 
      FALSE, 
      &tp, 
      sizeof(TOKEN_PRIVILEGES), 
      (PTOKEN_PRIVILEGES) NULL, 
      (PDWORD) NULL) )
   { 
      printf("AdjustTokenPrivileges error: %u\n", GetLastError() ); 
      return FALSE; 
   } 

   if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
   {
      printf("The token does not have the specified privilege. \n");
      return FALSE;
   } 

   CloseHandle(hToken);

   return TRUE;
}

BOOL ProcessingOperations::BuildProcessesMap()
{
   PROCESSENTRY32 pe32;

   // Take a snapshot of all processes in the system.
   HANDLE hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
   if( hProcessSnap == INVALID_HANDLE_VALUE )
   {
      printError( TEXT("CreateToolhelp32Snapshot (of processes)") );
      return( FALSE );
   }

   // Set the size of the structure before using it.
   pe32.dwSize = sizeof( PROCESSENTRY32 );

   // Retrieve information about the first process,
   // and exit if unsuccessful
   if( !Process32First( hProcessSnap, &pe32 ) )
   {
      printError( TEXT("Retrieve information about the first process has failed") ); // show cause of failure
      CloseHandle( hProcessSnap );          // clean the snapshot object
      return( FALSE );
   }

   m_mapProcesses.clear();

   do
   {
		 FILETIME  crtProcCreationTime, crtProcExitTime, crtProcKernelTime, crtProcUserTime;
	
     
		  HANDLE hCrtProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID );
		  if( nullptr != hCrtProcess )
		  {
			 ::GetProcessTimes( hCrtProcess, &crtProcCreationTime, &crtProcExitTime, &crtProcKernelTime, &crtProcUserTime );
			 CloseHandle( hCrtProcess );
		  }

		  HANDLE hParentProc = ::OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ParentProcessID );
		  if ( nullptr != hParentProc)
		  {
			 FILETIME  parentCreationTime, parentExitTime, parentKernelTime, parentUserTime;
			 GetProcessTimes( hParentProc, &parentCreationTime, &parentExitTime, &parentKernelTime, &parentUserTime );
		   

			 CloseHandle( hParentProc );
	 
			// get current time in FILETIME format
			//FILETIME crtSysTime;
			//::GetSystemTimeAsFileTime(&crtSysTime);

			// if the the parentProcess is terminated and the detected th32ParentProcessID is not a valid one
			// if ( ::CompareFileTime( &crtProcCreationTime, &parentCreationTime ) > 0 
			//	// ||  ::CompareFileTime( &crtProcCreationTime, &crtSysTime ) > 0 
			//	 )
			//{
			//	pe32.th32ParentProcessID = 0;
			//}
		  }

		  proc_info pi(pe32.th32ProcessID, 
						  pe32.th32ParentProcessID,
						  pe32.szExeFile);

		  m_mapProcesses.insert(pair<DWORD, proc_info>(pe32.th32ProcessID, pi));

	   } while( Process32Next( hProcessSnap, &pe32 ) );

   CloseHandle( hProcessSnap );

   return( TRUE );
}

void ProcessingOperations::printError( TCHAR* msg )
{
   DWORD eNum;
   TCHAR sysMsg[256];
   TCHAR* p;

   eNum = GetLastError( );
   FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, eNum,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      sysMsg, 256, NULL );

   // Trim the end of the line and terminate it with a null
   p = sysMsg;
   while( ( *p > 31 ) || ( *p == 9 ) )
      ++p;
   do { *p-- = 0; } while( ( p >= sysMsg ) &&
      ( ( *p == '.' ) || ( *p < 33 ) ) );

   // Display the message
   _tprintf( TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg );
}

void ProcessingOperations::printTopExpensiveProcesses(int top)
{
	if (m_mapProcesses.empty())
		BuildProcessesMap();

	if (m_mapProcesses.empty())
		cout << "Processes list is empty" << endl;

	struct data4sort {
		int		pid;
		ustring file_name;
		SIZE_T	file_size;
	};

	vector<data4sort> vect4top;
	vect4top.reserve(m_mapProcesses.size());

	for (auto &ob : m_mapProcesses)
	{
		PROCESS_MEMORY_COUNTERS_EX pmc;
		if (findProcessInfo(ob.second.procPID, pmc))
		{
			data4sort data;
			data.pid = ob.second.procPID;
			data.file_name = ob.second.procName;
			data.file_size = pmc.PrivateUsage;

			vect4top.push_back(data);
		}
	}
	
	std::sort(vect4top.begin(), vect4top.end(), 
					[](const data4sort& lhs, const data4sort& rhs)
					{
						return lhs.file_size > rhs.file_size;
					});

	int counter = 0;
	SIZE_T processesAllSize = 0;

	_tprintf_s(_T(" Top %d consumming memory processes \n"), top);
	_tprintf_s(_T("-------------------------------------------\n"));
	_tprintf_s(_T(" PID        Process Name \t RAM Usage\n"));
	_tprintf_s(_T("-------------------------------------------\n"));

	for (auto &ob : vect4top)
	{
		_tprintf_s(_T(" [%d]    %-15s \t %.2lf MB\n"), 
							ob.pid, ob.file_name.c_str(), (double)ob.file_size / MB_DIVIDER);

		counter++;
		processesAllSize += ob.file_size;

		if (counter == top)
			break;
	}

	_tprintf_s(_T("-------------------------------------------\n"));
	cout << "   Total used memory: " << (double) processesAllSize / MB_DIVIDER << " MB" << std::endl;
}

bool ProcessingOperations::get_filter_results(const ustring& process_name, const ustring& filter)
{
	return ((filter.empty() || std::string::npos != filter.find(_T("*"))
			? true
			: string_utils::search_substring(filter, process_name)));
}

bool ProcessingOperations::printAllProcessesInformation(bool show_details)
{
	int processesCount = 0;
	ULONG64 processesAllSize = 0;

	if (m_mapProcesses.empty())
		BuildProcessesMap();

	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682050(v=vs.85).aspx
	for (auto &proc_obj : m_mapProcesses)
	{
		ustring current_process(proc_obj.second.procName);

		auto procPID = proc_obj.second.procPID;

		PROCESS_MEMORY_COUNTERS_EX pmc;
		if (findProcessInfo(procPID, pmc))
		{
			_tprintf_s(_T("PID [%d] \t %-15s %.4lf MB\n"),
				procPID, proc_obj.second.procName.c_str(), (double)pmc.PrivateUsage / MB_DIVIDER);

			if (show_details)
			{
				printProcessDetailedInfo(procPID);
			}

			processesAllSize += pmc.PrivateUsage;
			processesCount++;
		}
	}

	if (processesCount != 0) {
		cout << "-----------------------------------" << std::endl;
		cout << "   Total used memory: " << (double)processesAllSize / MB_DIVIDER << " MB" << std::endl;
	}

	return true;
}

bool ProcessingOperations::printProcessInformation(const ustring& filter, bool show_details)
{
	int processesCount = 0;
	ULONG64 processesAllSize = 0;

	if (m_mapProcesses.empty())
		BuildProcessesMap();

	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682050(v=vs.85).aspx
	for (auto &proc_obj : m_mapProcesses)
	{
		ustring current_process(proc_obj.second.procName);

		if (string_utils::search_substring(current_process, filter))
		{
			auto procPID = proc_obj.second.procPID;

			PROCESS_MEMORY_COUNTERS_EX pmc;
			if (findProcessInfo(procPID, pmc))
			{
				_tprintf_s(_T("PID [%d] \t %-15s %.4lf MB\n"),
					procPID, proc_obj.second.procName.c_str(), (double)pmc.PrivateUsage / MB_DIVIDER);

				if (show_details)
				{
					printProcessDetailedInfo(procPID);
				}

				processesAllSize += pmc.PrivateUsage;
				processesCount++;
			}
		}
	}

	if (processesCount != 0) {
		cout << "-----------------------------------" << std::endl;
		cout << "   Total used memory: " << (double)processesAllSize / MB_DIVIDER << " MB" << std::endl;
	}
	else {
		cout << "Undetected process with \'" << filter << "' name." << std::endl;
	}

	return true;
}

bool ProcessingOperations::printProcessDetailedInfo(DWORD pid)
{
	// TO DO
	// command line
	// started from
	// PEB address
	// parent information

	return true;
}

bool ProcessingOperations::findProcessInfo(DWORD const processID, PROCESS_MEMORY_COUNTERS_EX& pmc)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
									PROCESS_VM_READ,
									FALSE, processID);
	if (NULL == hProcess)
		return false;

	if (K32GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
	{
			/*	
				printf("\tPageFaultCount: 0x%08X\n", pmc.PageFaultCount);
				printf("\tPeakWorkingSetSize: 0x%08X\n",
					pmc.PeakWorkingSetSize);
				printf("\tWorkingSetSize: 0x%08X\n", pmc.WorkingSetSize);
				printf("\tQuotaPeakPagedPoolUsage: 0x%08X\n",
					pmc.QuotaPeakPagedPoolUsage);
				printf("\tQuotaPagedPoolUsage: 0x%08X\n",
					pmc.QuotaPagedPoolUsage);
				printf("\tQuotaPeakNonPagedPoolUsage: 0x%08X\n",
					pmc.QuotaPeakNonPagedPoolUsage);
				printf("\tQuotaNonPagedPoolUsage: 0x%08X\n",
					pmc.QuotaNonPagedPoolUsage);
				printf("\tPagefileUsage: 0x%08X\n", pmc.PagefileUsage);
				printf("\tPeakPagefileUsage: 0x%08X\n",
					pmc.PeakPagefileUsage)
			*/
	}

	CloseHandle(hProcess);

	return true;
}

void ProcessingOperations::generateProcessesTree(DWORD proc_pid)
{
	if (m_mapProcesses.empty())
		BuildProcessesMap();

	ProcsTreeBuilder tree_builder(&m_mapProcesses);

	tree_builder.mapBuilder();
	tree_builder.mapHandshake();
	tree_builder.buildTree();

	auto it = m_mapProcesses.find(proc_pid);
	if (it != m_mapProcesses.end())
	{
		tree_builder.printTree(proc_pid);
	}
	else
	{
		cout << "Invalid Process ID | PID " << proc_pid << " not detected in memory" << std::endl;
	}
}
