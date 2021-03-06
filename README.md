# HCCT - Hot Calling Context Tree
*A space-efficient hot calling-context profiler*

**This page is under construction, as the project has just been moved from Google Code!**

HCCT is a performance profiler that leverages efficient streaming algorithms to build a relevant subset of the Calling Context Tree using limited memory resources. This subset, formally known as Hot Calling Context Tree, includes only those contexts that are representative of hot spots and possibly source of performance bottlenecks during the execution.

This tool works on any 32-bit Linux distribution and is based on the gcc 4.x compiler.

Check out the code and have a look at *profiler/README* to get started!

We have also developed a library for solving addresses to symbols, and a set of tools for the analysis and comparison of CCTs/HCCTs resulting from distinct executions. These components are available in the *analysis/* folder.

The main ideas behind this tool are described in the paper [Mining hot calling contexts in small space](http://dx.doi.org/10.1145/1993316.1993559) appeared at PLDI '11 (*Proceedings of the 32nd ACM SIGPLAN conference on Programming language design and implementation*).

This tool has originally been written as part of the Master's Thesis of one of the authors, and the original release of the code is available under the *thesis* folder. If you're not familiar with context-sensitive profiling techniques and data streaming algorithms, you can download the thesis [here](http://www.dis.uniroma1.it/~delia/files/thesis.pdf).
