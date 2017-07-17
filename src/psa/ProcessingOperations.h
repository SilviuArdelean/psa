
#pragma once
#include <map>
#include <stack>
#include <vector>


typedef std::multimap<DWORD, proc_info>::iterator   proc_iter;
typedef std::multimap<DWORD, proc_info>				procs_map;


class ProcessingOperations
{
  public:
    ProcessingOperations(void);
    ~ProcessingOperations(void);

    BOOL BuildProcessesMap();

	procs_map*  GetProcessesMap()   { 		return &m_mapProcesses; }

	bool printAllProcessesInformation(bool show_details = false);
	bool printProcessInformation(const ustring& process_name, bool show_details = false);
	void printTopExpensiveProcesses(int top);

	void generateProcessesTree(DWORD proc_pid);
	
   protected:
     void printError( TCHAR* msg );
	 bool printProcessDetailedInfo(DWORD pid);
	 bool get_filter_results(const ustring& process_name, const ustring& current_process);

	 bool findProcessInfo(DWORD const processID, PROCESS_MEMORY_COUNTERS_EX& pmc);
	
     BOOL SetPrivilege( HANDLE hToken,          // access token handle
                           LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
                           BOOL bEnablePrivilege);   // to enable or disable privilege

   protected:
      procs_map m_mapProcesses;	
};

