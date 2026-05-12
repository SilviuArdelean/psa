
#include "processes_tree_builder.h"

#include <map>

#include "generic_tree.h"
#include "generic_tree_handler.h"

ProcsTreeBuilder::ProcsTreeBuilder(std::multimap<DWORD, proc_info>* ptrMap)
    : ptr_search_tree_node_(nullptr), ptr_map_processes_(ptrMap) {
  ptr_root_ = std::unique_ptr<proc_info>(
      new proc_info(FAKE_ROOT_PID, FAKE_ROOT_PARENT_PID, _T("o"), 0));
  ptr_tree_ = std::unique_ptr<generic_tree<proc_info>>(
      new generic_tree<proc_info>(nullptr, *ptr_root_));
}

void ProcsTreeBuilder::PrintIt(generic_node<proc_info>* info) {
  ucout << info->data.procName.c_str()
        << _T(" [") << info->data.procPID << _T("] ") << std::endl;
}

uostream& operator<<(uostream& stream, const proc_info& info) {
  if (info.procPID != FAKE_ROOT_PID)
    stream << info.procName.c_str() << _T(" [") << info.procPID << _T("] ");
  else
    stream << info.procName.c_str();

  return stream;
}

void ProcsTreeBuilder::MapBuilder() {
  map_proc4tree_.clear();

  map_proc4tree_.insert(std::pair<DWORD, generic_node<proc_info>>(
      ptr_root_->procPID, *ptr_root_));

  for (auto it = ptr_map_processes_->begin(); it != ptr_map_processes_->end();
       ++it) {
    proc_info pi(it->second.procPID, it->second.parentPID, it->second.procName,
                 it->second.usedMemory);

    generic_node<proc_info> node_data(pi);
    map_proc4tree_.insert(
        std::pair<DWORD, generic_node<proc_info>>(pi.procPID, node_data));
  }
}

bool ProcsTreeBuilder::ParentProcExists(int nParentID) const {
  auto itParent = map_proc4tree_.find(nParentID);

  return (itParent != map_proc4tree_.end());
}

void ProcsTreeBuilder::MapHandshake() {
  for (auto& ob : map_proc4tree_) {
    if (ob.second.data.procPID == FAKE_ROOT_PID)
      continue;

    if ((ob.second.data.parentPID != 0) &&
        !ParentProcExists(ob.second.data.parentPID)) {
      ob.second.data.parentPID =
          FAKE_ROOT_PID;  // parent process does not exists anymore
    }

    if (ob.second.data.parentPID == 0 &&
        (!IsSystemProcess(
            ob.second.data)))  // Special case should follow the other path
    {
      ob.second.data.parentPID = ptr_root_->procPID;
    }

    // this info is not used and it may remains as it is
    if (auto ptrParent = GetMapParentPtr(ob.second.data.parentPID))
      ob.second.parent = ptrParent;
  }
}

void ProcsTreeBuilder::BuildTree() {
  BuildTreeRecursive(ptr_tree_->get_root());
}

void ProcsTreeBuilder::BuildTreeRecursive(generic_node<proc_info>* node) {
  // nice to have: find a better optimal solution to build the tree
  for (auto itrev = map_proc4tree_.begin(); itrev != map_proc4tree_.end();
       ++itrev) {
    if (itrev->first == FAKE_ROOT_PID)
      continue;

    if (itrev->second.data.parentPID == node->data.procPID) {
      auto crt_node_parent = ptr_tree_->add(node, itrev->second.data);
      if (!crt_node_parent)
        continue;

      BuildTreeRecursive(crt_node_parent);
    }
  }
}

void ProcsTreeBuilder::PrintTree(int const procPID) {
  generic_node<proc_info>* pNode = nullptr;

  if (0 == procPID || FAKE_ROOT_PID == procPID) {
    pNode = ptr_tree_->get_root();
  } else {
    ptr_search_tree_node_ = nullptr;
    FindSpecificProcess(ptr_tree_->get_root(), procPID);
    pNode = ptr_search_tree_node_;
  }

  if (!pNode) {
    ucout << _T("Invalid process") << std::endl;
    return;
  }

#ifdef __linux__
  generic_tree_handler<proc_info> gt;
  gt.set_parent(this);
  gt.dfs_traverse_nonstatic(pNode);
  // generic_tree_handler<proc_info>::dfs_traverse_nonunicode(pNode);    //
  // don't want ASCII only
#else
  generic_tree_handler<proc_info>::dfs_traverse(pNode);
#endif
}

void ProcsTreeBuilder::FindSpecificProcess(generic_node<proc_info>* pNode,
                                           int const procPID) {
  if (pNode->data.procPID == procPID) {
    ptr_search_tree_node_ = pNode;
    return;
  }

  for (auto itNode = pNode->listChildren.begin();
       itNode != pNode->listChildren.end(); ++itNode) {
    if (ptr_search_tree_node_)
      return;  // early exit once node is found
    generic_node<proc_info>* pItem = *itNode;
    FindSpecificProcess(pItem, procPID);
  }
}

bool ProcsTreeBuilder::IsSystemProcess(const proc_info& proc_data) {
  return (proc_data.procPID == 4 && proc_data.parentPID == 0);
}

generic_node<proc_info>* ProcsTreeBuilder::GetMapParentPtr(int parentPID) {
  auto it_parent = map_proc4tree_.find(parentPID);
  return ((it_parent != map_proc4tree_.end()) ? &(it_parent->second) : nullptr);
}