/* fread example: read a complete file */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char *argv[]) {
  FILE * pFile;
  long lSize;
  char * buffer;
  size_t result;

  pFile = fopen ( "./myfile" , "rb" );
  if (pFile==NULL) {
    fputs ("File error",stderr);
    return 1;
  }

  // obtain file size:
  fseek (pFile , 0 , SEEK_END);
  lSize = ftell (pFile);
  rewind (pFile);

  // allocate memory to contain the whole file:
  buffer = (char*) malloc (sizeof(char)*lSize);
  if (buffer == NULL) {
    fputs ("Memory error",stderr);
    return 2;
  }

  // copy the file into the buffer:
  result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {
    char buf[64];
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "\n\nReading error result %d, file size %d\n\n", (int)result, (int)lSize);
    fputs(buf,stderr);
    //fprintf (stderr, "\n\nReading error result %d, file size %d\n\n", (int)result, (int)lSize);
    return 3;
  }

  // terminate
  fclose (pFile);
  free (buffer);
  return 0;
}

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc

// RUN: %kleebindir/klee --use-one-checker=File --use-path-slicer=1 \
// RUN: --emit-all-errors --max-time 10 --libc=uclibc --posix-runtime --init-env \
// RUN: %s.bc --sym-args 1 10 10 --sym-files 2 2000 --max-fail 1 2> %s.output

// RUN: cat %s.output | FileCheck %s


// Expected results:
// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 2: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 3: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=5]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 4: TID: 0: INSTRID: 4: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %pFile = alloca %struct.FILE*                   ; <%struct.FILE**> [#uses=7]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 5: TID: 0: INSTRID: 5: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %lSize = alloca i64                             ; <i64*> [#uses=5]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 6: TID: 0: INSTRID: 6: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %buffer = alloca i8*                            ; <i8**> [#uses=4]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 7: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %result = alloca i64                            ; <i64*> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 12: TAKEN: CHECKER_IMPORTANT: INSTR: F: __user_main: BB: entry:   %1 = call %struct.FILE* bitcast (%4* (i8*, i8*)* @fopen to %struct.FILE* (i8*, i8*)*)(i8* noalias getelementptr inbounds ([9 x i8]* @.str, i64 0, i64 0), i8* noalias getelementptr inbounds ([3 x i8]* @.str1, i64 0, i64 0)) ; <%struct.FILE*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 13: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: entry:   store %struct.FILE* %1, %struct.FILE** %pFile, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 14: TID: 0: INSTRID: 14: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %2 = load %struct.FILE** %pFile, align 8        ; <%struct.FILE*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 15: TID: 0: INSTRID: 15: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %3 = icmp eq %struct.FILE* %2, null             ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 16: TID: 0: INSTRID: 16: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: __user_main: BB: entry:   br i1 %3, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 23: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb1:   %8 = load %struct.FILE** %pFile, align 8        ; <%struct.FILE*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 24: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: __user_main: BB: bb1:   %9 = call i64 bitcast (i64 (%4*)* @ftell to i64 (%struct.FILE*)*)(%struct.FILE* %8) ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 25: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: bb1:   store i64 %9, i64* %lSize, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 24: TID: 0: INSTRID: 28: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb1:   %11 = load i64* %lSize, align 8                 ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 25: TID: 0: INSTRID: 29: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: __user_main: BB: bb1:   %12 = call noalias i8* @malloc(i64 %11) nounwind ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 26: TID: 0: INSTRID: 30: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: bb1:   store i8* %12, i8** %buffer, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 27: TID: 0: INSTRID: 31: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb1:   %13 = load i8** %buffer, align 8                ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 28: TID: 0: INSTRID: 32: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: bb1:   %14 = icmp eq i8* %13, null                     ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 29: TID: 0: INSTRID: 33: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: __user_main: BB: bb1:   br i1 %14, label %bb2, label %bb3

// CHECK: IntraSlicer::calStat TAKEN: IDX: 32: TID: 0: INSTRID: 40: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb3:   %19 = load %struct.FILE** %pFile, align 8       ; <%struct.FILE*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 33: TID: 0: INSTRID: 41: TAKEN: CHECKER_IMPORTANT: INSTR: F: __user_main: BB: bb3:   %20 = call i64 bitcast (i64 (i8*, i64, i64, %4*)* @fread_unlocked to i64 (i8*, i64, i64, %struct.FILE*)*)(i8* noalias %18, i64 1, i64 %17, %struct.FILE* noalias %19) ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 34: TID: 0: INSTRID: 42: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: bb3:   store i64 %20, i64* %result, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 35: TID: 0: INSTRID: 43: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb3:   %21 = load i64* %lSize, align 8                 ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 36: TID: 0: INSTRID: 44: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb3:   %22 = load i64* %result, align 8                ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 37: TID: 0: INSTRID: 45: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: bb3:   %23 = icmp ne i64 %21, %22                      ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 38: TID: 0: INSTRID: 46: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: __user_main: BB: bb3:   br i1 %23, label %bb4, label %bb5

// CHECK: IntraSlicer::calStat TAKEN: IDX: 50: TID: 0: INSTRID: 58: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: bb4:   store i32 3, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 52: TID: 0: INSTRID: 66: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb6:   %38 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 53: TID: 0: INSTRID: 67: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: bb6:   store i32 %38, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 54: TID: 0: INSTRID: 68: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: bb6:   %retval7 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 55: TID: 0: INSTRID: 69: TAKEN: TEST_TARGET: INSTR: F: __user_main: BB: bb6:   ret i32 %retval7



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 56;  numTakenInstrs: 32;  numExedBrs: 4;  numTakenBrs: 3;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 62/70;  numTakenExtCalls/numExedExtCalls: 4/9;


