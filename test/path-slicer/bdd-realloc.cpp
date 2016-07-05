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
  long *p1;
};

void foo(int argc, char *argv[]) {
  int inputSize = atoi(argv[1]);
  fprintf(stderr, "inputSize %d\n", inputSize);
  struct small_ptr_struct *ptrs = (struct small_ptr_struct *)malloc(sizeof(struct small_ptr_struct));
  struct ptr_struct ptr = {&V0, &V1, &V2};
  memcpy(ptrs, &ptr, inputSize);
  struct small_ptr_struct *temp = (struct small_ptr_struct *)realloc(ptrs, sizeof(struct small_ptr_struct));
  /* 
    Since this is field sensitive, so 7777 is not taken, and 2222 is taken!
    Field sensitivity is preserved in this case.
  */
  *(temp->p0) = 7777;
  *(temp->p1) = 2222;
}

int main (int argc, char *argv[]) {
  foo(argc, argv);
  fprintf(stderr, "V0 %ld, V1 %ld, V2 %ld\n", V0, V1, V2);
  return (int)(V1);
}

/*  Testing purpose: the two assignments in foo() function should not be taken, since &V2 is not copied.
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 16 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:

// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 49: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 1: TID: 0: INSTRID: 50: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 51: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 52: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 5: TID: 0: INSTRID: 54: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 6: TID: 0: INSTRID: 55: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 56: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %1 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 57: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %2 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 58: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z3fooiPPc(i32 %1, i8** %2)

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 3: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %ptrs = alloca %struct.small_ptr_struct*        ; <%struct.small_ptr_struct**> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 15: TID: 0: INSTRID: 5: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %temp = alloca %struct.small_ptr_struct*        ; <%struct.small_ptr_struct**> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 27: TID: 0: INSTRID: 17: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %7 = call noalias i8* @malloc(i64 16) nounwind  ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 28: TID: 0: INSTRID: 18: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %8 = bitcast i8* %7 to %struct.small_ptr_struct* ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 29: TID: 0: INSTRID: 19: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooiPPc: BB: entry:   store %struct.small_ptr_struct* %8, %struct.small_ptr_struct** %ptrs, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 45: TID: 0: INSTRID: 35: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %20 = load %struct.small_ptr_struct** %ptrs, align 8 ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 46: TID: 0: INSTRID: 36: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %21 = bitcast %struct.small_ptr_struct* %20 to i8* ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 47: TID: 0: INSTRID: 37: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %22 = call i8* @realloc(i8* %21, i64 16) nounwind ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 48: TID: 0: INSTRID: 38: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %23 = bitcast i8* %22 to %struct.small_ptr_struct* ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 49: TID: 0: INSTRID: 39: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooiPPc: BB: entry:   store %struct.small_ptr_struct* %23, %struct.small_ptr_struct** %temp, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 54: TID: 0: INSTRID: 44: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %27 = load %struct.small_ptr_struct** %temp, align 8 ; <%struct.small_ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 55: TID: 0: INSTRID: 45: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooiPPc: BB: entry:   %28 = getelementptr inbounds %struct.small_ptr_struct* %27, i32 0, i32 1 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 56: TID: 0: INSTRID: 46: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooiPPc: BB: entry:   %29 = load i64** %28, align 8                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 57: TID: 0: INSTRID: 47: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z3fooiPPc: BB: entry:   store i64 2222, i64* %29, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 58: TID: 0: INSTRID: 48: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z3fooiPPc: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 64: TID: 0: INSTRID: 64: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %8 = load i64* @V1, align 8                     ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 65: TID: 0: INSTRID: 65: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %9 = trunc i64 %8 to i32                        ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 66: TID: 0: INSTRID: 66: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %9, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 67: TID: 0: INSTRID: 67: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %10 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 68: TID: 0: INSTRID: 68: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %10, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 69: TID: 0: INSTRID: 69: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %retval1 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 70: TID: 0: INSTRID: 70: TAKEN: TEST_TARGET: INSTR: F: main: BB: entry:   ret i32 %retval1



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 71;  numTakenInstrs: 31;  numExedBrs: 0;  numTakenBrs: 0;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 71/71;  numTakenExtCalls/numExedExtCalls: 2/6;



