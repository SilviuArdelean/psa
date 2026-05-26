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

#pragma once
#include "general.h"
#include "generic_tree.h"

class ProcsTreeBuilder {
 public:
  ProcsTreeBuilder(std::multimap<DWORD, proc_info>* ptrMap);
  ProcsTreeBuilder() = delete;

  ~ProcsTreeBuilder() {};

  void MapBuilder();
  void MapHandshake();
  void BuildTree();
  void PrintTree(int const procPID = 0, bool print_header = false);

  friend uostream& operator<<(uostream& stream, const proc_info& info);

  void PrintIt(generic_node<proc_info>* info);

 protected:
  [[nodiscard]] bool ParentProcExists(int nParentID) const;
  [[nodiscard]] bool IsSystemProcess(const proc_info& proc_data);
  void BuildTreeRecursive(generic_node<proc_info>* node);

  void FindSpecificProcess(generic_node<proc_info>* pNode, int const procPID);

  [[nodiscard]] generic_node<proc_info>* GetMapParentPtr(int parentPID);

  std::unique_ptr<proc_info> ptr_root_;
  std::unique_ptr<generic_tree<proc_info>> ptr_tree_;

  generic_node<proc_info>* ptr_search_tree_node_ = nullptr;
  std::multimap<DWORD, proc_info>* ptr_map_processes_ = nullptr;
  std::multimap<int, generic_node<proc_info>> map_proc4tree_;
};
