#include <errno.h>
#include <stdio.h>

void foo() {
  errno = 1;
}

/* NOTE: this is a testcase our current bc2bdd implementation missed. A clean way is to handle errno as global
    within bdd alias analysis. Since most of programs only read from errno, they don't write to errno,
    this issue could be defered, I think. */
int main (int argc, char *argv[]) {
  if (argc > 4) {  // Bug in bc2bdd on handling errno caused this branch to be not taken.
    foo();
  }
  errno++;  // This one is already handled correctly by considering errno in mustAlias() in intra-thread phase.
  return errno;
}

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 2> %s.output
// RUN: cat %s.output | FileCheck %s


// Expected results:
// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 5: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 6: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 10: TID: 0: INSTRID: 15: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: main: BB: bb1:   %3 = call i32* @__errno_location() nounwind readnone ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 16: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb1:   %4 = load i32* %3, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 17: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: bb1:   %5 = add nsw i32 %4, 1                          ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 18: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: bb1:   store i32 %5, i32* %3, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 14: TID: 0: INSTRID: 19: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: main: BB: bb1:   %6 = call i32* @__errno_location() nounwind readnone ; <i32*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 15: TID: 0: INSTRID: 20: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb1:   %7 = load i32* %6, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 16: TID: 0: INSTRID: 21: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: bb1:   store i32 %7, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 17: TID: 0: INSTRID: 22: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb1:   %8 = load i32* %0, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 23: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: bb1:   store i32 %8, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 24: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb1:   %retval2 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 25: TAKEN: TEST_TARGET: INSTR: F: main: BB: bb1:   ret i32 %retval2



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 21;  numTakenInstrs: 13;  numExedBrs: 1;  numTakenBrs: 0;  numExedSymBrs: 0;  numTakenSymBrs: 0

    
