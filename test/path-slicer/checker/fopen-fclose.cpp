#include <stdio.h>

int main(int argc, char *argv[]) {
  FILE *f = NULL;
  f = fopen("foo.txt", "w");
  if (argv[1][0] != 2)
    fclose(f);
  return 0;
}

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc

// RUN: %kleebindir/klee --use-one-checker=OpenClose --use-path-slicer=1 --max-time 10 --libc=uclibc --posix-runtime --init-env \
// RUN: %s.bc --sym-args 1 10 10 --sym-files 2 2000 --max-fail 1 2> %s.output

// RUN: cat %s.output | FileCheck %s


// Expected results:

// CHECK: IntraSlicer::calStat TAKEN: IDX: 1: TID: 0: INSTRID: 1: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 2: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 3: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 7: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: entry:   store i8** %newArgv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 9: TAKEN: CHECKER_IMPORTANT: INSTR: F: __user_main: BB: entry:   %1 = call %struct.FILE* bitcast (%4* (i8*, i8*)* @fopen to %struct.FILE* (i8*, i8*)*)(i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), i8* noalias getelementptr inbounds ([2 x i8]* @.str1, i64 0, i64 0)) ; <%struct.FILE*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 11: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %2 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 12: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %3 = getelementptr inbounds i8** %2, i64 1      ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 13: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %4 = load i8** %3, align 1                      ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 14: TID: 0: INSTRID: 14: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %5 = getelementptr inbounds i8* %4, i64 0       ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 15: TID: 0: INSTRID: 15: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %6 = load i8* %5, align 1                       ; <i8> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 16: TID: 0: INSTRID: 16: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %7 = icmp ne i8 %6, 2                           ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 17: TID: 0: INSTRID: 17: TAKEN: INTRA_BR_EVENT_BETWEEN: INSTR: F: __user_main: BB: entry:   br i1 %7, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 21: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: bb1:   store i32 0, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 22: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb1:   %10 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 23: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: bb1:   store i32 %10, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 24: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb1:   %retval2 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 25: TAKEN: CHECKER_ERROR: INSTR: F: __user_main: BB: bb1:   ret i32 %retval2



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 23;  numTakenInstrs: 17;  numExedBrs: 1;  numTakenBrs: 1;  numExedSymBrs: 1;  numTakenSymBrs: 1; StaticExed/Static Instrs: 23/26;  numTakenExtCalls/numExedExtCalls: 1/1;

// CHECK: KLEE: ERROR: checker reported error

