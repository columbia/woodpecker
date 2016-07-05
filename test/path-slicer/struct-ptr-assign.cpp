#include <stdio.h>
#include <memory.h>

long V0 = 0;
long V1 = 0;
long V2 = 0;

struct ptr_struct {
  long *p0;
  long *p1;
  long *p2;
};

/* This function call to foo() is critical for this testcase, because we need a calling context in order
to reproduce this bug. If everything is in the main() function, then the passed in calling context to bc2bdd
would be NULL, then it becomes context insensitve, then the bug can not be captured. */
void foo(struct ptr_struct *ptrs, int argc, char *argv[]) {
  //*(ptrs->p1) = 111;
}

int main (int argc, char *argv[]) {
  struct ptr_struct ptrs = {&V0, &V1, &V2};
  if (argc > 3)
    *(ptrs.p0) = 91;
  if (argc < 4)
    *(ptrs.p1) = 101;
  if (argc != 5)
    *(ptrs.p2) = 131;
  fprintf(stderr, "V0 %ld, V1 %ld, V2 %ld\n", V0, V1, V2);
  return (int)(V1);
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

// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 8: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=4]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 10: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 11: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 4: TID: 0: INSTRID: 12: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %ptrs = alloca %struct.ptr_struct               ; <%struct.ptr_struct*> [#uses=6]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 6: TID: 0: INSTRID: 14: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 19: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %3 = getelementptr inbounds %struct.ptr_struct* %ptrs, i32 0, i32 1 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 20: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %4 = load i64** getelementptr inbounds (%struct.ptr_struct* @_ZZ4mainE3C.0, i64 0, i32 1), align 8 ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 21: TAKEN: INTRA_STORE_ALIAS: INSTR: F: main: BB: entry:   store i64* %4, i64** %3, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 32: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb1:   %11 = load i32* %argc_addr, align 4             ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 33: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: bb1:   %12 = icmp sle i32 %11, 3                       ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 34: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: main: BB: bb1:   br i1 %12, label %bb2, label %bb3

// CHECK: IntraSlicer::calStat TAKEN: IDX: 23: TID: 0: INSTRID: 35: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: bb2:   %13 = getelementptr inbounds %struct.ptr_struct* %ptrs, i32 0, i32 1 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 24: TID: 0: INSTRID: 36: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb2:   %14 = load i64** %13, align 8                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 25: TID: 0: INSTRID: 37: TAKEN: INTRA_STORE_ALIAS: INSTR: F: main: BB: bb2:   store i64 101, i64* %14, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 39: TID: 0: INSTRID: 51: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb5:   %24 = load i64* @V1, align 8                    ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 40: TID: 0: INSTRID: 52: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: bb5:   %25 = trunc i64 %24 to i32                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 41: TID: 0: INSTRID: 53: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: bb5:   store i32 %25, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 42: TID: 0: INSTRID: 54: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb5:   %26 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 43: TID: 0: INSTRID: 55: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: bb5:   store i32 %26, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 44: TID: 0: INSTRID: 56: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: bb5:   %retval6 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 45: TID: 0: INSTRID: 57: TAKEN: TEST_TARGET: INSTR: F: main: BB: bb5:   ret i32 %retval6



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 46;  numTakenInstrs: 21;  numExedBrs: 5;  numTakenBrs: 1;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 46/58;  numTakenExtCalls/numExedExtCalls: 0/1;




