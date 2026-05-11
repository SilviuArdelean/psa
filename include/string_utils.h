#pragma once
#include <algorithm>
#include <codecvt>
#include <regex>
#include "general.h"

class string_utils {
 public:
  static bool compare_case_sensitive(const ustring& strFirst,
                                     const ustring& strSecond) {
    return (0 == strFirst.compare(strSecond));
  }

  static bool is_filename(const ustring& filename) {
#ifdef UNICODE
    const std::wregex pattern(_T("^([a-zA-Z0-9s._-]+)$"));
#else
    const std::regex pattern("^([a-zA-Z0-9s._-]+)$");
#endif

    return std::regex_match(filename.cbegin(), filename.cend(), pattern);
  }

  static bool search_substring(const ustring& str,
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
      return str.find(sub_str) != ustring::npos;  // no copies needed on Linux
#endif
    }

    // Direct find — respects exact casing
    return (str.find(sub_str) != ustring::npos);
  }

  static bool is_number(const ustring& s) {
    return (!s.empty() && std::find_if(s.begin(), s.end(), [](TCHAR c) {
                            return !std::isdigit(c);
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
