#include <stdio.h>

int globalV = 0;
int *q = &globalV;  // This assignment is totally missed by bc2bdd.

int main (int argc, char *argv[]) {
  if (argc < 4)
    *(q) += 123;
  fprintf(stderr, "globalVar is %d\n", globalV);
  return globalV;
}
 
/*  Testing purpose: This testcase has reproduced the bc2bdd problem: can not capture pointer assignment in global scope.
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:




// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 0: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 2: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 3: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 5: TID: 0: INSTRID: 5: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 7: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %1 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 8: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %2 = icmp sle i32 %1, 3                         ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 9: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: main: BB: entry:   br i1 %2, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 10: TID: 0: INSTRID: 10: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb:   %3 = load i32** @q, align 8                     ; <i32*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 11: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb:   %4 = load i32** @q, align 8                     ; <i32*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 12: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb:   %5 = load i32* %4, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 13: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: bb:   %6 = add nsw i32 %5, 123                        ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 14: TID: 0: INSTRID: 14: TAKEN: INTRA_STORE_ALIAS: INSTR: F: main: BB: bb:   store i32 %6, i32* %3, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 19: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb1:   %10 = load i32* @globalV, align 4               ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 20: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: bb1:   store i32 %10, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 21: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb1:   %11 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 22: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: bb1:   store i32 %11, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 23: TID: 0: INSTRID: 23: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb1:   %retval2 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 24: TID: 0: INSTRID: 24: TAKEN: TEST_TARGET: INSTR: F: main: BB: bb1:   ret i32 %retval2


// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 25;  numTakenInstrs: 18;  numExedBrs: 2;  numTakenBrs: 1;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 25/25;  numTakenExtCalls/numExedExtCalls: 0/1;


