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

#include <gtest/gtest.h>

#include <string>

#include "notification.h"

TEST(NotificationHelpersTest, SanitizeText_StripsDangerousCharacters) {
  const std::string input = "hello;$USER|rm -rf / <bad> @ok!?():'_";
  const std::string sanitized = psa::system::detail::sanitize_text(input);

  EXPECT_EQ("helloUSERrm -rf  bad @ok!?():'_", sanitized);
  EXPECT_EQ(std::string::npos, sanitized.find(';'));
  EXPECT_EQ(std::string::npos, sanitized.find('|'));
  EXPECT_EQ(std::string::npos, sanitized.find('<'));
  EXPECT_EQ(std::string::npos, sanitized.find('>'));
  EXPECT_EQ(std::string::npos, sanitized.find('$'));
}

TEST(NotificationHelpersTest, SanitizeText_TruncatesLongInput) {
  const std::string input(250, 'a');
  const std::string sanitized = psa::system::detail::sanitize_text(input, 200);

  EXPECT_EQ(200u, sanitized.size());
  EXPECT_EQ(std::string(200, 'a'), sanitized);
}

#ifdef _WIN32
TEST(NotificationHelpersTest, BuildWindowsScript_EscapesEmbeddedSingleQuotes) {
  const std::string script =
      psa::system::detail::build_windows_script("O'Hara", "it's done");

  EXPECT_NE(std::string::npos, script.find("O''Hara"));
  EXPECT_NE(std::string::npos, script.find("it''s done"));
  EXPECT_EQ(std::string::npos, script.find("O'Hara"));
  EXPECT_EQ(std::string::npos, script.find("it's done"));
  EXPECT_NE(std::string::npos,
            script.find("[System.Windows.MessageBoxImage]::Warning"));
}
#endif

#ifdef __linux__
TEST(NotificationHelpersTest,
     BuildLinuxArguments_PreservesEmbeddedSingleQuotesWithoutShellEscaping) {
  const std::vector<std::string> arguments =
      psa::system::detail::build_linux_arguments("O'Hara", "it's done");

  ASSERT_EQ(3u, arguments.size());
  EXPECT_EQ("--icon=dialog-warning", arguments[0]);
  EXPECT_EQ("O'Hara", arguments[1]);
  EXPECT_EQ("it's done", arguments[2]);
}
#endif
