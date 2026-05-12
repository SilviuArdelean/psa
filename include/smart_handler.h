#pragma once
#include "general.h"

class smart_handle {
  HANDLE handle_;

  static bool is_valid(HANDLE h) noexcept {
    return h != nullptr && h != INVALID_HANDLE_VALUE;
  }

 public:
  smart_handle(const HANDLE& h) : handle_(h) {}

  ~smart_handle() {
    if (is_valid(handle_)) {
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
      if (is_valid(handle_))
        CloseHandle(handle_);
      handle_ = rhs.handle_;
      rhs.handle_ = nullptr;
    }
    return *this;
  }

  operator HANDLE() const { return handle_; }

  operator bool() const { return is_valid(handle_); }
  bool operator!() const { return !is_valid(handle_); }
};
