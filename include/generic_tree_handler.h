#pragma once

#include "generic_tree.h"
#ifdef _WIN32
	#include <fcntl.h>  
	#include <io.h>  
#endif
#include <locale>

#define USE_CODECVT 0
#define USE_IMBUE   1

#if USE_CODECVT
#include <codecvt>
#endif 

template <typename T>
class generic_tree_handler
{

  public:

	static void dfs_traverse(generic_node<T>* node, bool lastRN = false)
	{
		if (!node)
			return;
#ifdef _WIN32 
		_setmode(_fileno(stdout), _O_U16TEXT);
#elif __linux__
		#if USE_CODECVT
				std::locale en("en_US.utf8",
					new codecvt_utf8<wchar_t, 0x10ffff, consume_header>{});
		#else
				std::locale en("en_US.utf8");
		#endif
		#ifdef USE_IMBUE
				std::wcout.imbue(en);
		#else
				std::locale::global(en);
		#endif
#endif
		//std::wiostream output;
		//output.imbue(std::locale("en_US.utf8"));

		// list of Unicode characters 
		// http://www.fileformat.info/info/unicode/category/So/list.htm

		if (node->level > 0)
		{
			if (node->level == 1)
			{
				if (!lastRN)
					std::wcout << _T("\u251C\u2500\u2500\u2500 ");		//std::wcout << "├───";
				else
					std::wcout << _T("\u2514\u2500\u2500\u2500 ");		//std::wcout << "└───";
			}
			else
			{
				for (auto i = 1; i < node->level; i++)
				{
					if (!lastRN)
						std::wcout << _T("\u2502   ");				//std::wcout << "|";
					else
						std::wcout << _T("    ");
				}

				std::wcout << _T("\u2514\u2500\u2500\u2500 ");			//std::wcout << "└───";
			}
		}

		std::wcout << node->data << _T("\n");

		for (auto it = node->listChildren.begin(); it != node->listChildren.end(); ++it)
		{
			auto it_temp = it;
			it_temp++;

			if (node->parent == nullptr 
					&& it_temp == node->listChildren.end())
			{
				
				lastRN = true;	// the last child of the root
			}

			dfs_traverse(*it, lastRN);
		}
	}

	static void dfs_traverse_nonunicode(generic_node<T>* node)
	{
		if (!node)
			return;

		if (node->level > 0)
		{
			if (node->level == 1)
			{
				std::wcout << _T("|---");
			}
			else
			{
				std::wcout << _T("|");

				for (auto i = 0; i < node->level - 1; i++)
				{
					std::wcout << "     ";
				}

				std::wcout << _T("+---- ");
			}
		}

		std::wcout << node->data << _T("\n");

		for (auto it = node->listChildren.begin(); it != node->listChildren.end(); ++it)
		{
			dfs_traverse_nonunicode(*it);
		}
	}
};