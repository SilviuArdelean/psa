// testGenericTree.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>

#include "generic_tree.h"
#include "generic_tree_handler.h"

typedef int INT;

int main() {
  int root_value = 333;
  int val = 0;
  generic_tree<int>* tree = new generic_tree<int>(nullptr, root_value);
  auto ptrRoot = tree->get_root();

  val = 22;
  auto p22 = tree->add(ptrRoot, val);
  val = 11;
  auto p11 = tree->add(p22, val);
  val = 6;
  auto p6 = tree->add(p11, val);
  val = 2;
  auto p2 = tree->add(p6, val);
  val = 8;
  auto p8 = tree->add(p11, val);

  val = 15;
  tree->add(p22, val);
  val = 18;
  tree->add(p22, val);

  val = 12;
  auto p12 = tree->add(ptrRoot, val);
  val = 9;
  auto p9 = tree->add(p12, val);
  val = 5;
  auto p5 = tree->add(p9, val);
  val = 7;
  auto p7 = tree->add(p12, val);

  val = 27;
  auto p27 = tree->add(ptrRoot, val);
  val = 21;
  auto p21 = tree->add(p27, val);
  val = 25;
  auto p25 = tree->add(p27, val);
  val = 26;
  auto p26 = tree->add(p27, val);

  val = 77;
  auto p77 = tree->add(ptrRoot, val);
  val = 68;
  auto p68 = tree->add(p77, val);
  val = 69;
  auto p69 = tree->add(p77, val);

  val = 67;
  auto p67 = tree->add(ptrRoot, val);
  val = 54;
  auto p54 = tree->add(p67, val);
  val = 56;
  auto p56 = tree->add(p54, val);
  val = 59;
  auto p59 = tree->add(p54, val);
  val = 62;
  auto p62 = tree->add(p67, val);
  val = 65;
  auto p65 = tree->add(p67, val);
  val = 71;
  auto p71 = tree->add(p65, val);

  generic_tree_handler<int>::dfs_traverse(ptrRoot);

  std::wcout << std::endl
             << std::endl
             << "-----------------------------" << std::endl;

  generic_tree_handler<int>::dfs_traverse_nonunicode(ptrRoot);

  std::wcout << std::endl;

  std::wcout << std::endl << "+++++++++++++++++++++++++++++" << std::endl;

  delete tree;
  return 0;
}