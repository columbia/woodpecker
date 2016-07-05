#include <stdio.h>
#include <string.h>

int globalVar = 0;
int var2 = 234;

void foo(int argc) {
  //void *dest = (void *)&globalVar;
  memcpy(&globalVar, &var2, sizeof(int));
}

int main (int argc, char *argv[]) {
  foo(argc);
  fprintf(stderr, "globalVar %d\n", globalVar);
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
// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 5: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 7: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 8: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 5: TID: 0: INSTRID: 10: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 12: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %1 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 13: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z3fooi(i32 %1) nounwind

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 3: TAKEN: INTRA_EXT_CALL_MOD_LIVE: INSTR: F: _Z3fooi: BB: entry:   %0 = call i8* @memcpy(i8* bitcast (i32* @globalVar to i8*), i8* bitcast (i32* @var2 to i8*), i64 4) ; <i8*> [#uses=0]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 4: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z3fooi: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 17: TID: 0: INSTRID: 17: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %5 = load i32* @globalVar, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 18: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %5, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 19: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %6 = load i32* %0, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 20: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %6, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 21: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %retval1 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 22: TAKEN: TEST_TARGET: INSTR: F: main: BB: entry:   ret i32 %retval1



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 23;  numTakenInstrs: 14;  numExedBrs: 0;  numTakenBrs: 0;  numExedSymBrs: 0;  numTakenSymBrs: 0

