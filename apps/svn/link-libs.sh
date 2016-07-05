#!/bin/bash

# Link the input bc with some uclibc libraries.

# List of libraries/micro-libraries:
# (1) getopt(), this is because it implicitly modify several linux-default pointers 
#      or integers, such as optarg. We have to link it in in order to make the 
#      slicing algorithm sound.

if [ -z $1 ]; then
        echo "Usage: <source file name: e.g., ./$0 cvs.bc>"
        exit 1
fi

ORIG_BC=$1
PROG=`echo $ORIG_BC | sed 's/\(.*\)\..*/\1/'`

#echo "Linking $PROG bc with some uclibc functions, please make sure the $ORIG_BC name has only one \".\" ";

mv $ORIG_BC tmp.o

llvm-ld -o $PROG tmp.o \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/unistd/getopt.os

rm $PROG
rm tmp.o

#echo "Linking finished. The result bc is still $ORIG_BC";

