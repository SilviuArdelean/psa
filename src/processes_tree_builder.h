#pragma once
#include "general.h"
#include "generic_tree.h"

class ProcsTreeBuilder {
 public:
  ProcsTreeBuilder(std::multimap<DWORD, proc_info>* ptrMap);
  ProcsTreeBuilder() = delete;

  ~ProcsTreeBuilder(){};

  void MapBuilder();
  void MapHandshake();
  void BuildTree();
  void PrintTree(int const procPID = 0);

  friend std::wostream& operator<<(std::wostream& stream,
                                   const proc_info& info);

  void PrintIt(generic_node<proc_info>* info);

 protected:
  bool ParentProcExists(int nParentID) const;
  bool IsSystemProcess(const proc_info& proc_data);
  void BuildTreeRecursive(generic_node<proc_info>* node);

  void FindSpecificProcess(generic_node<proc_info>* pNode, int const procPID);

  generic_node<proc_info>* GetMapParentPtr(int parentPID);

  std::unique_ptr<proc_info> ptr_root_;
  std::unique_ptr<generic_tree<proc_info>> ptr_tree_;

  generic_node<proc_info>* ptr_search_tree_node_ = nullptr;
  std::multimap<DWORD, proc_info>* ptr_map_processes_ = nullptr;
  std::multimap<int, generic_node<proc_info>> map_proc4tree_;
};
