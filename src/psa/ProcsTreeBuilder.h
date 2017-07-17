#pragma once
#include "general.h"
#include "generic_tree.h"

class ProcsTreeBuilder
{
public:
	ProcsTreeBuilder(std::multimap<DWORD, proc_info>* ptrMap);
	ProcsTreeBuilder() = delete;

	~ProcsTreeBuilder();

	void mapBuilder();
	void mapHandshake();
	void buildTree();
	void printTree(DWORD procPID = 0);

	friend std::wostream& operator << (std::wostream& stream, const proc_info& info);

protected:
	DWORD	_parentProcExists(DWORD nParentID) const;
	bool	_isSystemProcess(const proc_info& proc_data);
	void	_BuildTree(GenericTreeNode<proc_info>* node);

	void	_findSpecificProcess(GenericTreeNode<proc_info>* pNode, DWORD procPID);

	GenericTreeNode<proc_info>*  _getMapParentPtr(DWORD parentPID);

	proc_info*										m_ptrRoot;
	GenericTreeNode<proc_info>*						m_ptrSearchTreeNode;
	generic_tree<proc_info>*						m_ptrTree;
	std::multimap<DWORD, proc_info>*					m_ptrMapProcesses;
	std::multimap<int, GenericTreeNode<proc_info> >		m_mapProc4Tree;	
};

