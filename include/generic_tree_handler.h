#pragma once

#include <mutex>
#include "general.h"
#include "generic_tree.h"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

class ProcsTreeBuilder;

template <typename T>
class generic_tree_handler {
 public:
  static void dfs_traverse(generic_node<T>* node, bool lastRN = false) {
    if (!node)
      return;

    SetupOutput();

    // list of Unicode characters
    // http://www.fileformat.info/info/unicode/category/So/list.htm

    if (node->level > 0) {
      if (node->level == 1) {
        if (!lastRN)
          ucout
              << _T("\u251C\u2500\u2500\u2500 ");  // ucout << "├───";
        else
          ucout
              << _T("\u2514\u2500\u2500\u2500 ");  // ucout << "└───";
      } else {
        for (auto i = 1; i < node->level; i++) {
          if (!lastRN)
            ucout << _T("\u2502   ");  // ucout << "|";
          else
            ucout << _T("    ");
        }

        ucout << _T("\u2514\u2500\u2500\u2500 ");  // ucout << "└───";
      }
    }

    ucout << node->data << std::endl;

    for (auto it = node->listChildren.begin(); it != node->listChildren.end();
         ++it) {
      auto it_temp = it;
      it_temp++;

      if (node->parent == nullptr && it_temp == node->listChildren.end()) {
        lastRN = true;  // the last child of the root
      }

      dfs_traverse(*it, lastRN);
    }
  }

  void dfs_traverse_nonstatic(generic_node<T>* node, bool lastRN = false) {
    if (!node)
      return;

    SetupOutput();

    if (node->level > 0) {
      if (node->level == 1) {
        if (!lastRN)
          ucout << _T("\u251C\u2500\u2500\u2500 ");  // "├───"
        else
          ucout << _T("\u2514\u2500\u2500\u2500 ");  // "└───"
      } else {
        for (auto i = 1; i < node->level; i++) {
          ucout << _T("    ");
        }

        ucout << _T("\u2514\u2500\u2500\u2500 ");  // "└───"
      }
    }

    if (parent)
      parent->PrintIt(node);
    else
      ucout << node->data;

    for (auto it = node->listChildren.begin(); it != node->listChildren.end();
         ++it) {
      auto it_temp = it;
      it_temp++;

      if (node->parent == nullptr && it_temp == node->listChildren.end()) {
        lastRN = true;  // the last child of the root
      }

      dfs_traverse_nonstatic(*it, lastRN);
    }
  }

  static void dfs_traverse_nonunicode(generic_node<T>* node) {
    if (!node)
      return;

    if (node->level > 0) {
      if (node->level == 1) {
        ucout << _T("|---");
      } else {
        ucout << _T("|");

        for (auto i = 0; i < node->level - 1; i++) {
          ucout << "     ";
        }

        ucout << _T("+--- ");
      }
    }

    ucout << node->data;

    for (auto it = node->listChildren.begin(); it != node->listChildren.end();
         ++it) {
      dfs_traverse_nonunicode(*it);
    }
  }

  void set_parent(ProcsTreeBuilder* _parent) { parent = _parent; }

 private:
  ProcsTreeBuilder* parent = nullptr;

  // Configures stdout for Unicode output — executed exactly once per process.
  static void SetupOutput() {
#ifdef _WIN32
    static std::once_flag flag;
    std::call_once(flag, [] { _setmode(_fileno(stdout), _O_U16TEXT); });
#endif
    // On Linux, stdout is UTF-8 by default — no setup needed.
  }
};
