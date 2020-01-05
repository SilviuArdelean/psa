#pragma once
#include "general.h"

class smart_handle {
  HANDLE handle_;

 public:
  smart_handle(const HANDLE& h) : handle_(h) {}

  ~smart_handle() {
    if (nullptr != handle_) {
      CloseHandle(handle_);
      handle_ = nullptr;
    }
  }

  HANDLE get_handle() const { return handle_; }

  smart_handle(const smart_handle& rhs) : handle_(rhs.handle_) {}

  smart_handle& operator= (const HANDLE& h) {
    if (handle_ != &h) {
      handle_ = h;
    }
    return *this;
  }

  operator HANDLE() const { return handle_; }

  operator bool() const { return handle_ != nullptr; }
  bool operator!() const { return handle_ == nullptr; }
};
