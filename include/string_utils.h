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
#include <algorithm>
#include <codecvt>
#include <regex>
#include "general.h"

class string_utils {
 public:
  [[nodiscard]] static bool compare_case_sensitive(const ustring& strFirst,
                                     const ustring& strSecond) {
    return (0 == strFirst.compare(strSecond));
  }

  [[nodiscard]] static bool is_filename(const ustring& filename) {
#ifdef UNICODE
    const std::wregex pattern(_T("^([a-zA-Z0-9\\s._-]+)$"));
#else
    const std::regex pattern("^([a-zA-Z0-9\\s._-]+)$");
#endif

    return std::regex_match(filename.cbegin(), filename.cend(), pattern);
  }

  [[nodiscard]] static bool search_substring(const ustring& str,
                                             const ustring& sub_str,
                                             bool case_insensitive = true) {
    if (case_insensitive) {
#ifdef _WIN32
      ustring path2seach(str);
      ustring str4seach(sub_str);
      std::transform(path2seach.begin(), path2seach.end(), path2seach.begin(),
                     toupper);
      std::transform(str4seach.begin(), str4seach.end(), str4seach.begin(),
                     toupper);
      return path2seach.find(str4seach) != ustring::npos;
#else
      // By design: Linux processes and system environments are case-sensitive
      // by convention. We intentionally ignore the case sensitive flag here
      // to enforce strict matching on Linux.
      return str.find(sub_str) != ustring::npos;  // no copies needed on Linux
#endif
    }

    // Direct find — respects exact casing
    return (str.find(sub_str) != ustring::npos);
  }

  [[nodiscard]] static bool is_number(const ustring& s) {
    return (!s.empty() && std::find_if(s.begin(), s.end(), [](TCHAR c) {
                            return !(c >= _T('0') && c <= _T('9'));
                          }) == s.end());
  }

  static std::wstring utf8ToUtf16(const std::string& utf8Str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(utf8Str);
  }

  static std::string utf16ToUtf8(const std::wstring& utf16Str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(utf16Str);
  }
};
