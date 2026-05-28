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

#include <gtest/gtest.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "generic_tree.h"
#include "generic_tree_handler.h"

using INT = int;

// ===================== Print Helper (local) =====================
namespace {
template <typename T>
void print_tree_indented(std::ostream& os,
                         generic_node<T>* node,
                         int indent = 0) {
  if (!node)
    return;
  for (int i = 0; i < indent; ++i)
    os << "  ";
  os << node->data << "\n";
  for (auto child : node->list_children) {
    print_tree_indented(os, child, indent + 1);
  }
}
}  // namespace

class GenericTreeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    root_value = 333;
    tree = new generic_tree<INT>(nullptr, root_value);
    ptrRoot = tree->get_root();

    int val = 22;
    p22 = tree->add(ptrRoot, val);
    val = 11;
    p11 = tree->add(p22, val);
    val = 6;
    p6 = tree->add(p11, val);
    val = 2;
    p2 = tree->add(p6, val);
    val = 8;
    p8 = tree->add(p11, val);
    val = 15;
    p15 = tree->add(p22, val);
    val = 18;
    p18 = tree->add(p22, val);

    val = 12;
    p12 = tree->add(ptrRoot, val);
    val = 9;
    p9 = tree->add(p12, val);
    val = 5;
    p5 = tree->add(p9, val);
    val = 7;
    p7 = tree->add(p12, val);

    val = 27;
    p27 = tree->add(ptrRoot, val);
    val = 21;
    p21 = tree->add(p27, val);
    val = 25;
    p25 = tree->add(p27, val);
    val = 26;
    p26 = tree->add(p27, val);

    val = 77;
    p77 = tree->add(ptrRoot, val);
    val = 68;
    p68 = tree->add(p77, val);
    val = 69;
    p69 = tree->add(p77, val);

    val = 67;
    p67 = tree->add(ptrRoot, val);
    val = 54;
    p54 = tree->add(p67, val);
    val = 56;
    p56 = tree->add(p54, val);
    val = 59;
    p59 = tree->add(p54, val);
    val = 62;
    p62 = tree->add(p67, val);
    val = 65;
    p65 = tree->add(p67, val);
    val = 71;
    p71 = tree->add(p65, val);
  }
  void TearDown() override { delete tree; }

  INT root_value;
  generic_tree<INT>* tree = nullptr;
  generic_node<INT>* ptrRoot = nullptr;
  // Keep pointers to some nodes for assertions
  generic_node<INT>*p22, *p11, *p6, *p2, *p8, *p15, *p18;
  generic_node<INT>*p12, *p9, *p5, *p7;
  generic_node<INT>*p27, *p21, *p25, *p26;
  generic_node<INT>*p77, *p68, *p69;
  generic_node<INT>*p67, *p54, *p56, *p59, *p62, *p65, *p71;
};

TEST_F(GenericTreeTest, TreeStructureIsCorrect) {
  ASSERT_NE(ptrRoot, nullptr);
  EXPECT_EQ(ptrRoot->data, 333);
  // Root should have 5 children
  EXPECT_EQ(ptrRoot->list_children.size(), 5);
  // Check some children values
  auto it = ptrRoot->list_children.begin();
  EXPECT_EQ((*it)->data, 22);
  ++it;
  EXPECT_EQ((*it)->data, 12);
  ++it;
  EXPECT_EQ((*it)->data, 27);
  ++it;
  EXPECT_EQ((*it)->data, 77);
  ++it;
  EXPECT_EQ((*it)->data, 67);
  // Check a deeper node
  EXPECT_EQ(p11->parent->data, 22);
  EXPECT_EQ(p6->parent->data, 11);
  EXPECT_EQ(p2->parent->data, 6);
  EXPECT_EQ(p8->parent->data, 11);
  EXPECT_EQ(p71->parent->data, 65);
}

// ===================== Single Node Tree =====================
TEST(GenericTreeEdgeCases, SingleNodeTree) {
  generic_tree<INT> tree(nullptr, 42);
  auto root = tree.get_root();
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->data, 42);
  EXPECT_EQ(root->parent, nullptr);
  EXPECT_TRUE(root->list_children.empty());
}

// ===================== Add Nodes =====================
TEST(GenericTreeOperations, AddChildToRoot) {
  generic_tree<INT> tree(nullptr, 1);
  auto root = tree.get_root();
  auto child = tree.add(root, 2);
  ASSERT_EQ(root->list_children.size(), 1);
  EXPECT_EQ((*root->list_children.begin())->data, 2);
  EXPECT_EQ(child->parent, root);
}

