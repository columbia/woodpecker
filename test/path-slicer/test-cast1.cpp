#include <stdio.h>
#include <string.h>

int globalVar = 0;

void foo(long arg0) {
  int *p = (int *)arg0;
  globalVar += (*p);
  fprintf(stderr, "*p is %d\n", globalVar);
}

int main (int argc, char *argv[]) {
  foo((long)(argv[0]));
  return globalVar;
}

/*  Testing purpose: TBD.
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:


// CHECK: IntraSlicer::calStat STATISTICS:

