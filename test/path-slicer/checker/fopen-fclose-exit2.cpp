#include <stdio.h>
#include <stdlib.h>

int foo(char input) {
  if (input == 'a')
    return 0;
  else
    return 1;
}

int main(int argc, char *argv[]) {
  if (argc < 2)
    return -1;
  if (argv[1][1] != 'x') {
    if (foo((char)argv[1][0])) /* the return value of foo() affects exit() or not, but it does not affect hitting the events, so it should not be taken. */
      exit(2);
  } else {
    FILE *fp = fopen("foo.txt", "w");
    if (!fp)
      return -1;
    fprintf(stderr, "do something!!!!!!!!!!!!!!!!!!!!\n");
    fclose(fp);
  }
  return 0;
}

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc

// RUN: %kleebindir/klee --use-one-checker=OpenClose --use-path-slicer=1 --max-time 20 --libc=uclibc --posix-runtime --init-env \
// RUN: %s.bc --sym-args 1 2 2 --sym-files 2 8 --max-fail 1 2> %s.output

// RUN: cat %s.output | FileCheck %s


// Expected results:

// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 16: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 1: TID: 0: INSTRID: 17: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 23: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: entry:   store i32 %newArgc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 24: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: entry:   store i8** %newArgv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 25: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %1 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 10: TID: 0: INSTRID: 26: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %2 = icmp sle i32 %1, 1                         ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 27: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: __user_main: BB: entry:   br i1 %2, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 30: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb1:   %3 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 31: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: bb1:   %4 = getelementptr inbounds i8** %3, i64 1      ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 14: TID: 0: INSTRID: 32: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb1:   %5 = load i8** %4, align 1                      ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 15: TID: 0: INSTRID: 33: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: bb1:   %6 = getelementptr inbounds i8* %5, i64 1       ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 16: TID: 0: INSTRID: 34: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb1:   %7 = load i8* %6, align 1                       ; <i8> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 17: TID: 0: INSTRID: 35: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: bb1:   %8 = icmp ne i8 %7, 120                         ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 36: TAKEN: INTRA_BR_EVENT_BETWEEN_CALLER_N_POST: INSTR: F: __user_main: BB: bb1:   br i1 %8, label %bb2, label %bb5

// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 48;  numTakenInstrs: 14;  numExedBrs: 5;  numTakenBrs: 2;  numExedSymBrs: 3;  numTakenSymBrs: 2;

