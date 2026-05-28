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

ProcsTreeBuilder::ProcsTreeBuilder(std::multimap<DWORD, proc_info>* ptr_map)
    : ptr_search_tree_node_(nullptr), ptr_map_processes_(ptr_map) {
  ptr_root_ = std::unique_ptr<proc_info>(
      new proc_info(kFakeRootPID, kFakeRootParentPID, _T("o"), 0, _T("")));
  ptr_tree_ = std::unique_ptr<generic_tree<proc_info>>(
      new generic_tree<proc_info>(nullptr, *ptr_root_));
}

void ProcsTreeBuilder::PrintIt(generic_node<proc_info>* info) {
  ucout << info->data.proc_name.c_str() << _T(" [") << info->data.proc_pid
        << _T("] ") << std::endl;
}

uostream& operator<<(uostream& stream, const proc_info& info) {
  if (info.proc_pid != kFakeRootPID)
    stream << info.proc_name.c_str() << _T(" [") << info.proc_pid << _T("] ");
  else
    stream << info.proc_name.c_str();

  return stream;
}

void ProcsTreeBuilder::MapBuilder() {
  map_proc4tree_.clear();

  map_proc4tree_.insert(std::pair<DWORD, generic_node<proc_info>>(
      ptr_root_->proc_pid, *ptr_root_));

  for (const auto& proc : *ptr_map_processes_ | std::views::values) {
    proc_info pi(proc.proc_pid, proc.parent_pid, proc.proc_name,
                 proc.used_memory, proc.cmdline_args);
    map_proc4tree_.emplace(pi.proc_pid, pi);
  }
}

bool ProcsTreeBuilder::ParentProcExists(int parent_pid) const {
  auto itParent = map_proc4tree_.find(parent_pid);

  return (itParent != map_proc4tree_.end());
}

void ProcsTreeBuilder::MapHandshake() {
  for (auto& ob : map_proc4tree_) {
    if (ob.second.data.proc_pid == kFakeRootPID)
      continue;

    if ((ob.second.data.parent_pid != 0) &&
        !ParentProcExists(ob.second.data.parent_pid)) {
      ob.second.data.parent_pid = kFakeRootPID;
    }

    if (ob.second.data.parent_pid == 0 && (!IsSystemProcess(ob.second.data))) {
      ob.second.data.parent_pid = ptr_root_->proc_pid;
    }

    if (auto ptrParent = GetMapParentPtr(ob.second.data.parent_pid))
      ob.second.parent = ptrParent;
  }
}

void ProcsTreeBuilder::BuildTree() {
  BuildTreeRecursive(ptr_tree_->get_root());
}

void ProcsTreeBuilder::BuildTreeRecursive(generic_node<proc_info>* node) {
  for (const auto& kv : map_proc4tree_) {
    if (kv.first == kFakeRootPID ||
        kv.second.data.parent_pid != node->data.proc_pid)
      continue;

    if (auto* added = ptr_tree_->add(node, kv.second.data))
      BuildTreeRecursive(added);
  }
}

void ProcsTreeBuilder::PrintTree(int const proc_pid, bool print_header) {
  generic_node<proc_info>* pnode = nullptr;

  if (0 == proc_pid || kFakeRootPID == proc_pid) {
    pnode = ptr_tree_->get_root();
  } else {
    ptr_search_tree_node_ = nullptr;
    FindSpecificProcess(ptr_tree_->get_root(), proc_pid);
    pnode = ptr_search_tree_node_;
  }

  if (!pnode) {
    ucout << _T("Invalid process") << std::endl;
    return;
  }

  if (print_header) {
    ucout << _T("PID\tProcess Name") << std::endl;
  }

#ifdef __linux__
  generic_tree_handler<proc_info> gt;
  gt.set_parent(this);
  gt.dfs_traverse_nonstatic(pnode);
#else
  generic_tree_handler<proc_info>::dfs_traverse(pnode);
#endif
}

void ProcsTreeBuilder::FindSpecificProcess(generic_node<proc_info>* node,
                                           int const proc_pid) {
  if (node->data.proc_pid == proc_pid) {
    ptr_search_tree_node_ = node;
    return;
  }

  for (auto* child : node->list_children) {
    if (ptr_search_tree_node_)
      return;
    FindSpecificProcess(child, proc_pid);
  }
}

bool ProcsTreeBuilder::IsSystemProcess(const proc_info& proc_data) {
  return (proc_data.proc_pid == 4 && proc_data.parent_pid == 0);
}

generic_node<proc_info>* ProcsTreeBuilder::GetMapParentPtr(int parent_pid) {
  auto it_parent = map_proc4tree_.find(parent_pid);
  return ((it_parent != map_proc4tree_.end()) ? &(it_parent->second) : nullptr);
}
