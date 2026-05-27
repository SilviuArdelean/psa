/*
 * Copyright (c) 2017-2026 Silviu-Marius Ardelean
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "processes_tree_builder.h"
#include "pch.h"

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
  ucout << info->data.procName.c_str() << _T(" [") << info->data.procPID
        << _T("] ") << std::endl;
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

  for (const auto& proc : *ptr_map_processes_ | std::views::values) {
    proc_info pi(proc.procPID, proc.parentPID, proc.procName, proc.usedMemory);
    map_proc4tree_.emplace(pi.procPID, pi);
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
  auto children = map_proc4tree_ | std::views::filter([&](const auto& kv) {
                    return kv.first != FAKE_ROOT_PID &&
                           kv.second.data.parentPID == node->data.procPID;
                  });
  for (const auto& child_node : children | std::views::values) {
    if (auto* added = ptr_tree_->add(node, child_node.data))
      BuildTreeRecursive(added);
  }
}

void ProcsTreeBuilder::PrintTree(int const procPID, bool print_header) {
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

  // Print header only if requested
  if (print_header) {
    ucout << _T("PID\tProcess Name") << std::endl;
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

  for (auto* child : pNode->listChildren) {
    if (ptr_search_tree_node_)
      return;  // early exit once node is found
    FindSpecificProcess(child, procPID);
  }
}

bool ProcsTreeBuilder::IsSystemProcess(const proc_info& proc_data) {
  return (proc_data.procPID == 4 && proc_data.parentPID == 0);
}

generic_node<proc_info>* ProcsTreeBuilder::GetMapParentPtr(int parentPID) {
  auto it_parent = map_proc4tree_.find(parentPID);
  return ((it_parent != map_proc4tree_.end()) ? &(it_parent->second) : nullptr);
}
