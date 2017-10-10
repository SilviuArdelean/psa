#pragma once
#include "general.h"
#include "generic_tree.h"

class ProcsTreeBuilder
{
public:
	ProcsTreeBuilder(std::multimap<DWORD, proc_info>* ptrMap);
	ProcsTreeBuilder() = delete;

	~ProcsTreeBuilder() {};

	void mapBuilder();
	void mapHandshake();
	void buildTree();
	void printTree(int const procPID = 0);

	friend std::wostream& operator << (std::wostream& stream, const proc_info& info);

protected:
	DWORD	_parentProcExists(int nParentID) const;
	bool	_isSystemProcess(const proc_info& proc_data);
	void	_BuildTree(generic_node<proc_info>* node);

	void	_findSpecificProcess(generic_node<proc_info>* pNode, int const procPID);

	generic_node<proc_info>*  _getMapParentPtr(int parentPID);

	std::unique_ptr<proc_info>						m_ptrRoot;
	std::unique_ptr<generic_tree<proc_info>>		m_ptrTree;

	generic_node<proc_info>*							m_ptrSearchTreeNode;
	std::multimap<DWORD, proc_info>*					m_ptrMapProcesses;
	std::multimap<int, generic_node<proc_info> >		m_mapProc4Tree;	
};
