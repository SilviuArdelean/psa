#pragma once

#include "generic_tree.h"
#ifdef _WIN32
	#include <fcntl.h>  
	#include <io.h>  
#endif
#include <locale>

class ProcsTreeBuilder;

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
#else
//		setlocale(LC_ALL, "en_US.UTF-8");
		std::wcout.sync_with_stdio(false);
		std::wcout.imbue(std::locale("en_US.utf8"));
#endif

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

		std::wcout << node->data << std::endl;

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

	void dfs_traverse_nonstatic(generic_node<T>* node, bool lastRN = false)
	{
		if (!node)
			return;

#ifdef _WIN32 
		_setmode(_fileno(stdout), _O_U16TEXT);
#else
		//		setlocale(LC_ALL, "en_US.UTF-8");
		std::wcout.sync_with_stdio(false);
		std::wcout.imbue(std::locale("en_US.utf8"));
#endif

		if (node->level > 0)
		{
			if (node->level == 1)
			{
				wchar_t ch1 = 0x2514, ch2 = 0x2500, ch3 = 0x251C;

				if (!lastRN)
					std::wcout << ch3 << ch2 << ch2 << ch2 << _T(" ");			//std::wcout << "├───";
				else
					std::wcout << ch1 << ch2 << ch2 << ch2 << _T(" ");			//std::wcout << "└───";
			}
			else
			{
				//bool first = true;
				for (auto i = 1; i < node->level; i++)
				{
					// needs checking if there are other nodes on the same level and if yes then add 0x2502
					// temporary comment to have a cleaner tree
					//if (!lastRN && first) {
					//	first = false;
					//wchar_t ch2 = 0x2502;
					//	std::wcout << ch2 << _T("    ");				//std::wcout << "|";
					//}
					//else
						std::wcout << _T("    ");
				}

				wchar_t ch1 = 0x2514, ch2 = 0x2500;// , ch3 = 0x251C;

				std::wcout << ch1 << ch2 << ch2 << ch2 << _T(" ");			//std::wcout << "└───";	├───
			}
		}

		if (parent)	
			parent->print_it(node);
		else 
			std::wcout << node->data;

		for (auto it = node->listChildren.begin(); it != node->listChildren.end(); ++it)
		{
			auto it_temp = it;
			it_temp++;

			if (node->parent == nullptr
				&& it_temp == node->listChildren.end())
			{

				lastRN = true;	// the last child of the root
			}

			dfs_traverse_nonstatic(*it, lastRN);
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

				std::wcout << _T("+--- ");
			}
		}

		std::wcout << node->data;

		for (auto it = node->listChildren.begin(); it != node->listChildren.end(); ++it)
		{
			dfs_traverse_nonunicode(*it);
		}
	}
	
	void set_parent(ProcsTreeBuilder *_parent)
	{
		parent = _parent;
	}

 private:
	ProcsTreeBuilder *parent = nullptr;
};