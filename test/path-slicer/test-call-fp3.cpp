#include <stdio.h>
#include <memory.h>

int V1 = 0;

void bar1(int i) {
  V1++;
}

struct small_commonio_ops {
  void (*fp_1)(int);
}; 

/* This function call to foo() is critical for this testcase, because we need a calling context in order
to reproduce this bug. If everything is in the main() function, then the passed in calling context to bc2bdd
would be NULL, then it becomes context insensitve, then the bug can not be captured. */
void foo(struct small_commonio_ops *ops, int argc, char *argv[]) {
  ops->fp_1(argc);
}

int main (int argc, char *argv[]) {
  struct small_commonio_ops group_ops2;// = {bar1};
  group_ops2.fp_1 = bar1;
  foo(&group_ops2, argc, argv);
  //group_ops2.fp_1(argc);
  fprintf(stderr, "V1 %d\n", V1);
  return V1;
}

/*  Testing purpose: This testcase has reproduced the bc2bdd problem: can not capture function pointer assignment in global scope.
    The taken slice does not contain any instruction in the bar1() function, because the bdd of "globalVar" has missed.
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:



// CHECK: IntraSlicer::calStat STATISTICS



