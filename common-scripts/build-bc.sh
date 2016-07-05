#!/bin/bash

# This is a script to compile a c/cpp file into LLVM bc file.

if [ -z $1 ]; then
        echo "Usage: <source file name: e.g., ./build-bc.sh tr.c>"
        exit 1
fi

SRC=$1
PROG=`echo $SRC | sed 's/\(.*\)\..*/\1/'`
llgcc $SRC -o $PROG &> /dev/null
mv $PROG.bc $SRC.bc

