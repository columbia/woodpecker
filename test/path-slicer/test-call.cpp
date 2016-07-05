#include <stdio.h>

int globalVar = 0;

void foo(int argc) {
  if (argc > 4)
    globalVar++;
}

/* Slicing target: the last return instruction, including the returned value.
    Testcase purpose 1: the not executed branch will modify shared variable; make 
    sure our algorithm will take this branch.
    Testcase purpose 2: make sure whether the return instruction and the call 
    instruction to foo() will be taken.
*/
int main (int argc, char *argv[]) {
  foo(argc);
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
// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 11: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 13: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 14: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 5: TID: 0: INSTRID: 16: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 18: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %1 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 19: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z3fooi(i32 %1) nounwind

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 0: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooi: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 2: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooi: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 3: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooi: BB: entry:   %0 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 4: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooi: BB: entry:   %1 = icmp sgt i32 %0, 4                         ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 14: TID: 0: INSTRID: 5: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z3fooi: BB: entry:   br i1 %1, label %bb, label %return

// CHECK: IntraSlicer::calStat TAKEN: IDX: 15: TID: 0: INSTRID: 10: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z3fooi: BB: return:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 16: TID: 0: INSTRID: 20: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %2 = load i32* @globalVar, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 17: TID: 0: INSTRID: 21: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %2, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 22: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %3 = load i32* %0, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 23: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %3, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 24: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %retval1 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 25: TAKEN: TEST_TARGET: INSTR: F: main: BB: entry:   ret i32 %retval1



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 22;  numTakenInstrs: 18;  numExedBrs: 1;  numTakenBrs: 1;  numExedSymBrs: 0;  numTakenSymBrs: 0

