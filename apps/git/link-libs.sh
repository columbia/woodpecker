#!/bin/bash

# Link the input bc with some uclibc libraries.

# List of libraries/micro-libraries:
# (1) getopt(), this is because it implicitly modify several linux-default pointers 
#      or integers, such as optarg. We have to link it in in order to make the 
#      slicing algorithm sound.
# (2) atexit(). Most coreutils programs use atexit(...), if we do not link in 
#      this function, we miss a call edge.
# (3) (Optional) all mem*.os in uclibc/libc/string/.  
# (4) (Optional) all str*.os in uclibc/libc/string/.  
# (5) error().

if [ -z $1 ]; then
        echo "Usage: <source file name: e.g., ./$0 a.bc>"
        exit 1
fi

ORIG_BC=$1

PROG=`echo $ORIG_BC | sed 's/\(.*\)\..*/\1/'`

#echo "Linking $PROG bc with some uclibc functions, please make sure the $ORIG_BC name has only one \".\" ";

mv $ORIG_BC tmp.o

llvm-ld -disable-opt -o atexit-libs \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/stdlib/atexit.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/stdlib/on_exit.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/stdlib/exit.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/stdlib/__cxa_atexit.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/stdlib/__exit_handler.os

llvm-ld -disable-opt -o $PROG tmp.o atexit-libs.bc

rm $PROG
rm tmp.o

opt -internalize < $PROG.bc > tmp2.bc
#opt -dse -functionattrs -memcpyopt -argpromotion -scalarrepl -inline -deadarghaX0r -jump-threading -prune-eh -ipsccp -globalopt -globalsmodref-aa -gvn -constmerge -instcombine -simplifycfg -adce -globaldce < $PROG.bc > tmp2.bc
cp tmp2.bc $PROG.bc


# Since we have link the bc module with *exit*.os with uclibc, we have to rename the exit() function.
opt -load $LLVM_ROOT/install/lib/libdsym-rename-exit.so --dsym-rename-exit < $PROG.bc > tmp.bc
mv tmp.bc $PROG.bc


