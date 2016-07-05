#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

long V0 = 0;
long V1 = 0;
long V2 = 0;

struct ptr_struct {
  long *p0;
  long *p1;
  long *p2;
};

struct small_ptr_struct {
  long *p0;
};

void foo(int argc, char *argv[]) {
	int inputSize = atoi(argv[1]);
	long **pp = (long **)malloc(inputSize*(sizeof(long *)));
	long **qq = (long **)malloc(2*inputSize*(sizeof(long *)));
	long **mm = (long **)malloc(3*inputSize*(sizeof(long *)));

	pp[0] = &V0;
	pp[1] = &V1;
	pp[2] = &V2;

	mm[0] = &V0;
	mm[1] = &V1;
	mm[2] = &V0;

	*(pp[1]) = 1212;			// should be taken because of conservative may alias.
	qq = pp;
	*(qq[2]) = 3333;			// should be taken because of overwritten V2.

	qq = mm;
	*(mm[2]) = 1111;		// should not be taken because no alias.
}

int main (int argc, char *argv[]) {
  foo(argc, argv);
  fprintf(stderr, "V0 %ld, V1 %ld, V2 %ld\n", V0, V1, V2);
  return (int)(V2);
}

/*  Testing purpose: the two assignments in foo() function should not be taken, since &V2 is not copied.
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 16 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results: (CURRENT SLICING RESULT IS WRONG, THE "8888" STATEMENT SHOULD BE TAKEN, BUT IN FACT NOT!)


// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 67: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 1: TID: 0: INSTRID: 68: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 69: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 70: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 5: TID: 0: INSTRID: 72: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 6: TID: 0: INSTRID: 73: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 74: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %1 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 75: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %2 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 76: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z3fooiPPc(i32 %1, i8** %2) nounwind

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 1: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 2: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %inputSize = alloca i32                         ; <i32*> [#uses=4]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 3: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %pp = alloca i64**                              ; <i64***> [#uses=6]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 14: TID: 0: INSTRID: 4: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %qq = alloca i64**                              ; <i64***> [#uses=4]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 15: TID: 0: INSTRID: 5: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %mm = alloca i64**                              ; <i64***> [#uses=6]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 8: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooiPPc: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 9: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %0 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 10: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %1 = getelementptr inbounds i8** %0, i64 1      ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 11: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %2 = load i8** %1, align 1                      ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 12: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %3 = call i32 @atoi(i8* %2) nounwind readonly   ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 23: TID: 0: INSTRID: 13: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooiPPc: BB: entry:   store i32 %3, i32* %inputSize, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 24: TID: 0: INSTRID: 14: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %4 = load i32* %inputSize, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 25: TID: 0: INSTRID: 15: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %5 = sext i32 %4 to i64                         ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 26: TID: 0: INSTRID: 16: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %6 = mul i64 %5, 8                              ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 27: TID: 0: INSTRID: 17: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %7 = call noalias i8* @malloc(i64 %6) nounwind  ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 28: TID: 0: INSTRID: 18: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %8 = bitcast i8* %7 to i64**                    ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 29: TID: 0: INSTRID: 19: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooiPPc: BB: entry:   store i64** %8, i64*** %pp, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 36: TID: 0: INSTRID: 26: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %14 = load i32* %inputSize, align 4             ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 37: TID: 0: INSTRID: 27: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %15 = sext i32 %14 to i64                       ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 38: TID: 0: INSTRID: 28: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %16 = mul i64 %15, 24                           ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 39: TID: 0: INSTRID: 29: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %17 = call noalias i8* @malloc(i64 %16) nounwind ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 40: TID: 0: INSTRID: 30: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %18 = bitcast i8* %17 to i64**                  ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 41: TID: 0: INSTRID: 31: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooiPPc: BB: entry:   store i64** %18, i64*** %mm, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 42: TID: 0: INSTRID: 32: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %19 = load i64*** %pp, align 8                  ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 43: TID: 0: INSTRID: 33: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %20 = getelementptr inbounds i64** %19, i64 0   ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 44: TID: 0: INSTRID: 34: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64* @V0, i64** %20, align 1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 45: TID: 0: INSTRID: 35: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %21 = load i64*** %pp, align 8                  ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 46: TID: 0: INSTRID: 36: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %22 = getelementptr inbounds i64** %21, i64 1   ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 47: TID: 0: INSTRID: 37: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64* @V1, i64** %22, align 1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 48: TID: 0: INSTRID: 38: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %23 = load i64*** %pp, align 8                  ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 49: TID: 0: INSTRID: 39: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %24 = getelementptr inbounds i64** %23, i64 2   ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 50: TID: 0: INSTRID: 40: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64* @V2, i64** %24, align 1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 51: TID: 0: INSTRID: 41: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %25 = load i64*** %mm, align 8                  ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 52: TID: 0: INSTRID: 42: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %26 = getelementptr inbounds i64** %25, i64 0   ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 53: TID: 0: INSTRID: 43: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64* @V0, i64** %26, align 1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 54: TID: 0: INSTRID: 44: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %27 = load i64*** %mm, align 8                  ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 55: TID: 0: INSTRID: 45: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %28 = getelementptr inbounds i64** %27, i64 1   ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 56: TID: 0: INSTRID: 46: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64* @V1, i64** %28, align 1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 57: TID: 0: INSTRID: 47: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %29 = load i64*** %mm, align 8                  ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 58: TID: 0: INSTRID: 48: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %30 = getelementptr inbounds i64** %29, i64 2   ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 59: TID: 0: INSTRID: 49: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64* @V0, i64** %30, align 1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 60: TID: 0: INSTRID: 50: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %31 = load i64*** %pp, align 8                  ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 61: TID: 0: INSTRID: 51: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %32 = getelementptr inbounds i64** %31, i64 1   ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 62: TID: 0: INSTRID: 52: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %33 = load i64** %32, align 1                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 63: TID: 0: INSTRID: 53: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64 1212, i64* %33, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 64: TID: 0: INSTRID: 54: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %34 = load i64*** %pp, align 8                  ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 65: TID: 0: INSTRID: 55: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooiPPc: BB: entry:   store i64** %34, i64*** %qq, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 66: TID: 0: INSTRID: 56: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %35 = load i64*** %qq, align 8                  ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 67: TID: 0: INSTRID: 57: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %36 = getelementptr inbounds i64** %35, i64 2   ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 68: TID: 0: INSTRID: 58: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %37 = load i64** %36, align 1                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 69: TID: 0: INSTRID: 59: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64 3333, i64* %37, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 76: TID: 0: INSTRID: 66: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z3fooiPPc: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 82: TID: 0: INSTRID: 82: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %8 = load i64* @V2, align 8                     ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 83: TID: 0: INSTRID: 83: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %9 = trunc i64 %8 to i32                        ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 84: TID: 0: INSTRID: 84: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %9, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 85: TID: 0: INSTRID: 85: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %10 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 86: TID: 0: INSTRID: 86: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %10, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 87: TID: 0: INSTRID: 87: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %retval1 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 88: TID: 0: INSTRID: 88: TAKEN: TEST_TARGET: INSTR: F: main: BB: entry:   ret i32 %retval1



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 89;  numTakenInstrs: 68;  numExedBrs: 0;  numTakenBrs: 0;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 89/89;  numTakenExtCalls/numExedExtCalls: 3/5;

