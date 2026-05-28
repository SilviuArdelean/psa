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

#include <list>
#include <memory>
#include <ostream>

template <typename S>
struct generic_node {
  generic_node() = delete;

  generic_node(const S& _data) : data(_data), parent(nullptr), level(0) {}

  generic_node(generic_node* const _parent, S const& _data)
      : data(_data), parent(_parent) {
    level = (parent) ? parent->level + 1 : 0;
  }

  generic_node(const generic_node& other)
      : data(other.data),
        list_children(other.list_children),
        parent(other.parent),
        level(other.level) {}

  generic_node& operator=(const generic_node& rhs) {
    if (this != &rhs) {
      data = rhs.data;
      list_children = rhs.list_children;
      parent = rhs.parent;
      level = rhs.level;
    }

    return *this;
  }

  generic_node(generic_node&& other) noexcept
      : data(std::move(other.data)),
        list_children(std::move(other.list_children)),
        parent(other.parent),
        level(other.level) {
    other.parent = nullptr;
    other.list_children.clear();
    other.level = 0;
  }

  generic_node& operator=(generic_node&& other) noexcept {
    if (this != &other) {
      data = std::move(other.data);
      parent = other.parent;
      list_children = std::move(other.list_children);
      level = other.level;

      other.parent = nullptr;
      other.list_children.clear();
      other.level = 0;
    }

    return *this;
  }

  S data;
  std::list<generic_node<S>*> list_children;
  generic_node<S>* parent = nullptr;
  int level;
};

template <typename T>
class generic_tree {
 public:
  generic_tree(generic_node<T>* _parent, const T& root_value) {
    root_ = new_node(_parent, root_value);
  }

  generic_tree(const generic_tree& other) {
    root_ = copy_subtree(nullptr, other.root_);
  }

  generic_tree(generic_tree&& other) noexcept {
    root_ = other.root_;
    other.root_ = nullptr;
  }

  generic_tree& operator=(const generic_tree& other) {
    if (this != &other) {
      generic_node<T>* new_root = copy_subtree(nullptr, other.root_);
      tree_cleaner(root_);
      root_ = new_root;
    }
    return *this;
  }

  generic_tree& operator=(generic_tree&& other) noexcept {
    if (this != &other) {
      tree_cleaner(root_);
      root_ = other.root_;
      other.root_ = nullptr;
    }
    return *this;
  }

  ~generic_tree() { tree_cleaner(root_); }

 private:
  generic_node<T>* copy_subtree(generic_node<T>* parent,
                                const generic_node<T>* node) {
    if (!node)
      return nullptr;
    auto* new_node_ptr = new generic_node<T>(parent, node->data);
    for (auto child : node->list_children) {
      auto* child_copy = copy_subtree(new_node_ptr, child);
      new_node_ptr->list_children.push_back(child_copy);
    }
    return new_node_ptr;
  }

 public:
  generic_node<T>* add(generic_node<T>* _parent, const T& _data) {
    generic_node<T>* pnew = new_node(_parent, _data);
    _parent->list_children.push_back(pnew);
    return pnew;
  }

  [[nodiscard]] generic_node<T>* get_root() const { return root_; }

  template <typename F>
  void traverse_preorder(generic_node<T>* node, F&& func) {
    if (!node)
      return;
    func(node);
    for (auto child : node->list_children) {
      traverse_preorder(child, func);
    }
  }

  template <typename F>
  void traverse_postorder(generic_node<T>* node, F&& func) {
    if (!node)
      return;
    for (auto child : node->list_children) {
      traverse_postorder(child, func);
    }
    func(node);
  }

  void remove(generic_node<T>* node) {
    if (!node || node == root_)
      return;
    auto parent = node->parent;
    if (!parent)
      return;
    parent->list_children.remove(node);
    tree_cleaner(node);
  }

  generic_node<T>* find(generic_node<T>* node, const T& value) {
    if (!node)
      return nullptr;
    if (node->data == value)
      return node;
    for (auto child : node->list_children) {
      auto found = find(child, value);
      if (found)
        return found;
    }
    return nullptr;
  }

  int count_nodes(generic_node<T>* node) {
    if (!node)
      return 0;
    int count = 1;
    for (auto child : node->list_children) {
      count += count_nodes(child);
    }
    return count;
  }

  int count_nodes() { return count_nodes(root_); }

  int count_leaves(generic_node<T>* node) {
    if (!node)
      return 0;
    if (node->list_children.empty())
      return 1;
    int count = 0;
    for (auto child : node->list_children) {
      count += count_leaves(child);
    }
    return count;
  }

  int count_leaves() { return count_leaves(root_); }

  int count_internal_nodes(generic_node<T>* node) {
    if (!node || node->list_children.empty())
      return 0;
    int count = 1;
    for (auto child : node->list_children) {
      count += count_internal_nodes(child);
    }
    return count;
  }

  int count_internal_nodes() { return count_internal_nodes(root_); }

  void print_tree(std::ostream& os) const { print_tree_node(os, root_, 0); }

 private:
  void print_tree_node(std::ostream& os,
                       generic_node<T>* node,
                       int indent) const {
    if (!node)
      return;
    for (int i = 0; i < indent; ++i)
      os << "  ";
    os << node->data << "\n";
    for (auto child : node->list_children) {
      print_tree_node(os, child, indent + 1);
    }
  }

 protected:
  generic_node<T>* new_node(generic_node<T>* _parent, const T& _data) {
    return new generic_node<T>(_parent, _data);
  }

  void tree_cleaner(generic_node<T>* node) {
    if (!node)
      return;

    for (auto& ob : node->list_children)
      tree_cleaner(ob);

    delete node;
  }

  generic_node<T>* root_;
};
