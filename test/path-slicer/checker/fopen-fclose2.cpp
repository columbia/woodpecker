#include <stdio.h>

int main(int argc, char *argv[]) {
  FILE *f = NULL;
  f = fopen("foo.txt", "w");

  // A self-constructed branch which is irrelevant to the fopen-fclose bug. 
  // None of these branches should be taken.
  if (argc == 3) {
    int sum = 0;
    for(int i = 0; i < argc; i++) {
      if (argv[i][0] == 'a')
        break;
      sum += (int)argv[i][0];
    }
    fprintf(stderr, "SUM %d\n", sum);
  }

  // A relevant branch to this bug. This branc should be taken.
  if (argc != 2)
    fclose(f);
  return 0;
}

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc

// RUN: %kleebindir/klee --use-one-checker=OpenClose --use-path-slicer=1 --mark-pruned-only=1 --max-time 15 --libc=uclibc --posix-runtime --init-env \
// RUN: %s.bc --sym-args 1 4 10 --sym-files 2 2000 --max-fail 1 2> %s.output

// RUN: cat %s.output | FileCheck %s


// Expected results:

// CHECK: KLEE: ERROR: checker reported error

// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 0: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=4]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 4: TID: 0: INSTRID: 4: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %f = alloca %struct.FILE*                       ; <%struct.FILE**> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 8: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: entry:   store i32 %newArgc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 11: TAKEN: CHECKER_IMPORTANT: INSTR: F: __user_main: BB: entry:   %1 = call %struct.FILE* bitcast (%4* (i8*, i8*)* @fopen to %struct.FILE* (i8*, i8*)*)(i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), i8* noalias getelementptr inbounds ([2 x i8]* @.str1, i64 0, i64 0)) ; <%struct.FILE*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 12: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: entry:   store %struct.FILE* %1, %struct.FILE** %f, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 16: TID: 0: INSTRID: 51: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb5:   %30 = load i32* %argc_addr, align 4             ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 17: TID: 0: INSTRID: 52: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: bb5:   %31 = icmp ne i32 %30, 2                        ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 53: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: __user_main: BB: bb5:   br i1 %31, label %bb6, label %bb7

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 54: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb6:   %32 = load %struct.FILE** %f, align 8           ; <%struct.FILE*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 55: TAKEN: CHECKER_IMPORTANT: INSTR: F: __user_main: BB: bb6:   %33 = call i32 bitcast (i32 (%4*)* @fclose to i32 (%struct.FILE*)*)(%struct.FILE* %32) ; <i32> [#uses=0]



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 27;  numTakenInstrs: 10;  numExedBrs: 3;  numTakenBrs: 1;  numExedSymBrs: 2;  numTakenSymBrs: 1; StaticExed/Static Instrs: 62/62;  numTakenExtCalls/numExedExtCalls: 2/2;


