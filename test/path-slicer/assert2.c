#include <stdio.h>
#include <assert.h>
int x;

int main(int argc, char *argv[]) {
  x = argc;
  if (x == 7) {
    fprintf(stderr, "Has an assert failure.\n");
    assert(x != 8);
    return 1;
  }
  fprintf(stderr, "Good.\n");
  return 0;
}

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 2> %s.output
// RUN: cat %s.output | FileCheck %s


// Expected results:
// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 0: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 2: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 3: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 5: TID: 0: INSTRID: 5: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 7: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %1 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 8: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %1, i32* @x, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 9: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %2 = load i32* @x, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 10: TID: 0: INSTRID: 10: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %3 = icmp eq i32 %2, 7                          ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 13: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: main: BB: entry:   br i1 %3, label %bb, label %bb3

// CHECK: IntraSlicer::calStat TAKEN: IDX: 15: TID: 0: INSTRID: 23: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: bb3:   store i32 0, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 17: TID: 0: INSTRID: 25: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb4:   %10 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 26: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: bb4:   store i32 %10, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 27: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb4:   %retval5 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 28: TAKEN: TEST_TARGET: INSTR: F: main: BB: bb4:   ret i32 %retval5



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 21;  numTakenInstrs: 14;  numExedBrs: 2;  numTakenBrs: 1;  numExedSymBrs: 0;  numTakenSymBrs: 0


