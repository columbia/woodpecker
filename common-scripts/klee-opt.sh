#!/bin/bash

# This script runs the optimization passes which KLEE will run on the input bc, 
# so the further passes run within KLEE will have no effect. This step is very 
# important because if some return instructions are removed and new return
# instructions are inserted by the CFG Simplification pass (for example), then
# the recording part can not know where the new inserted return instructions 
# come from and will cause slicing results to be incorrect.

# Currently we have these passes running on the bc module.
# 1. lowerinvoke: transform all invoke instructions to be call instructions, ease recording and slicing.
# 2. lowerswitch: transform all switch statements to be if statements, ease recording and slicing.
# 3. simplifycfg: simplify control flow graph, this is done within KLEE optimization passes.

# Usage: This script just run on the original bc and replace the new bc with 
# the old one (without changing file name).

if [ -z $1 ]; then
        echo "Usage: <bc name: e.g., ./klee-opt.sh tr.bc>"
        exit 1
fi

INPUTBC=$1;
cp $1 $1.orig;
opt -lowerinvoke -lowerswitch -simplifycfg < $1.orig > $1.orig2;
opt -load $LLVM_ROOT/install/lib/libdsym-lower-intrinsic.so --dsym-lower-intrinsic < $1.orig2 > $1;
rm $1.orig $1.orig2;

