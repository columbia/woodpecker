#include <stdio.h>
#include <assert.h>

int main(int argc, char *argv[]) {
  if (argc < 3)
    exit(0);
  int i = atoi(argv[1]);
  int j = atoi(argv[2]);
  if (argc == 4) 
  for (i = 0; i < 1; i++) {
    for (j = 0; j < 1; j++) {
      /* If the last condition is argc == 4, then the bug could not be found within 10 minutes;
        if it is argc == 3, it could be found within one minute. Weird.
        I think the main reason is it spent too much time for KLEE to explore all paths in "argc == 3". */
      //if (/*i == 7 && j == 3 && */argc == 4) {
        char *c = NULL;
        int result = *c;
      //}
    }
  }
  return 0;
}

// Testing commands:
// RUN: %srcroot/test/path-slicer/build.sh %s
// RUN: %kleebindir/klee %s.bc 1 2 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:
// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs:

