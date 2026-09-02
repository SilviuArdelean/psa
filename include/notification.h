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

#include <cstddef>
#include <string>
#include <vector>

// Kept free of platform headers so helpers can be unit-tested directly.
namespace psa::system {

bool show_notification(const std::string& title, const std::string& message);

}  // namespace psa::system

// Exposed for unit testing without spawning a shell or a UI dialog.
namespace psa::system::detail {

std::string sanitize_text(const std::string& input, size_t max_len = 200);

std::string build_windows_script(const std::string& title,
                                 const std::string& message);

std::vector<std::string> build_linux_arguments(const std::string& title,
                                               const std::string& message);

}  // namespace psa::system::detail
