#include "stdafx.h"
#include "ProcsTreeBuilder.h"
#include "generic_tree.h"
#include "generic_tree_handler.h"

ProcsTreeBuilder::ProcsTreeBuilder(std::multimap<DWORD, proc_info>* ptrMap)
	: m_ptrMapProcesses(ptrMap)
	, m_ptrSearchTreeNode(nullptr)
{
	m_ptrRoot = std::unique_ptr<proc_info>(new proc_info(FAKE_ROOT_PID, FAKE_ROOT_PARENT_PID, _T("o"), 0));
	m_ptrTree = std::unique_ptr<generic_tree<proc_info>>(new generic_tree<proc_info>(nullptr, *m_ptrRoot));
}

ProcsTreeBuilder::~ProcsTreeBuilder()
{
}

std::wostream& operator << (std::wostream& stream, const proc_info& info)
{
	if (info.procPID != FAKE_ROOT_PID)
		stream << info.procName.c_str() << " [" << info.procPID << "] ";
	else
		stream << info.procName.c_str();

	return stream;
}

void ProcsTreeBuilder::mapBuilder()
{
	m_mapProc4Tree.clear();

	m_mapProc4Tree.insert(std::pair<DWORD, generic_node<proc_info>>(m_ptrRoot->procPID, *m_ptrRoot));

	for (auto it = m_ptrMapProcesses->begin(); it != m_ptrMapProcesses->end(); ++it)
	{
		proc_info pi(it->second.procPID,
						it->second.parentPID,
						it->second.procName,
						it->second.usedMemory);

		generic_node<proc_info>  node_data(pi);
		m_mapProc4Tree.insert(std::pair<DWORD, generic_node<proc_info>>(pi.procPID, node_data));
	}
}

DWORD ProcsTreeBuilder::_parentProcExists(DWORD nParentID) const
{
	auto itParent = m_mapProc4Tree.find(nParentID);

	return (itParent != m_mapProc4Tree.end());
}

void ProcsTreeBuilder::mapHandshake()
{
	for (auto &ob : m_mapProc4Tree)
	{
		if (ob.second.data.procPID == FAKE_ROOT_PID)
			continue;

		if ((ob.second.data.parentPID != 0) && 
			!_parentProcExists(ob.second.data.parentPID))
		{
			ob.second.data.parentPID = FAKE_ROOT_PID;		// parent process does not exists anymore
		}

		if (ob.second.data.parentPID == 0
			&& (!_isSystemProcess(ob.second.data)))		// Special case should follow the other path
		{
			ob.second.data.parentPID = m_ptrRoot->procPID;
		}

		// this info is not used and it may remains as it is
		if (auto ptrParent = _getMapParentPtr(ob.second.data.parentPID))
			ob.second.parent = ptrParent;
	}
}

void ProcsTreeBuilder::buildTree()
{
	_BuildTree(m_ptrTree->get_root());
}

void ProcsTreeBuilder::_BuildTree(generic_node<proc_info>* node)
{
	// nice to have: find a better optimal solution to build the tree
	for (auto itrev = m_mapProc4Tree.begin(); itrev != m_mapProc4Tree.end(); ++itrev)
	{
		if (itrev->first == FAKE_ROOT_PID)
			continue;

		if (itrev->second.data.parentPID == node->data.procPID)
		{
			auto crt_node_parent = m_ptrTree->add(node, itrev->second.data);
			if (!crt_node_parent)
				continue;

			_BuildTree(crt_node_parent);
		}
	}
}

void ProcsTreeBuilder::printTree(DWORD const procPID)
{
	generic_node<proc_info>* pNode = nullptr;

	if (0 == procPID || FAKE_ROOT_PID == procPID) {
		pNode = m_ptrTree->get_root();
	}
	else {
		m_ptrSearchTreeNode = nullptr;
		_findSpecificProcess(m_ptrTree->get_root(), procPID);
		pNode = m_ptrSearchTreeNode;
	}

	generic_tree_handler<proc_info>::dfs_traverse(pNode);
}

void ProcsTreeBuilder::_findSpecificProcess(generic_node<proc_info>* pNode, DWORD const procPID)
{
	if (pNode->data.procPID == procPID)
	{
		m_ptrSearchTreeNode = pNode;
		return;
	}

	for (auto itNode = pNode->listChildren.begin(); itNode != pNode->listChildren.end(); ++itNode)
	{
		generic_node<proc_info> *pItem = *itNode;
		_findSpecificProcess(pItem, procPID);
	}
}

bool ProcsTreeBuilder::_isSystemProcess(const proc_info& proc_data)
{
	return (proc_data.procPID == 4 && proc_data.parentPID == 0);
}

generic_node<proc_info>* ProcsTreeBuilder::_getMapParentPtr(DWORD parentPID)
{
	auto it_parent = m_mapProc4Tree.find(parentPID);
	return ((it_parent != m_mapProc4Tree.end())
				? &(it_parent->second)
				: nullptr);
}