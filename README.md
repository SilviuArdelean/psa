### psa

Processes Status Analysis
version 0.4
------------------------------------------

Processes Status Analysis (psa) cross-platform application that allows different analyses over the operating system's processes.

Parameters
------------------------------------------

* -a        : list all processes information from current processes snapshot.
* -e [no]   : top [no] most expensive memory consuming processes | top 10 by default
* -k        : kill specific process by PID or name
* --filter-param : filter processes to kill by command line substring (used with -k)
* -o        : info only one process name criteria (can be used with -d/--details)
* -d, --details : show detailed process information (used with -o)
* --pid <pid> : identify process information by PID (full process report)
* -t [pid]  : tree snapshot of current processes or of the subprocesses of a specified process PID.

Usage
------------------------------------------

All the results can be redirected within a file.
Usage examples.

```
psa -t                                    // full snapshot tree
psa -t 768                                // tree with the children processes of the process PID 768
psa -o chrome                             // find how much memory uses your Chrome!   o_O
psa -t > processes_tree.txt               // full snapshot tree redirection to a file
psa -e 20 > top_expensive_processes.txt   // top most 'expensive' processes and save information in a file
psa -k notep                              // kill all the processs containing 'notep' within the process name
psa -k 7891                               // kill the process having PID = 7891
psa -k chrome --filter-param "network"    // kill all processes containing 'chrome' in name and 'network' in command line
psa -o chrome --details                   // show details and command line for all processes matching 'chrome'
psa -o 1234 --details                     // show details and command line for process with PID 1234
psa -od 1234                              // short form with PID filter
psa -o -d 1234                            // equivalent short form with PID filter
psa --pid 1234                            // print full process report for PID 1234
```

Compatibility
------------------------------------------

The currently supported operating systems:

* Windows
* Linux    - (Debian tested) requiers libprocps-dev library

GoogleTest Submodule
--------------------

This repository uses GoogleTest as a submodule for unit testing. After cloning, run:

    git submodule update --init --recursive

to fetch the test dependencies into `external/gtest`.

If you are updating or changing the submodule, commit the `.gitmodules` file and the submodule reference, but do not commit the contents of `external/gtest` itself.

Development Environment
----------------------

**Compiler Requirements:**

* A C++20-compliant compiler is required (e.g., MSVC 2019/2022, GCC 10+, Clang 11+).

**Windows:**

* Visual Studio 2019 or newer recommended.
* Enable C++20 in your project settings.

**Linux:**

* GCC 10+ or Clang 11+ required.
* Install build essentials and required libraries:

  ```sh
  sudo apt-get update
  sudo apt-get install build-essential cmake libprocps-dev
  ```

* To build with CMake:

  ```sh
  mkdir build && cd build
  cmake .. -DCMAKE_CXX_STANDARD=20
  make
  ```

Unit Testing
------------

Unit tests in this project use GoogleTest. You can set up GoogleTest in one of two ways:

1. **Submodule (recommended for Visual Studio users):**
   * After cloning, run:
     `git submodule update --init --recursive`
   * This will fetch GoogleTest into `external/gtest` for direct Visual Studio integration.

2. **CMake (for CMake-based builds):**
   * CMake will automatically download and build GoogleTest as needed when you configure the project.

If you use the submodule, do not commit the contents of `external/gtest`—only the submodule reference and `.gitmodules` file.

Contributing
------------

People on the project: @slymaximus (Silviu-Marius Ardelean)
