#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

long V0 = 0;
long V1 = 0;
long V2 = 0;

struct ptr_struct {
  long *p0;
  long *p1;
  long *p2;
};

struct small_ptr_struct {
  long *p0;
  long *p1;
};

void foo(int argc, char *argv[]) {
  int inputSize = atoi(argv[1]);
  fprintf(stderr, "inputSize %d\n", inputSize);
  struct small_ptr_struct *ptrs = (struct small_ptr_struct *)malloc(sizeof(struct small_ptr_struct));
  struct ptr_struct ptr = {&V0, &V1, &V2};
  memcpy(ptrs, &ptr, inputSize);
  *(ptrs->p0) = 7777;
  *(ptrs->p1) = 2222;
}

int main (int argc, char *argv[]) {
  foo(argc, argv);
  fprintf(stderr, "V0 %ld, V1 %ld, V2 %ld\n", V0, V1, V2);
  return (int)(V2);
}

/*  Testing purpose: the two assignments in foo() function should not be taken, since &V2 is not copied.
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 16 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 45: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 46: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 58: TID: 0: INSTRID: 58: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %8 = load i64* @V2, align 8                     ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 59: TID: 0: INSTRID: 59: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %9 = trunc i64 %8 to i32                        ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 60: TID: 0: INSTRID: 60: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %9, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 61: TID: 0: INSTRID: 61: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %10 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 62: TID: 0: INSTRID: 62: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %10, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 63: TID: 0: INSTRID: 63: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %retval1 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 64: TID: 0: INSTRID: 64: TAKEN: TEST_TARGET: INSTR: F: main: BB: entry:   ret i32 %retval1



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 65;  numTakenInstrs: 9;  numExedBrs: 0;  numTakenBrs: 0;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 65/65;  numTakenExtCalls/numExedExtCalls: 0/5;




