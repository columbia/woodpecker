#!/bin/bash

# Build the debug version of x86.

if [ -z $1 ]; then
        echo "Usage: <source file name: e.g., ./$0 tr.bc>"
        exit 1
fi

ORIG_BC=$1
PROG=`echo $ORIG_BC | sed 's/\(.*\)\..*/\1/'`
cp $PROG.bc tmp0.bc
opt -load $LLVM_ROOT/install/lib/libid-manager.so \
  -load $LLVM_ROOT/install/lib/libdsym-track-branch.so \
  --dsym-track-branch < tmp0.bc > tmp1.bc
opt -load $LLVM_ROOT/install/lib/libdsym-erase-uclibc-fini.so \
  --dsym-erase-uclibc-fini < tmp1.bc > tmp2.bc

llc tmp2.bc -o tmp2.s
gcc tmp2.s -o $PROG.x86 $DIRECT_SYM_ROOT/instruments/track-branch/func/track-branch-func.a -lstdc++ -ldl
cp tmp2.bc $PROG.bc.x86
rm tmp0.bc tmp1.bc tmp2.s

