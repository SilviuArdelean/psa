# Build and Test Guide

This document collects the supported ways to configure, build, and test psa on Windows and Linux, plus the current status for macOS.

## 1. Requirements

### Common

- CMake 3.16 or newer
- A C++20-capable compiler
- Network access during CMake configure, because the build fetches `fmt` and GoogleTest with `FetchContent`

### Windows

- Visual Studio 2019 or newer
- MSVC with C++20 support
- Recommended shells:
  - Developer PowerShell for Visual Studio
  - Developer Command Prompt for Visual Studio
  - `cmd.exe` also works if CMake and the compiler toolchain are already on `PATH`

### Linux

- GCC 10+ or Clang 11+
- `build-essential`
- `cmake`
- `pkg-config`
- `libprocps-dev`

Install Linux prerequisites on Debian/Ubuntu:

```sh
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config libprocps-dev
```

### macOS

- CMake can generate a build on macOS.
- The project is not fully ported yet. `src/CMakeLists.txt` already marks macOS process-listing support as requiring additional porting.
- Treat macOS as experimental until process APIs are implemented.

## 2. Repository Setup

Clone the repository and initialize submodules if you want the Visual Studio solution/submodule workflow:

```sh
git clone <repo-url>
cd psa
git submodule update --init --recursive
```

Notes:

- The CMake build fetches GoogleTest automatically, so the submodule is not strictly required for CMake builds.
- The Visual Studio solution files in the repository are easier to use if the submodule is present under `external/gtest`.

## 3. CMake Builds

The root `CMakeLists.txt` configures:

- the main executable target: `psa`
- the CLI/unit/integration test target: `test_psa_cli`
- the generic tree test target: `test_generic_tree`

### Recommended practice

- Use a fresh build directory per configuration, for example `build/debug`, `build/release`, or `build/notify-validate`.
- If CMake reports that `CMakeCache.txt` points to an older source path, delete that build directory and configure again.

### Configure and build with CMake on Windows

From `cmd.exe`:

```bat
cd /d D:\sources\silviu\psa
cmake -S . -B build\Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build\Debug --parallel
```

From PowerShell:

```powershell
Set-Location 'D:\sources\silviu\psa'
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug --parallel
```

For a Release build:

```bat
cd /d D:\sources\silviu\psa
cmake -S . -B build\Release -DCMAKE_BUILD_TYPE=Release
cmake --build build\Release --parallel
```

Windows output notes:

- Debug executable: `build\<dir>\bin\Debug\psad.exe`
- Release executable: `build\<dir>\bin\Release\psa.exe`

### Configure and build with CMake on Linux

```sh
cd /path/to/psa
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --parallel
```

For Release:

```sh
cd /path/to/psa
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel
```

Linux output notes:

- Debug executable: `build/debug/bin/psa`
- Release executable: `build/release/bin/psa`

### Configure on macOS later

When macOS support is completed, the expected CMake workflow will be the same:

```sh
cd /path/to/psa
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --parallel
```

Today this only covers build system generation. Runtime support is not complete yet.

## 4. Visual Studio Workflow on Windows

You can work with the repository in Visual Studio in two ways.

### Option A: Open the existing solution

- Open `psa.sln`
- Select `Debug` or `Release`
- Build the solution

This path is useful if you want to inspect the older Visual Studio project structure already stored in the repository.

### Option B: Open the folder and use CMake

- Open the repository folder in Visual Studio or VS Code
- Let CMake configure the project
- Build the `psa`, `test_psa_cli`, or `test_generic_tree` targets

This is the workflow that best matches the current root `CMakeLists.txt`.

## 5. Running Tests

The project registers tests with CTest using `gtest_discover_tests(...)`.

### Run all tests with CTest on Windows

```bat
cd /d D:\sources\silviu\psa
ctest --test-dir build\Debug -C Debug --output-on-failure
```

If you configured a different build directory, replace `build\Debug` with that directory.

### Run all tests with CTest on Linux

