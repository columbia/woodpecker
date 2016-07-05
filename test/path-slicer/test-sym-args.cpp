#include <stdio.h>

int main(int argc, char *argv[]) {
  return 0;
}

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc

// RUN: %kleebindir/klee --use-one-checker=OpenClose --use-path-slicer=1 --max-time 10 --libc=uclibc --posix-runtime --init-env \
// RUN: %s.bc --sym-args 1 10 10 --sym-files 2 2000 --max-fail 1 2> %s.output

// RUN: cat %s.output | FileCheck %s


// Expected results:

// CHECK: IntraSlicer::calStat STATISTICS

