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
  fprintf(stderr, "inputSize %d\n", inputSize);
  struct ptr_struct *ptrs = (struct ptr_struct *)malloc(sizeof(struct ptr_struct));
  struct small_ptr_struct ptr[3] = {{&V0}, {&V1}, {&V2}};
  memcpy(ptrs, &ptr, inputSize);
  *(ptrs->p0) = 7777;
  *(ptrs->p1) = 8888;
	/* This shows the current problem, if the size of memcpy is dynamic (not a constant int),
	and if the two src and dest pointers are dynamic arrays (although currently ptr[3] is constant array),
	bc2bdd will only copy the first element of the array "ptr", so the store to &V1 is missed.

Below is the debugging output from bc2bdd, and it shows only the first element (a pointer with size 8
bytes) is modeled by bc2bdd.

-----------------
iterate_add_copy szSrc 8, szDest 24, numbytes -1
srcGep: 74 <i64**> = GEP  src <[3 x { i64* }]*> 0, 0
75 <i64*> = load 74 <i64**>
destGep: 76 <i64**> = GEP  dest <{ i64*, i64*, i64* }*> 0, 0
store 75 <i64*>, 76 <i64**> 
-----------------

	*/
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

// Expected results: (CURRENT SLICING RESULT IS WRONG, THE "8888" STATEMENT SHOULD BE TAKEN, BUT IN FACT NOT!)


// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 48: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 49: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 61: TID: 0: INSTRID: 61: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %8 = load i64* @V1, align 8                     ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 62: TID: 0: INSTRID: 62: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %9 = trunc i64 %8 to i32                        ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 63: TID: 0: INSTRID: 63: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %9, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 64: TID: 0: INSTRID: 64: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %10 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 65: TID: 0: INSTRID: 65: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %10, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 66: TID: 0: INSTRID: 66: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %retval1 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 67: TID: 0: INSTRID: 67: TAKEN: TEST_TARGET: INSTR: F: main: BB: entry:   ret i32 %retval1



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 68;  numTakenInstrs: 9;  numExedBrs: 0;  numTakenBrs: 0;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 68/68;  numTakenExtCalls/numExedExtCalls: 0/5;


