### psa
Processes Status Analysis  
version 0.2  
------------------------------------------
Processes Status Analysis (psa) cross-platform application that allows different analyses over the operating system's processes.

Parameters
------------------------------------------
* -a        : list all processes information from current processes snapshot. 
* -e [no]   : top [no] most expensive memory consuming processes | top 10 by default  
* -k        : kill specific process by PID  
* -o        : info only one process name criteria  
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
```

Compatibility
------------------------------------------
The currently supported operating systems:
* Windows
* Linux    - (Debian tested) requiers libprocps-dev library

Contributing
------------
People on the project: @slymaximus (Silviu-Marius Ardelean)
