
#include "processes_tree_builder.h"

#include <map>
#include <ranges>

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
  std::wcout << info->data.procName.c_str()
             << _T(" [" << info->data.procPID << "] ") << std::endl;
}

std::wostream& operator<<(std::wostream& stream, const proc_info& info) {
  if (info.procPID != FAKE_ROOT_PID)
    stream << info.procName.c_str() << _T(" [" << info.procPID << "] ");
  else
    stream << info.procName.c_str();

  return stream;
}

void ProcsTreeBuilder::MapBuilder() {
  map_proc4tree_.clear();

  map_proc4tree_.insert(std::pair<DWORD, generic_node<proc_info>>(
      ptr_root_->procPID, *ptr_root_));

  for (auto& [key, proc] : *ptr_map_processes_) {
    proc_info pi(proc.procPID, proc.parentPID, proc.procName, proc.usedMemory);
    map_proc4tree_.emplace(pi.procPID, generic_node<proc_info>(pi));
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
  auto children = map_proc4tree_
      | std::views::filter([&](auto& kv) {
          return kv.first != FAKE_ROOT_PID
              && kv.second.data.parentPID == node->data.procPID;
        });

  for (auto& [key, child_node] : children) {
    auto crt_node_parent = ptr_tree_->add(node, child_node.data);
    if (crt_node_parent)
      BuildTreeRecursive(crt_node_parent);
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

  for (auto* pItem : pNode->listChildren) {
    if (ptr_search_tree_node_)
      return;  // early exit once node is found
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