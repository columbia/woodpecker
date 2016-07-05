#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

long V0 = 0;
long V1 = 0;
long V2 = 0;

struct small_ptr_struct {
  long *p0;
};

void foo(int argc, char *argv[]) {
  int inputSize = atoi(argv[1]);
  fprintf(stderr, "inputSize %d\n", inputSize);
  struct small_ptr_struct ptrs[4];
  struct small_ptr_struct ptr[3];
  ptr[0].p0 = &V0;
  ptr[1].p0 = &V1;
  ptr[2].p0 = &V2;
  memcpy(ptrs, ptr, inputSize);
   /* All the three statements are taken because memcpy() do array copy conservatively (for all array indexes). */
  *(ptrs[0].p0) = 7777;
  *(ptrs[1].p0) = 8888;
  *(ptr[1].p0) += 9999;
}

int main (int argc, char *argv[]) {
  foo(argc, argv);
  fprintf(stderr, "V0 %ld, V1 %ld, V2 %ld\n", V0, V1, V2);
  return (int)(V2);
}

/*  Testing purpose: this tesecase shows the memcpy() handles array copying correctly,
	since all elements of an array is collapsed into one bdd location, so we only need to copy the first element
	from src array to dest array, so the two bdd of arrays will be collapsed together, so eventually
	it models an assignment "dest = src" for array memcpy().
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 16 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:


// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 50: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 1: TID: 0: INSTRID: 51: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 52: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 53: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 5: TID: 0: INSTRID: 55: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 6: TID: 0: INSTRID: 56: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 57: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %1 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 58: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %2 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 59: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z3fooiPPc(i32 %1, i8** %2)

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 1: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 2: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %inputSize = alloca i32                         ; <i32*> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 3: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %ptrs = alloca [4 x %struct.small_ptr_struct]   ; <[4 x %struct.small_ptr_struct]*> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 14: TID: 0: INSTRID: 4: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %ptr = alloca [3 x %struct.small_ptr_struct]    ; <[3 x %struct.small_ptr_struct]*> [#uses=6]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 17: TID: 0: INSTRID: 7: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooiPPc: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 8: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %0 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 9: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %1 = getelementptr inbounds i8** %0, i64 1      ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 10: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %2 = load i8** %1, align 1                      ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 11: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %3 = call i32 @atoi(i8* %2) nounwind readonly   ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 12: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooiPPc: BB: entry:   store i32 %3, i32* %inputSize, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 26: TID: 0: INSTRID: 16: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %7 = getelementptr inbounds [3 x %struct.small_ptr_struct]* %ptr, i64 0, i64 0 ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 27: TID: 0: INSTRID: 17: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %8 = getelementptr inbounds %struct.small_ptr_struct* %7, i32 0, i32 0 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 28: TID: 0: INSTRID: 18: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64* @V0, i64** %8, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 29: TID: 0: INSTRID: 19: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %9 = getelementptr inbounds [3 x %struct.small_ptr_struct]* %ptr, i64 0, i64 1 ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 30: TID: 0: INSTRID: 20: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %10 = getelementptr inbounds %struct.small_ptr_struct* %9, i32 0, i32 0 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 31: TID: 0: INSTRID: 21: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64* @V1, i64** %10, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 32: TID: 0: INSTRID: 22: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %11 = getelementptr inbounds [3 x %struct.small_ptr_struct]* %ptr, i64 0, i64 2 ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 33: TID: 0: INSTRID: 23: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %12 = getelementptr inbounds %struct.small_ptr_struct* %11, i32 0, i32 0 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 34: TID: 0: INSTRID: 24: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64* @V2, i64** %12, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 35: TID: 0: INSTRID: 25: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %13 = load i32* %inputSize, align 4             ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 36: TID: 0: INSTRID: 26: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %14 = sext i32 %13 to i64                       ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 37: TID: 0: INSTRID: 27: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %15 = getelementptr inbounds [4 x %struct.small_ptr_struct]* %ptrs, i64 0, i64 0 ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 38: TID: 0: INSTRID: 28: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %16 = getelementptr inbounds [3 x %struct.small_ptr_struct]* %ptr, i64 0, i64 0 ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 39: TID: 0: INSTRID: 29: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %17 = bitcast %struct.small_ptr_struct* %15 to i8* ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 40: TID: 0: INSTRID: 30: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %18 = bitcast %struct.small_ptr_struct* %16 to i8* ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 41: TID: 0: INSTRID: 31: TAKEN: INTRA_EXT_CALL_MOD_LIVE: INSTR: F: _Z3fooiPPc: BB: entry:   %19 = call i8* @memcpy(i8* %17, i8* %18, i64 %14) ; <i8*> [#uses=0]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 42: TID: 0: INSTRID: 32: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %20 = getelementptr inbounds [4 x %struct.small_ptr_struct]* %ptrs, i64 0, i64 0 ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 43: TID: 0: INSTRID: 33: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %21 = getelementptr inbounds %struct.small_ptr_struct* %20, i32 0, i32 0 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 44: TID: 0: INSTRID: 34: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %22 = load i64** %21, align 8                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 45: TID: 0: INSTRID: 35: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64 7777, i64* %22, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 46: TID: 0: INSTRID: 36: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %23 = getelementptr inbounds [4 x %struct.small_ptr_struct]* %ptrs, i64 0, i64 1 ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 47: TID: 0: INSTRID: 37: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %24 = getelementptr inbounds %struct.small_ptr_struct* %23, i32 0, i32 0 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 48: TID: 0: INSTRID: 38: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %25 = load i64** %24, align 8                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 49: TID: 0: INSTRID: 39: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64 8888, i64* %25, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 50: TID: 0: INSTRID: 40: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %26 = getelementptr inbounds [3 x %struct.small_ptr_struct]* %ptr, i64 0, i64 1 ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 51: TID: 0: INSTRID: 41: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %27 = getelementptr inbounds %struct.small_ptr_struct* %26, i32 0, i32 0 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 52: TID: 0: INSTRID: 42: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %28 = load i64** %27, align 8                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 53: TID: 0: INSTRID: 43: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %29 = getelementptr inbounds [3 x %struct.small_ptr_struct]* %ptr, i64 0, i64 1 ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 54: TID: 0: INSTRID: 44: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %30 = getelementptr inbounds %struct.small_ptr_struct* %29, i32 0, i32 0 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 55: TID: 0: INSTRID: 45: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %31 = load i64** %30, align 8                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 56: TID: 0: INSTRID: 46: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %32 = load i64* %31, align 8                    ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 57: TID: 0: INSTRID: 47: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %33 = add nsw i64 %32, 9999                     ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 58: TID: 0: INSTRID: 48: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64 %33, i64* %28, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 59: TID: 0: INSTRID: 49: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z3fooiPPc: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 65: TID: 0: INSTRID: 65: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %8 = load i64* @V2, align 8                     ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 66: TID: 0: INSTRID: 66: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %9 = trunc i64 %8 to i32                        ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 67: TID: 0: INSTRID: 67: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %9, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 68: TID: 0: INSTRID: 68: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %10 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 69: TID: 0: INSTRID: 69: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %10, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 70: TID: 0: INSTRID: 70: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %retval1 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 71: TID: 0: INSTRID: 71: TAKEN: TEST_TARGET: INSTR: F: main: BB: entry:   ret i32 %retval1



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 72;  numTakenInstrs: 60;  numExedBrs: 0;  numTakenBrs: 0;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 72/72;  numTakenExtCalls/numExedExtCalls: 2/4;
