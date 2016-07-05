#include <stdio.h>
#include <memory.h>

long V0 = 0;
long V1 = 0;
long V2 = 0;

void bar1(int i) {
  V1 = 800;	// This statement totally overwrites the 101 one, so only this is taken, and that 101 one is not.
}

struct ptr_tiny_struct {
  long *x0;
  void (*fp_1)(int);
};

struct ptr_struct {
  struct ptr_tiny_struct *p_tiny;
  long *p0;
  long *p1;
  long *p2;
};

struct ptr_tiny_struct tiny = {&V0, bar1};

/* This function call to foo() is critical for this testcase, because we need a calling context in order
to reproduce this bug. If everything is in the main() function, then the passed in calling context to bc2bdd
would be NULL, then it becomes context insensitve, then the bug can not be captured. */
void foo(struct ptr_struct *ptrs, int argc, char *argv[]) {
  if (argc > 3)
    *(ptrs->p0) = 91;
  if (argc < 4)
    *(ptrs->p1) = 101;
  if (argc != 5)
    *(ptrs->p2) = 131;
  if (strstr(argv[0], "nested"))
    ptrs->p_tiny->fp_1(argc);
}

int main (int argc, char *argv[]) {
  struct ptr_struct ptrs = {&tiny, &V0, &V1, &V2};
  foo(&ptrs, argc, argv);
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
 

// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 51: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 1: TID: 0: INSTRID: 52: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 53: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 54: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 4: TID: 0: INSTRID: 55: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %ptrs = alloca %struct.ptr_struct               ; <%struct.ptr_struct*> [#uses=5]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 6: TID: 0: INSTRID: 57: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 58: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 59: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %1 = getelementptr inbounds %struct.ptr_struct* %ptrs, i32 0, i32 0 ; <%struct.ptr_tiny_struct**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 60: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %2 = load %struct.ptr_tiny_struct** getelementptr inbounds (%struct.ptr_struct* @_ZZ4mainE3C.0, i64 0, i32 0), align 8 ; <%struct.ptr_tiny_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 10: TID: 0: INSTRID: 61: TAKEN: INTRA_STORE_ALIAS: INSTR: F: main: BB: entry:   store %struct.ptr_tiny_struct* %2, %struct.ptr_tiny_struct** %1, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 71: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %9 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 72: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %10 = load i8*** %argv_addr, align 8            ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 73: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z3fooP10ptr_structiPPc(%struct.ptr_struct* %ptrs, i32 %9, i8** %10)

// CHECK: IntraSlicer::calStat TAKEN: IDX: 23: TID: 0: INSTRID: 5: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooP10ptr_structiPPc: BB: entry:   %ptrs_addr = alloca %struct.ptr_struct*         ; <%struct.ptr_struct**> [#uses=5]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 24: TID: 0: INSTRID: 6: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooP10ptr_structiPPc: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=5]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 25: TID: 0: INSTRID: 7: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooP10ptr_structiPPc: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 27: TID: 0: INSTRID: 9: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooP10ptr_structiPPc: BB: entry:   store %struct.ptr_struct* %ptrs, %struct.ptr_struct** %ptrs_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 28: TID: 0: INSTRID: 10: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooP10ptr_structiPPc: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 29: TID: 0: INSTRID: 11: TAKEN: INTRA_STORE_OW: INSTR: F: _Z3fooP10ptr_structiPPc: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 49: TID: 0: INSTRID: 36: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb5:   %15 = load i8*** %argv_addr, align 8            ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 50: TID: 0: INSTRID: 37: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb5:   %16 = getelementptr inbounds i8** %15, i64 0    ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 51: TID: 0: INSTRID: 38: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb5:   %17 = load i8** %16, align 1                    ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 52: TID: 0: INSTRID: 39: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb5:   %18 = call i8* @strstr(i8* %17, i8* getelementptr inbounds ([7 x i8]* @.str, i64 0, i64 0)) nounwind readonly ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 53: TID: 0: INSTRID: 40: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb5:   %19 = icmp ne i8* %18, null                     ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 54: TID: 0: INSTRID: 41: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb5:   br i1 %19, label %bb6, label %return

// CHECK: IntraSlicer::calStat TAKEN: IDX: 55: TID: 0: INSTRID: 42: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb6:   %20 = load %struct.ptr_struct** %ptrs_addr, align 8 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 56: TID: 0: INSTRID: 43: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb6:   %21 = getelementptr inbounds %struct.ptr_struct* %20, i32 0, i32 0 ; <%struct.ptr_tiny_struct**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 57: TID: 0: INSTRID: 44: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb6:   %22 = load %struct.ptr_tiny_struct** %21, align 8 ; <%struct.ptr_tiny_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 58: TID: 0: INSTRID: 45: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb6:   %23 = getelementptr inbounds %struct.ptr_tiny_struct* %22, i32 0, i32 1 ; <void (i32)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 59: TID: 0: INSTRID: 46: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb6:   %24 = load void (i32)** %23, align 8            ; <void (i32)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 60: TID: 0: INSTRID: 47: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb6:   %25 = load i32* %argc_addr, align 4             ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 61: TID: 0: INSTRID: 48: TAKEN: INTRA_NON_MEM: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb6:   call void %24(i32 %25)

// CHECK: IntraSlicer::calStat TAKEN: IDX: 65: TID: 0: INSTRID: 3: TAKEN: INTRA_STORE_OW: INSTR: F: _Z4bar1i: BB: entry:   store i64 800, i64* @V1, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 66: TID: 0: INSTRID: 4: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z4bar1i: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 67: TID: 0: INSTRID: 49: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z3fooP10ptr_structiPPc: BB: bb6:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 73: TID: 0: INSTRID: 79: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %16 = load i64* @V1, align 8                    ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 74: TID: 0: INSTRID: 80: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %17 = trunc i64 %16 to i32                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 75: TID: 0: INSTRID: 81: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %17, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 76: TID: 0: INSTRID: 82: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %18 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 77: TID: 0: INSTRID: 83: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %18, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 78: TID: 0: INSTRID: 84: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %retval1 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 79: TID: 0: INSTRID: 85: TAKEN: TEST_TARGET: INSTR: F: main: BB: entry:   ret i32 %retval1



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 80;  numTakenInstrs: 42;  numExedBrs: 6;  numTakenBrs: 1;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 80/86;  numTakenExtCalls/numExedExtCalls: 1/2;

