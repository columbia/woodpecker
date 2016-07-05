#include <stdio.h>

int globalVar = 0;
int Var2 = 0;
int *p;
int *q;

int foo() {
  if (globalVar)
    Var2++;
  return 0;
}

/* Slicing target: the last return instruction, including the returned value.
    Testcase purpose: store instructions with poniters. The if branch should be 
    taken.
*/
int main (int argc, char *argv[]) {
  p = &globalVar;
  q = p;
  foo();
  if (argc == 0)
    (*q) = 5;
  foo();
  return globalVar;
}

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:
// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 15: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 17: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 18: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 5: TID: 0: INSTRID: 20: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 26: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %3 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 23: TID: 0: INSTRID: 27: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %4 = icmp eq i32 %3, 0                          ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 24: TID: 0: INSTRID: 28: TAKEN: INTRA_BR_WR_BETWEEN: INSTR: F: main: BB: entry:   br i1 %4, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 37: TID: 0: INSTRID: 33: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb1:   %7 = load i32* @globalVar, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 38: TID: 0: INSTRID: 34: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: bb1:   store i32 %7, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 39: TID: 0: INSTRID: 35: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb1:   %8 = load i32* %0, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 40: TID: 0: INSTRID: 36: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: bb1:   store i32 %8, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 41: TID: 0: INSTRID: 37: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb1:   %retval2 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 42: TID: 0: INSTRID: 38: TAKEN: TEST_TARGET: INSTR: F: main: BB: bb1:   ret i32 %retval2



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 43;  numTakenInstrs: 13;  numExedBrs: 3;  numTakenBrs: 1;  numExedSymBrs: 0;  numTakenSymBrs: 0

