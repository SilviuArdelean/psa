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

#include "pch.h"

#include "psa_cli_parsing.h"

namespace {

bool is_all_digits(const char* value) {
  if (!value || !*value)
    return false;

  while (*value) {
    if (!isdigit(static_cast<unsigned char>(*value++)))
      return false;
  }

  return true;
}

void normalize_windows_short_option(std::string& argument) {
#ifdef _WIN32
  if (argument.size() == 2 && argument[0] == '-' &&
      std::strchr("ADEKOT", argument[1])) {
    argument[1] = static_cast<char>(tolower(argument[1]));
  }
#else
  static_cast<void>(argument);
#endif
}

bool should_expand_numeric_short_option(const std::string& argument,
                                        int index,
                                        int argc,
                                        char* argv[]) {
  return (argument == "-e" || argument == "-t") && index + 1 < argc &&
         is_all_digits(argv[index + 1]);
}

std::string expand_numeric_short_option(const std::string& argument,
                                        const char* value) {
  const std::string longform = (argument == "-e") ? "--entries=" : "--tree=";
  return longform + value;
}

}  // namespace

namespace cli_parsing {

bool has_legacy_help_switch(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "-?") == 0) {
      return true;
    }
  }

  return false;
}

std::vector<std::string> normalize_arguments(int argc, char* argv[]) {
  std::vector<std::string> cooked_storage;
  cooked_storage.reserve(argc);

  for (int i = 0; i < argc; ++i) {
    std::string argument = argv[i];
    normalize_windows_short_option(argument);

    if (should_expand_numeric_short_option(argument, i, argc, argv)) {
      cooked_storage.push_back(
          expand_numeric_short_option(argument, argv[i + 1]));
      ++i;
      continue;
    }

    cooked_storage.push_back(std::move(argument));
  }

  return cooked_storage;
}

std::vector<const char*> build_argv_view(
    const std::vector<std::string>& cooked_storage) {
  std::vector<const char*> cooked_argv;
  cooked_argv.reserve(cooked_storage.size());

  for (const auto& argument : cooked_storage)
    cooked_argv.push_back(argument.c_str());

  return cooked_argv;
}

bool is_flag_like_value(const std::string& value) {
  return !value.empty() && value[0] == '-';
}

}  // namespace cli_parsing
