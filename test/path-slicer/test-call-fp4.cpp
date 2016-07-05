#include <stdio.h>
#include <memory.h>

int V1 = 0;
int V2 = 0;
int V3 = 0;

void bar1(int i) {
  if (i < 4)
    V1++;
}
/*
void bar2(const char *argv_i) {
  if (!strstr(argv_i, "fp3"))
    V2++;
}

void bar3(int argc, char **argv) {
  if (strstr(argv[0], "fp3"))
    V3++;
}
*/
struct small_commonio_ops {
  void (*fp_1)(int);
};
/*
struct commonio_ops {
  void (*fp_2)(const char *);
  void (*fp_1)(int);
  void (*fp_3)(int, char **);
};

struct commonio_ops group_ops = {
  bar2,
  bar1,
  bar3
};

struct arg_struct {
  struct commonio_ops *ops;
  int *q1;
  int *q2;
};

struct arg_struct args = {
  &group_ops,
  &V1,
  &V2
};
*/
/* This function call to foo() is critical for this testcase, because we need a calling context in order
to reproduce this bug. If everything is in the main() function, then the passed in calling context to bc2bdd
would be NULL, then it becomes context insensitve, then the bug can not be captured. */
struct small_commonio_ops group_ops2 = {bar1};

void foo(int argc, char *argv[]) {
  group_ops2.fp_1(argc);
}

int main (int argc, char *argv[]) {
  foo(argc, argv);
  //group_ops2.fp_1(argc);
  fprintf(stderr, "V1 %d, V2 %d, V3 %d\n", V1, V2, V3);
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



