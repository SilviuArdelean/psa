#include "stdafx.h"
#include "ProcsTreeBuilder.h"
#include "generic_tree.h"
#include "generic_tree_handler.h"

ProcsTreeBuilder::ProcsTreeBuilder(std::multimap<DWORD, proc_info>* ptrMap)
	: m_ptrMapProcesses(ptrMap)
{
	m_ptrRoot = new proc_info(FAKE_ROOT_PID, FAKE_ROOT_PARENT_PID, _T("o"));
	m_ptrTree = new generic_tree<proc_info>(nullptr, *m_ptrRoot);
}


ProcsTreeBuilder::~ProcsTreeBuilder()
{
	delete m_ptrRoot;
	delete m_ptrTree;
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

	m_mapProc4Tree.insert(std::pair<DWORD, GenericTreeNode<proc_info>>(m_ptrRoot->procPID, *m_ptrRoot));

	for (auto it = m_ptrMapProcesses->begin(); it != m_ptrMapProcesses->end(); ++it)
	{
		proc_info pi(it->second.procPID,
						it->second.parentPID,
						it->second.procName);

		GenericTreeNode<proc_info>  node_data(pi);
		m_mapProc4Tree.insert(std::pair<DWORD, GenericTreeNode<proc_info>>(pi.procPID, node_data));
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
			ob.second.data.parentPID = 0;		// parent process does not exists anymore
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

void ProcsTreeBuilder::_BuildTree(GenericTreeNode<proc_info>* node)
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

void ProcsTreeBuilder::printTree()
{
	generic_tree_handler<proc_info>::dfs_traverse(m_ptrTree->get_root());
}


bool ProcsTreeBuilder::_isSystemProcess(const proc_info& proc_data)
{
	return (proc_data.procPID == 4 && proc_data.parentPID == 0);
}

GenericTreeNode<proc_info>* ProcsTreeBuilder::_getMapParentPtr(DWORD parentPID)
{
	auto it_parent = m_mapProc4Tree.find(parentPID);
	return ((it_parent != m_mapProc4Tree.end())
				? &(it_parent->second)
				: nullptr);
}