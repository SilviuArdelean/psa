#pragma once

#include <list>
#include <memory>

template <typename S>
struct generic_node {
  generic_node() = delete;

  generic_node(S& _data) : data(_data), parent(nullptr), level(0) {}

  generic_node(generic_node* const _parent, S const& _data)
      : data(_data), parent(_parent) {
    level = (parent) ? parent->level + 1 : 0;
  }

  generic_node(const generic_node& other)
      : data(other.data),
        listChildren(std::move(other.listChildren)),
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
  short level;
  generic_node<S>* parent;

  std::list<generic_node<S>*> listChildren;
};

template <typename T>
class generic_tree {
 public:
  generic_tree(generic_node<T>* _parent, T& root_value) {
    ptrRoot = _new_node(_parent, root_value);
  }

  ~generic_tree() { _tree_cleaner(ptrRoot); }

  generic_node<T>* add(generic_node<T>* _parent, T& _data) {
    generic_node<T>* pnew = _new_node(_parent, _data);
    _parent->listChildren.push_back(pnew);

    return pnew;
  }

  generic_node<T>* get_root() const { return ptrRoot; }

 protected:
  generic_node<T>* _new_node(generic_node<T>* _parent, T& _data) {
    return new generic_node<T>(_parent, _data);
  }

  void _tree_cleaner(generic_node<T>* node) {
    if (!node)
      return;

    for (auto& ob : node->listChildren)
      _tree_cleaner(ob);

    delete node;
  }

 protected:
  generic_node<T>* ptrRoot;
};