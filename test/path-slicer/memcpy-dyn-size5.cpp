#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

long V0 = 0;
long V1 = 0;
long V2 = 0;

struct small_ptr_struct {
  long *p0;
};

void foo(int argc, char *argv[]) {
  int inputSize = atoi(argv[1]);
  fprintf(stderr, "inputSize %d\n", inputSize);
  struct small_ptr_struct ptr[3] = {{&V0}, {&V1}, {&V2}};		// This is not correctly handled by bc2bdd in comp_init yet.
  *(ptr[1].p0) += 9999;
}

int main (int argc, char *argv[]) {
  foo(argc, argv);
  fprintf(stderr, "V0 %ld, V1 %ld, V2 %ld\n", V0, V1, V2);
  return (int)(V1);
}

/*  Testing purpose: tbd.
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 16 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:


// CHECK: IntraSlicer::calStat STATISTICS:

