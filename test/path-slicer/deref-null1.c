#include <stdio.h>
#include <assert.h>

int main(int argc, char *argv[]) {
  int i;
  for (i = 0; i < 10; i++) {
    if (i == 7) {
      char *c = NULL;
      int result = *c;
    }
  }
  return 0;
}

/*  Testing purpose: TBD.
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:
// CHECK: KLEE: ERROR: memory error: out of bound pointer