```sh
cd /path/to/psa
ctest --test-dir build/debug --output-on-failure
```

### Build only one test target

CLI tests:

```bat
cd /d D:\sources\silviu\psa
cmake --build build\Debug --target test_psa_cli --parallel
```

Generic tree tests:

```bat
cd /d D:\sources\silviu\psa
cmake --build build\Debug --target test_generic_tree --parallel
```

Equivalent Linux commands:

```sh
cd /path/to/psa
cmake --build build/debug --target test_psa_cli --parallel
cmake --build build/debug --target test_generic_tree --parallel
```

### Run one GTest target directly

Windows:

```bat
cd /d D:\sources\silviu\psa
build\Debug\bin\Debug\test_psa_cli.exe
build\Debug\bin\Debug\test_generic_tree.exe
```

Linux:

```sh
cd /path/to/psa
./build/debug/bin/test_psa_cli
./build/debug/bin/test_generic_tree
```

### Run only the notification-related tests

This is useful when working on the `--notify` feature.

Windows `cmd.exe` example:

```bat
cd /d D:\sources\silviu\psa
cmake -S . -B build\notify-validate -DCMAKE_BUILD_TYPE=Debug
cmake --build build\notify-validate --target test_psa_cli --parallel
build\notify-validate\bin\Debug\test_psa_cli.exe --gtest_filter=*Notify*:*NotificationHelpersTest*
```

PowerShell example:

```powershell
Set-Location 'D:\sources\silviu\psa'
cmake -S . -B build/notify-validate -DCMAKE_BUILD_TYPE=Debug
cmake --build build/notify-validate --target test_psa_cli --parallel
.\Build\notify-validate\bin\Debug\test_psa_cli.exe --gtest_filter=*Notify*:*NotificationHelpersTest*
```

Linux example:

```sh
cd /path/to/psa
cmake -S . -B build/notify-validate -DCMAKE_BUILD_TYPE=Debug
cmake --build build/notify-validate --target test_psa_cli --parallel
./build/notify-validate/bin/test_psa_cli --gtest_filter='*Notify*:*NotificationHelpersTest*'
```

## 6. Running the Application

### Windows

Debug build:

```bat
cd /d D:\sources\silviu\psa
build\Debug\bin\Debug\psad.exe -a
build\Debug\bin\Debug\psad.exe --notify
build\Debug\bin\Debug\psad.exe --notify "hello from cmd"
```

Release build:

```bat
cd /d D:\sources\silviu\psa
build\Release\bin\Release\psa.exe -a
```

### Linux

```sh
cd /path/to/psa
./build/debug/bin/psa -a
./build/debug/bin/psa --notify "hello"
```

Notification note on Linux:

- `--notify` uses `notify-send`
- it requires a running desktop notification service
- it may fail in plain SSH sessions without `DISPLAY` or DBus session forwarding

## 7. VS Code Tasks

The workspace already contains tasks for common CMake flows, including:

- configure debug
- build debug
- configure release
- build release
- run all tests with CTest

If you are working in VS Code, these tasks are the quickest way to repeat the standard flows without typing commands manually.

## 8. Troubleshooting

### Stale CMake cache after moving or renaming the repo

Symptom:

- CMake says the current `CMakeCache.txt` directory differs from the directory where the cache was created.

Fix:

- delete the affected build directory
- or configure into a brand new build directory

Example:

```bat
cd /d D:\sources\silviu\psa
rmdir /s /q build\Debug
cmake -S . -B build\Debug -DCMAKE_BUILD_TYPE=Debug
```

### Linux configure failure for `libprocps`

Symptom:

- CMake cannot find `libprocps`
- `pkg_check_modules(PROCPS REQUIRED libprocps)` fails

Fix:

```sh
sudo apt-get install pkg-config libprocps-dev
```

### Notification feature does not show anything

Windows:

- the notification wrapper uses a PowerShell-backed dialog
- it needs an interactive desktop session

Linux:

- `notify-send` must be installed and reachable on `PATH`
- a notification daemon must be running in the current desktop session
