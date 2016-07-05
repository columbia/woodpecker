1 External dependent projects: llvm, xtern.
Please checkout and install these projects based on their "readme".
Please build (make install) llvm/common, llvm/bc2bdd, and llvm/aaol as well.

2 Set these environment Variables correctly.
> export DIRECT_SYM_ROOT
> export LLVM_ROOT
> export APPS_DIR
> export XTERN_ROOT

3 Configure direct-sym and direct-sym/klee
> cd $DIRECT_SYM_ROOT/
> ./configure \
	--with-llvmsrc=$LLVM_ROOT/llvm-2.7/ \
	--with-llvmobj=$LLVM_ROOT/llvm-obj/ \
	--prefix=$LLVM_ROOT/install/

You must use either "Debug" or "Release" to replace "<Debug or Release>"
in the configure command below consistently with the mode of LLVM you built.

> cd DIRECT_SYM_ROOT/klee/
> ./configure \
	--with-llvmsrc=$LLVM_ROOT/llvm-2.7/ \
	--with-llvmobj=$LLVM_ROOT/llvm-obj \
	--with-llvm-build-mode=<Debug or Release> \
	--with-runtime=<Debug or Release> \
	--with-uclibc=$DIRECT_SYM_ROOT/klee/uclibc \
	--enable-posix-runtime

4 Make xtern and direct-sym
You must configure direct-sym/klee before you build xtern,
and you must build xtern before you build direct-sym.

> cd $XTERN_ROOT
(checkout the "heming" branch from git, and follow the readme.txt
in $XTERN_ROOT to configure and build it).

> cd $DIRECT_SYM_ROOT
> make ENABLE_OPTIMIZED=0/1

5 Run our testsuite to make sure you have installed everything correctly.
This may take a few minutes.
> cd $DIRECT_SYM_ROOT
> make ENABLE_OPTIMIZED=0/1 -C test check

6 Recommendations (for direct-sym evaluation/development only)
add this to your ~/.bashrc so that you can diagnose the calls to 
critical libc functions such as fopen() and fclose(). 
> export DYN_INTERCEPT=$DIRECT_SYM_ROOT/instruments/dyn-intercept-calls/dyn-intercept-calls.so
Usage:
> LD_PRELOAD=$DYN_INTERCEPT ./a.out <args>

7 Other systems settings.
Plese enlarge your "ulimit -s" in your ~/.bashrc to be 819200
so that KLEE will not run out of stack space.
And please set "ulimit -n 32768" so that we do not run out of
file descriptors in our symbolic file system (you can set this
in /root/.bashrc and switch to root to run the eval-path-slicer-coverage.pl
script).

8 Run our applications. Use coreutils as an example.
Build coreutils:
> cd $APPS_DIR && git pull
> cd $DIRECT_SYM_ROOT/apps/coreutils/ && ./mk-llvm 8.12

Run coreutils:
Please refer to $DIRECT_SYM_ROOT/eval/eval-path-slicer-coverage.pl on 
how to launch the evaluation scripts. A sample run is (run "File" checker,
"both" real prune and mark pruned, from the "0th to 20th" program sorted by
alphabet in "/home/heming/rcs/direct-sym/apps/coreutils/" absolute directory,
each run is "3600" seconds):
> cd $DIRECT_SYM_ROOT/eval/
> ./eval-path-slicer-coverage.pl 3600 /home/heming/rcs/direct-sym/apps/coreutils/ both File 0 20

For each coreutils, the formatted results look like this (this format is easy to
shown on our wiki page):

|| App || Checker || Mark/Real prune || Finished || All time (sec) || Path 
slicer time || Init time || Pruned states || All states (paths) || \# Tests 
|| \# Instrs exed || \# Not pruned Instrs exed || \# Not pruned internal 
Instrs exed || \# Static Instrs exed || \# Static Instrs || \# Static exed 
events || \# Static events (no fp) || \# Chkr errs || Note ||

| base64.bc | OpenClose | Mark | *No* | 3636.34 | 1.48957 | 9.63395 | 5176 | 
5312 | 91 | 11862689 | *1369538* | 25831 | 1327 | 2126 | 2 | 2 | 2 | tbd |

| base64.bc | OpenClose | Real | *Yes* | 188.402 | 3.76802 | 9.92034 | 470 | 
688 | 37 | 3706298 | *3706298* | 91113 | 1124 | 2126 | 2 | 2 | 5 | tbd |

