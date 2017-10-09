
#pragma once
#include <map>
#include <stack>
#include <vector>
#include "general.h"


typedef std::multimap<DWORD, proc_info>::iterator   proc_iter;
typedef std::multimap<DWORD, proc_info>				procs_map;


class ProcessingOperations
{
  public:
    ProcessingOperations(void);
    ~ProcessingOperations(void);

    bool BuildProcessesMap();

	procs_map*  GetProcessesMap()   { 		return &m_mapProcesses; }

	bool printAllProcessesInformation(bool const show_details = false);
	bool printProcessInformation(const ustring& process_name, bool const show_details = false);
	void printTopExpensiveProcesses(const int top);

	void generateProcessesTree(DWORD const proc_pid);
	
   protected:     
	 bool	printProcessDetailedInfo(DWORD pid);
	 bool	get_filter_results(const ustring& process_name, const ustring& current_process);
	 

#ifdef _WIN32
	 void printError(TCHAR* msg);
	 SIZE_T	getProcessUsedMemory(DWORD const processID) const;
     BOOL SetPrivilege( HANDLE hToken,          // access token handle
                           LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
                           BOOL bEnablePrivilege);   // to enable or disable privilege
#endif

   protected:
      procs_map m_mapProcesses;	
};

