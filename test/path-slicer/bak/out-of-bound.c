#include <stdio.h>
#include <assert.h>

int main(int argc, char *argv[]) {
  int a[5];
  int x = argc;
  if (x == 7) {
    fprintf(stderr, "Has an out of bound access!\n");
    fprintf(stderr, "Value (%d).\n", a[x]);
    //a[x]++;
  } else {
    fprintf(stderr, "Good.\n");
    a[0]++;
  }
  return 0;
}

// Testing commands:
// RUN: %srcroot/test/path-slicer/build.sh %s
// RUN: %kleebindir/klee %s.bc 1 2 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:
// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs:

