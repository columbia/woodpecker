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
LINK_MEM_LIBS=$2
DISABLE_INLINING=$3
INLINING_FLAG=""

PROG=`echo $ORIG_BC | sed 's/\(.*\)\..*/\1/'`

#echo "Linking $PROG bc with some uclibc functions, please make sure the $ORIG_BC name has only one \".\" ";

mv $ORIG_BC tmp.o

MEM_LIBS="$DIRECT_SYM_ROOT/klee/uclibc/libc/string/memccpy.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/memchr.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/memcmp.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/memcpy.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/memmem.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/memmove.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/mempcpy.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/memrchr.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/memset.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strcasecmp_l.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strcat.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strcmp.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strdup.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strlcpy.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strncasecmp.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strncpy.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strpbrk.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strsignal.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strtok.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strxfrm.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strcasecmp.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strchrnul.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strcpy.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strerror.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strlen.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strncat.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strndup.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strrchr.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strspn.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strtok_r.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strcasestr.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strchr.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strcspn.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strlcat.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strncasecmp_l.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strncmp.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strnlen.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strsep.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strstr.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/string/strxfrm_l.os"

if [ "$LINK_MEM_LIBS" = "--link-mem-libs" ];
then
  echo "Linking with mem libs..."
else
  MEM_LIBS=""
fi

if [ "$DISABLE_INLINING" = "--disable-inlining" ];
then
  echo "Linking with disable inlining..."
  INLINING_FLAG="--disable-inlining"
else
  INLINING_FLAG=""
fi

llvm-ld $INLINING_FLAG -o $PROG tmp.o $MEM_LIBS \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/unistd/getopt.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/misc/error/error.os \
  $DIRECT_SYM_ROOT/klee/uclibc/libc/misc/gnu/obstack.os

rm $PROG
rm tmp.o

# Since we have link the bc module with *exit*.os with uclibc, we have to rename the exit() function.
#opt -load $LLVM_ROOT/install/lib/libdsym-rename-exit.so --dsym-rename-exit < $PROG.bc > tmp.bc
#mv tmp.bc $PROG.bc

#echo "Linking finished. The result bc is still $ORIG_BC";

