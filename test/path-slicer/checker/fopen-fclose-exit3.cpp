#include <stdio.h>
#include <stdlib.h>

int foo(char input) {
  if (input == 'a')
    return 0;
  else
    return 1;
}

int bar(int argc, char *argv[]) {
  if (argc < 2)
    return -1;
  // 'y' is 121.
  if (argv[1][2] == 'y') {// This branch should be taken because of not-postdominate (have exit() in between).
    int ret = foo((char)argv[1][0]);
    if (ret)
      exit(1);
  }
  // 'x' is 120.
  if (argv[1][1] == 'x') { // This branch should be taken because of event-between.
    FILE *fp = fopen("foo.txt", "w");
    if (!fp)
      return -1;
    fprintf(stderr, "do something!!!!!!!!!!!!!!!!!!!!\n");
    fclose(fp);
  }
  if (argv[1][3] != 'z') // This branch should not be taken.
    exit(33);
  return 0;
}

int main(int argc, char *argv[]) {
  return bar(argc, argv);
}

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc

// RUN: %kleebindir/klee --use-one-checker=OpenClose --use-path-slicer=1 --max-time 20 --libc=uclibc --posix-runtime --init-env \
// RUN: %s.bc --sym-args 1 2 4 --sym-files 2 8 --max-fail 1 2> %s.output

// RUN: cat %s.output | FileCheck %s


// Expected results:

// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 85: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 1: TID: 0: INSTRID: 86: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 5: TID: 0: INSTRID: 90: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: entry:   store i32 %newArgc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 6: TID: 0: INSTRID: 91: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: entry:   store i8** %newArgv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 92: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %1 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 93: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %2 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 94: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %3 = call i32 @_Z3bariPPc(i32 %1, i8** %2)      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 10: TID: 0: INSTRID: 16: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3bariPPc: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 17: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3bariPPc: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=5]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 17: TID: 0: INSTRID: 23: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3bariPPc: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 24: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3bariPPc: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 25: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3bariPPc: BB: entry:   %1 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 26: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3bariPPc: BB: entry:   %2 = icmp sle i32 %1, 1                         ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 27: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z3bariPPc: BB: entry:   br i1 %2, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 30: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3bariPPc: BB: bb1:   %3 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 23: TID: 0: INSTRID: 31: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3bariPPc: BB: bb1:   %4 = getelementptr inbounds i8** %3, i64 1      ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 24: TID: 0: INSTRID: 32: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3bariPPc: BB: bb1:   %5 = load i8** %4, align 1                      ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 25: TID: 0: INSTRID: 33: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3bariPPc: BB: bb1:   %6 = getelementptr inbounds i8* %5, i64 2       ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 26: TID: 0: INSTRID: 34: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3bariPPc: BB: bb1:   %7 = load i8* %6, align 1                       ; <i8> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 27: TID: 0: INSTRID: 35: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3bariPPc: BB: bb1:   %8 = icmp eq i8 %7, 121                         ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 28: TID: 0: INSTRID: 36: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z3bariPPc: BB: bb1:   br i1 %8, label %bb2, label %bb4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 29: TID: 0: INSTRID: 51: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3bariPPc: BB: bb4:   %19 = load i8*** %argv_addr, align 8            ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 30: TID: 0: INSTRID: 52: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3bariPPc: BB: bb4:   %20 = getelementptr inbounds i8** %19, i64 1    ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 31: TID: 0: INSTRID: 53: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3bariPPc: BB: bb4:   %21 = load i8** %20, align 1                    ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 32: TID: 0: INSTRID: 54: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3bariPPc: BB: bb4:   %22 = getelementptr inbounds i8* %21, i64 1     ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 33: TID: 0: INSTRID: 55: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3bariPPc: BB: bb4:   %23 = load i8* %22, align 1                     ; <i8> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 34: TID: 0: INSTRID: 56: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3bariPPc: BB: bb4:   %24 = icmp eq i8 %23, 120                       ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 35: TID: 0: INSTRID: 57: TAKEN: INTRA_BR_EVENT_BETWEEN_CALLER_N_POST: INSTR: F: _Z3bariPPc: BB: bb4:   br i1 %24, label %bb5, label %bb8



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 44;  numTakenInstrs: 28;  numExedBrs: 4;  numTakenBrs: 3;  numExedSymBrs: 4;  numTakenSymBrs: 3; 