TEST(GenericTreeOperations, AddMultipleChildren) {
  generic_tree<INT> tree(nullptr, 10);
  auto root = tree.get_root();
  auto first_child = tree.add(root, 2);
  tree.add(root, 3);
  auto nested = tree.add(root->list_children.front(), 4);
  EXPECT_EQ((*root->list_children.begin())->data, 2);
  EXPECT_EQ((*std::next(root->list_children.begin()))->data, 3);
  EXPECT_EQ(nested->parent, first_child);
}

// ===================== Root Access =====================
TEST(GenericTreeRoot, RootIsAccessible) {
  generic_tree<INT> tree(nullptr, 123);
  auto root = tree.get_root();
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->data, 123);
}

// ===================== Print Helper Tests =====================
TEST(GenericTreePrint, IndentedPreorderPrint) {
  generic_tree<INT> tree(nullptr, 1);
  auto root = tree.get_root();
  auto c2 = tree.add(root, 2);
  tree.add(root, 3);
  tree.add(c2, 4);
  std::ostringstream oss;
  print_tree_indented(oss, root);
  std::string expected = "1\n  2\n    4\n  3\n";
  EXPECT_EQ(oss.str(), expected);
}

// ===================== Print/Serialization =====================
TEST(GenericTreePrint, PrintTreeIndentedOutput) {
  generic_tree<INT> tree(nullptr, 1);
  auto root = tree.get_root();
  tree.add(root, 2);
  tree.add(root, 3);
  tree.add(root->list_children.front(), 4);
  std::ostringstream oss;
  tree.print_tree(oss);
  std::string expected = "1\n  2\n    4\n  3\n";
  EXPECT_EQ(oss.str(), expected);
}

// ===================== Traversal =====================
TEST_F(GenericTreeTest, PreorderTraversalVisitsAllNodes) {
  std::vector<INT> visited;
  tree->traverse_preorder(
      ptrRoot, [&](generic_node<INT>* node) { visited.push_back(node->data); });
  EXPECT_EQ(visited.size(), tree->count_nodes());
  EXPECT_EQ(visited.front(), root_value);
}

TEST_F(GenericTreeTest, PostorderTraversalVisitsAllNodes) {
  std::vector<INT> visited;
  tree->traverse_postorder(
      ptrRoot, [&](generic_node<INT>* node) { visited.push_back(node->data); });
  EXPECT_EQ(visited.size(), tree->count_nodes());
  EXPECT_EQ(visited.back(), root_value);
}

// ===================== Removal =====================
TEST_F(GenericTreeTest, RemoveNodeRemovesSubtree) {
  int before = tree->count_nodes();
  tree->remove(p22);
  int after = tree->count_nodes();
  EXPECT_LT(after, before);
  // p22 should be gone
  EXPECT_EQ(tree->find(ptrRoot, 22), nullptr);
}

// ===================== Search =====================
TEST_F(GenericTreeTest, FindReturnsCorrectNode) {
  auto found = tree->find(ptrRoot, 71);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->data, 71);
  EXPECT_EQ(found, p71);
}

TEST_F(GenericTreeTest, FindReturnsNullptrIfNotFound) {
  auto found = tree->find(ptrRoot, 9999);
  EXPECT_EQ(found, nullptr);
}

// ===================== Counting =====================
TEST_F(GenericTreeTest, CountNodesAndLeaves) {
  int total = tree->count_nodes();
  int leaves = tree->count_leaves();
  int internals = tree->count_internal_nodes();
  EXPECT_EQ(total, leaves + internals);
  EXPECT_GT(total, 0);
  EXPECT_GT(leaves, 0);
  EXPECT_GT(internals, 0);
}

// ===================== Copy/Move Semantics =====================
TEST(GenericTreeCopyMove, CopyConstructorCreatesDeepCopy) {
  generic_tree<INT> tree(nullptr, 1);
  auto root = tree.get_root();
  tree.add(root, 2);
  tree.add(root, 3);
  tree.add(root->list_children.front(), 4);
  generic_tree<INT> copy(tree);
  std::ostringstream oss1, oss2;
  tree.print_tree(oss1);
  copy.print_tree(oss2);
  EXPECT_EQ(oss1.str(), oss2.str());
  // Mutate copy and check original is unchanged
  copy.add(copy.get_root(), 99);
  std::ostringstream oss3;
  copy.print_tree(oss3);
  EXPECT_NE(oss1.str(), oss3.str());
}

