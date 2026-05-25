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
        listChildren(other.listChildren),
        parent(other.parent),
        level(other.level) {}

  generic_node& operator=(const generic_node& rhs) {
    if (this != &rhs) {
      data = rhs.data;
      listChildren = std::move(rhs.listChildren);
      level = rhs.level;
    }

    return *this;
  }

  generic_node(generic_node&& other) {
    data = other.data;
    parent = other.parent;
    listChildren = std::move(other.listChildren);
    level = other.level;

    other.parent = nullptr;
    other.listChildren.clear();
    other.level = 0;
  }

  generic_node& operator=(generic_node&& other) {
    if (this != &other) {
      data = other.data;
      parent = other.parent;
      listChildren = std::move(other.listChildren);
      level = other.level;

      other.data = 0;
      other.parent = nullptr;
      other.listChildren.clear();
      other.level = 0;
    }

    return *this;
  }

  S data;
  std::list<generic_node<S>*> listChildren;
  generic_node<S>* parent = nullptr;
  int level;
};

template <typename T>
class generic_tree {
 public:
  generic_tree(generic_node<T>* _parent, const T& root_value) {
    root_ = new_node(_parent, root_value);
  }

  ~generic_tree() { tree_cleaner(root_); }

  generic_node<T>* add(generic_node<T>* _parent, const T& _data) {
    generic_node<T>* pnew = new_node(_parent, _data);
    _parent->listChildren.push_back(pnew);

    return pnew;
  }

  [[nodiscard]] generic_node<T>* get_root() const { return root_; }

 protected:
  generic_node<T>* new_node(generic_node<T>* _parent, const T& _data) {
    return new generic_node<T>(_parent, _data);
  }

  void tree_cleaner(generic_node<T>* node) {
    if (!node)
      return;

    for (auto& ob : node->listChildren)
      tree_cleaner(ob);

    delete node;
  }

 protected:
  generic_node<T>* root_;
};
