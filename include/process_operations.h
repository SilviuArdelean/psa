#pragma once
#include "general.h"
#include "string_utils.h"

#ifdef __linux__
#include <proc/readproc.h>
#include<signal.h>
#endif

#define process_fake_name	_T("_|_")

class process_operations
{
public:
	static void kill_process_by_name(const TCHAR *process_name)
	{
		execute_kill_process(0, process_name);
	}

	static void kill_process_by_pid(const int process_pid)
	{
		execute_kill_process(process_pid, process_fake_name);
	}

	static void kill_process_by_name_optimized(const TCHAR *process_name, const procs_map& map_processes)
	{
		execute_kill_process_optimized(0, process_name, map_processes);
	}

	static void kill_process_by_pid_optimized(const int process_pid, const procs_map& map_processes)
	{
		execute_kill_process_optimized(process_pid, process_fake_name, map_processes);
	}

private:

	static bool is_essential_proccess(const int process_pid) 
	{
		return (process_pid == 0 || process_pid == 1);
	}

	static void execute_kill_process(const int process_pid, const TCHAR *process_name)
	{
#ifdef _WIN32
		PROCESSENTRY32 pEntry;
		pEntry.dwSize = sizeof(pEntry);

		HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
		BOOL hRes = Process32First(hSnapShot, &pEntry);
		while (hRes)
		{
			if ((!is_essential_proccess(process_pid) && pEntry.th32ProcessID == process_pid)
				|| string_utils::search_substring(pEntry.szExeFile, process_name))
			{
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, (DWORD)pEntry.th32ProcessID);
				if (NULL != hProcess)
				{
					TerminateProcess(hProcess, 9);
					CloseHandle(hProcess);
				}
			}

			hRes = Process32Next(hSnapShot, &pEntry);
		}
		
		CloseHandle(hSnapShot);

#else	// Linux stuff

		PROCTAB* proc = openproc(PROC_FILLARG			// fillarg used for cmdline
									| PROC_FILLSTAT);	// fillstat used for cmd

		static proc_t _process;			// needs static because of readproc() throws segmantation fault unleast on Ubuntu 16 
										// https://gitlab.com/procps-ng/procps/issues/33

		memset(&_process, 0, sizeof(_process));	// zero out the allocated proc_info memory

		while (readproc(proc, &_process) != NULL)
		{
			ustring strProcessCmd = (_process.cmdline != NULL) ? *_process.cmdline : _process.cmd;

			if ((!is_essential_proccess(process_pid) && _process.tid == process_pid)
					|| string_utils::search_substring(strProcessCmd, process_name))
			{
				kill(_process.tid, SIGKILL);

				if (_process.tid == process_pid)	break;
			}
		}

		closeproc(proc);
#endif
	}

	static void execute_kill_process_optimized(const int process_pid, const TCHAR *process_name, const procs_map& map_processes)
	{
		if (string_utils::search_substring(process_fake_name, process_name))
		{
			// Try to kill the process based by PID
			auto it_process = map_processes.find(process_pid);
			if (it_process != map_processes.end())
			{
				#ifdef _WIN32
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, (DWORD)process_pid);
					if (NULL != hProcess)
					{
						TerminateProcess(hProcess, 9);
						CloseHandle(hProcess);
					}
				#else	// Linux stuff
					kill(process_pid, SIGKILL);
				#endif
			}
		}
		else 
		{
			for (auto &proc : map_processes)
			{
				if (string_utils::search_substring(proc.second.procName, process_name))
				{
#ifdef _WIN32
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, (DWORD)proc.second.procPID);
					if (NULL != hProcess)
					{
						TerminateProcess(hProcess, 9);
						CloseHandle(hProcess);
					}
#else
					kill(proc.second.procPID, SIGKILL);
#endif
				}
			}
		}
	}

};