TEST(GenericTreeCopyMove, MoveConstructorTransfersOwnership) {
  generic_tree<INT> tree(nullptr, 1);
  auto root = tree.get_root();
  tree.add(root, 2);
  std::ostringstream oss1;
  tree.print_tree(oss1);
  generic_tree<INT> moved(std::move(tree));
  std::ostringstream oss2;
  moved.print_tree(oss2);
  EXPECT_EQ(oss1.str(), oss2.str());
  // Original should be empty (printing should be empty string)
  std::ostringstream oss3;
  tree.print_tree(oss3);
  EXPECT_EQ(oss3.str(), "");
}

TEST(GenericTreeCopyMove, CopyAssignmentCreatesDeepCopy) {
  generic_tree<INT> tree1(nullptr, 1);
  tree1.add(tree1.get_root(), 2);
  generic_tree<INT> tree2(nullptr, 10);
  tree2 = tree1;
  std::ostringstream oss1, oss2;
  tree1.print_tree(oss1);
  tree2.print_tree(oss2);
  EXPECT_EQ(oss1.str(), oss2.str());
}

TEST(GenericTreeCopyMove, MoveAssignmentTransfersOwnership) {
  generic_tree<INT> tree1(nullptr, 1);
  tree1.add(tree1.get_root(), 2);
  generic_tree<INT> tree2(nullptr, 10);
  tree2 = std::move(tree1);
  std::ostringstream oss1, oss2;
  tree2.print_tree(oss2);
  tree1.print_tree(oss1);
  EXPECT_EQ(oss1.str(), "");
  EXPECT_EQ(oss2.str(), "1\n  2\n");
}

// ===================== Additional Edge/Stress Tests =====================
TEST(GenericTreeEdgeCases, RemoveLeafNode) {
  generic_tree<INT> tree(nullptr, 1);
  auto root = tree.get_root();
  auto leaf = tree.add(root, 2);
  EXPECT_EQ(tree.count_nodes(), 2);
  tree.remove(leaf);
  EXPECT_EQ(tree.count_nodes(), 1);
  EXPECT_EQ(tree.find(root, 2), nullptr);
}

TEST(GenericTreeEdgeCases, RemoveInternalNode) {
  generic_tree<INT> tree(nullptr, 1);
  auto root = tree.get_root();
  auto c2 = tree.add(root, 2);
  auto c3 = tree.add(c2, 3);
  EXPECT_EQ(tree.count_nodes(), 3);
  tree.remove(c2);
  EXPECT_EQ(tree.count_nodes(), 1);
  EXPECT_EQ(tree.find(root, 2), nullptr);
  EXPECT_EQ(tree.find(root, 3), nullptr);
}

TEST(GenericTreeEdgeCases, RemoveRootNodeNoEffect) {
  generic_tree<INT> tree(nullptr, 1);
  auto root = tree.get_root();
  tree.remove(root);  // Should do nothing
  EXPECT_EQ(tree.count_nodes(), 1);
}

TEST(GenericTreeEdgeCases, SearchForDuplicateValues) {
  generic_tree<INT> tree(nullptr, 1);
  auto root = tree.get_root();
  auto c2 = tree.add(root, 2);
  auto c3 = tree.add(root, 2);  // duplicate value
  auto found = tree.find(root, 2);
  ASSERT_NE(found, nullptr);
  // Should find the first occurrence in preorder
  EXPECT_EQ(found, c2);
}

TEST(GenericTreeEdgeCases, PrintDeepTree) {
  generic_tree<INT> tree(nullptr, 0);
  auto node = tree.get_root();
  std::string expected = "0\n";
  for (int i = 1; i <= 10; ++i) {
    node = tree.add(node, i);
    expected += std::string(i * 2, ' ') + std::to_string(i) + "\n";
  }
  std::ostringstream oss;
  tree.print_tree(oss);
  EXPECT_EQ(oss.str(), expected);
}

TEST(GenericTreeCopyMove, CopyAfterMutation) {
  generic_tree<INT> tree(nullptr, 1);
  auto root = tree.get_root();
  tree.add(root, 2);
  generic_tree<INT> copy(tree);
  tree.add(root, 3);  // mutate original after copy
  std::ostringstream oss1, oss2;
  tree.print_tree(oss1);
  copy.print_tree(oss2);
  EXPECT_NE(oss1.str(), oss2.str());
}

TEST(GenericTreeCopyMove, MoveLargeTree) {
  generic_tree<INT> tree(nullptr, 0);
  auto node = tree.get_root();
  for (int i = 1; i <= 1000; ++i) {
    node = tree.add(node, i);
  }
  generic_tree<INT> moved(std::move(tree));
  EXPECT_EQ(moved.count_nodes(), 1001);
  EXPECT_EQ(tree.count_nodes(), 0);
}
