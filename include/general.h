#pragma once

#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#include "Psapi.h"
#include "Shlwapi.h"
#include "TlHelp32.h"

#pragma comment(lib, "psapi.lib") 
#pragma comment(lib, "Shlwapi.lib")
#endif

/* changed for deprecation warnings */

#ifdef _UNICODE
#define ustrcmp			wcscmp
#define ustrcpy			wcscpy
#define ustring			std::wstring
#define usnprintf		_snwprintf
#define ufopen			_wfopen
#define ufopen_s		_wfopen_s
#define ufprintf		fwprintf
#define uprintf			wprintf
#define ufputs			fputws
#define ustrncat		wcsncat
#define ustrlen			wcslen
#define uvsnprintf		_vsntprintf
#define ustrchr			wcschr
#define ustrrchr		wcsrchr
#define ustricmp		_wcsicmp
#define ufscanf_s		fwscanf_s
#define ustringstream	std::wstringstream
#define itou			_itow_s
#define utok			wcstok_s
#define SEPARATOR		_T("]|["
#define cout			std::wcout
#else
#define ustrcmp			strcmp
#define ustring			std::string
#define ustrcpy			strcpy
#define usnprintf		snprintf
#define ufopen			fopen
#define ufopen_s		_fopen_s
#define ufprintf		fprintf
#define uprintf			printf
#define ufputs			fputs
#define ustrncat		strncat
#define ustrlen			strlen 
#define uvsnprintf		_vsntprintf
#define ustrchr			strchr
#define ustrrchr		strrchr
#define ustricmp		stricmp
#define ufscanf_s		fscanf_s
#define ustringstream	std::stringstream
#define itou			_itoa_s
#define	utok			strtok_s
#define SEPARATOR		"]|["
#define cout			std::cout
#endif

#define MB_DIVIDER		(1024 * 1024)
#define KB_DEVIDER		(1024 * 1024 * 1024)

#define FAKE_ROOT_PID			999999
#define FAKE_ROOT_PARENT_PID	1000000

struct proc_info 
{
	DWORD	procPID;
	DWORD	parentPID;
	ustring procName;
	SIZE_T	usedMemory;

	proc_info()
		: procPID(0)
		, parentPID(0)
		, procName(_T(""))
		, usedMemory(0)
	{
	}

	proc_info(DWORD	_procPID, DWORD	_parentPID,
		ustring _procName, SIZE_T _usedMemory)
		: procPID(_procPID)
		, parentPID(_parentPID)
		, procName(_procName)
		, usedMemory(_usedMemory)
	{
	}

	proc_info(const proc_info& rhs)
		: procPID(rhs.procPID)
		, parentPID(rhs.parentPID)
		, procName(rhs.procName)
		, usedMemory(rhs.usedMemory)
	{
	}

	proc_info& operator = (const proc_info& rhs)
	{
		if (this != &rhs)
		{
			procPID		= rhs.procPID;
			parentPID	= rhs.parentPID;
			procName	= rhs.procName;
			usedMemory	= rhs.usedMemory;
		}

		return *this;
	}

	proc_info(proc_info&& rhs)
	{
		procPID		= rhs.procPID;
		parentPID	= rhs.parentPID;
		procName	= rhs.procName;
		usedMemory	= rhs.usedMemory;

		rhs.procPID = 0;
		rhs.parentPID = 0;
		rhs.procName = _T("");
		rhs.usedMemory = 0;
	}

	proc_info& operator = (proc_info&& rhs)
	{
		if (this != &rhs)
		{
			procPID		= rhs.procPID;
			parentPID	= rhs.parentPID;
			procName	= rhs.procName;
			usedMemory	= rhs.usedMemory;

			rhs.procPID = 0;
			rhs.parentPID = 0;
			rhs.procName.clear();
			rhs.usedMemory = 0;
		}

		return *this;
	}

};

enum PSA_INTERNAL_ERRORS {
	invalid_processing_operations = 10001
};

