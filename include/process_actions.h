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
#include <array>
#include <cstring>
#include <format>
#include <span>
#include <string>
#include <vector>
#include "general.h"
#include "string_utils.h"

#ifdef _WIN32
#include <winternl.h>
#include "smart_handler.h"
#else
#include <proc/readproc.h>
#include <signal.h>
#include <unistd.h>
#include <fstream>
#endif

#define process_fake_name _T("_|_")

namespace process_actions {
void kill_process_by_name(const TCHAR* process_name);
void kill_process_by_pid(const int process_pid);
void kill_process_by_name_optimized(const TCHAR* process_name,
                                    const procs_map& map_processes);
void kill_process_by_pid_optimized(const int process_pid,
                                   const procs_map& map_processes);
ustring GetProcessPath(int process_pid);
[[nodiscard]] ustring GetProcessCmdLine(int process_pid);

bool is_essential_proccess(const int process_pid);
void execute_kill_process(const int process_pid, const TCHAR* process_name);
void execute_kill_process_optimized(const int process_pid,
                                    const TCHAR* process_name,
                                    const procs_map& map_processes);
#ifdef __linux__
ssize_t get_exe_for_pid(int pid, std::span<char> buf);
#endif
}  // namespace process_actions
