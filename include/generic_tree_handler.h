#pragma once
#include "generic_tree.h"

template <typename T>
class generic_tree_handler
{

  public:

	static void dfs_traverse(GenericTreeNode<T>* node, bool lastRN = false)
	{
		if (!node)
			return;

		_setmode(_fileno(stdout), _O_U16TEXT);

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

	static void dfs_traverse_nonunicode(GenericTreeNode<T>* node)
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