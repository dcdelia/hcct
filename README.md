# HCCT - Hot Calling Context Tree.
A space-efficient hot calling-context profiler.

The following text might be inaccurate, as the project has just been moved from Google Code.

A performance profiler based on efficient streaming algorithms to build a relevant subset of the Calling Context Tree of a program that includes only those contexts that are likely to represent hot spots/performance bottlenecks in the execution.

Check out the 'source' folder and have a look at README to get started! This tool works on any 32-bit Linux distribution.

The main ideas behind this tool are described in a paper, [Mining hot calling contexts in small space](http://dx.doi.org/10.1145/1993316.1993559), that appeared in Proceedings of the 32nd ACM SIGPLAN conference on Programming language design and implementation (PLDI '11). The paper is available [here](http://www.dis.uniroma1.it/~demetres/didattica/ae/upload/papers/pldi149-delia.pdf) for download.

This tool has been written as part of Master's Thesis of one of the authors. Even though some parts are partially out-of-date, if you are not familiar with context-sensitive profiling techniques and data streaming algorithms please have a look at it (available [here](http://www.dis.uniroma1.it/~delia/files/thesis.pdf here)).


