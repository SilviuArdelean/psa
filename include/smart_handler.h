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

  // Non-copyable: copying a HANDLE would cause double-close in destructors
  smart_handle(const smart_handle&) = delete;
  smart_handle& operator=(const smart_handle&) = delete;

  // Move constructor: transfers ownership, nulls out the source
  smart_handle(smart_handle&& rhs) noexcept : handle_(rhs.handle_) {
    rhs.handle_ = nullptr;
  }

  // Move assignment: closes existing handle before taking ownership
  smart_handle& operator=(smart_handle&& rhs) noexcept {
    if (this != &rhs) {
      if (nullptr != handle_)
        CloseHandle(handle_);
      handle_ = rhs.handle_;
      rhs.handle_ = nullptr;
    }
    return *this;
  }

  operator HANDLE() const { return handle_; }

  operator bool() const { return handle_ != nullptr; }
  bool operator!() const { return handle_ == nullptr; }
};
