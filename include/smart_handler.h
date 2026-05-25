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
