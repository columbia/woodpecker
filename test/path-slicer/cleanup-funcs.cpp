#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <assert.h>

long V1 = 0;
long V2 = 0;

#define CLEANUP_FUNCTIONS 5
typedef /*@null@*/void (*cleanup_function) (/*@null@*/void *arg);
typedef /*@null@*/void * parg_t;
static pid_t cleanup_pid = 0;

static cleanup_function cleanup_functions[CLEANUP_FUNCTIONS];
static parg_t cleanup_function_args[CLEANUP_FUNCTIONS];

void add_cleanup (/*@notnull@*/cleanup_function pcf, /*@null@*/void *arg);
void do_cleanups (void);

void do_cleanups (void)
{
	unsigned int i;

	/* Make sure there were no overflow */
	assert (NULL == cleanup_functions[CLEANUP_FUNCTIONS-1]);

	if (getpid () != cleanup_pid) {
		return;
	}

	i = CLEANUP_FUNCTIONS;
	do {
		i--;
		if (cleanup_functions[i] != NULL) {
			cleanup_functions[i] (cleanup_function_args[i]);
      		V1 += 888;
		}
	} while (i>0);
}

/*
 * add_cleanup - Add a cleanup_function to the cleanup_functions stack.
 */
void add_cleanup (/*@notnull@*/cleanup_function pcf, /*@null@*/void *arg)
{
	unsigned int i;
	assert (NULL != pcf);

	assert (NULL == cleanup_functions[CLEANUP_FUNCTIONS-2]);

	if (0 == cleanup_pid) {
		cleanup_pid = getpid ();
	}

	/* Add the cleanup_function at the end of the stack */
	for (i=0; NULL != cleanup_functions[i]; i++);
	cleanup_functions[i] = pcf;
	cleanup_function_args[i] = arg;
}

void cleanup_report_add_group_group (void *group_name)
{
	const char *name = (const char *)group_name;
	fprintf(stderr, "cleanup_report_add_group_group name %s\n", name);
  	V1 += 1111;
}

void cleanup_report_add_group_group2 (void *group_name)
{
	const char *name = (const char *)group_name;
	fprintf(stderr, "cleanup_report_add_group_group name %s\n", name);
  	V2 += 2222;
}

int main (int argc, char *argv[]) {
  add_cleanup(cleanup_report_add_group_group, argv[0]);
  add_cleanup(cleanup_report_add_group_group2, argv[0]);
  do_cleanups();
  fprintf(stderr, "V1 %ld\n", V1);
  return V1;
}

/*  Testing purpose:
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:

// CHECK: IntraSlicer::calStat TAKEN: IDX: 1: TID: 0: INSTRID: 115: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 116: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 117: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 6: TID: 0: INSTRID: 120: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 121: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %1 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 122: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %2 = getelementptr inbounds i8** %1, i64 0      ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 123: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %3 = load i8** %2, align 1                      ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 10: TID: 0: INSTRID: 124: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z11add_cleanupPFvPvES_(void (i8*)* @_Z30cleanup_report_add_group_groupPv, i8* %3) nounwind

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 26: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   %pcf_addr = alloca void (i8*)*                  ; <void (i8*)**> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 27: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   %arg_addr = alloca i8*                          ; <i8**> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 28: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   %i = alloca i32                                 ; <i32*> [#uses=5]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 15: TID: 0: INSTRID: 30: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   store void (i8*)* %pcf, void (i8*)** %pcf_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 16: TID: 0: INSTRID: 31: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   store i8* %arg, i8** %arg_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 17: TID: 0: INSTRID: 32: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   %0 = load void (i8*)** %pcf_addr, align 8       ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 33: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   %1 = icmp eq void (i8*)* %0, null               ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 34: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   br i1 %1, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 37: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb1:   %2 = load void (i8*)** getelementptr inbounds ([5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 3), align 8 ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 38: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb1:   %3 = icmp ne void (i8*)* %2, null               ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 39: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb1:   br i1 %3, label %bb2, label %bb3

// CHECK: IntraSlicer::calStat TAKEN: IDX: 23: TID: 0: INSTRID: 42: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb3:   %4 = load i32* @_ZL11cleanup_pid, align 4       ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 24: TID: 0: INSTRID: 43: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb3:   %5 = icmp eq i32 %4, 0                          ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 25: TID: 0: INSTRID: 44: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb3:   br i1 %5, label %bb4, label %bb5

// CHECK: IntraSlicer::calStat TAKEN: IDX: 26: TID: 0: INSTRID: 45: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb4:   %6 = call i32 @getpid() nounwind                ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 27: TID: 0: INSTRID: 46: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb4:   store i32 %6, i32* @_ZL11cleanup_pid, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 29: TID: 0: INSTRID: 48: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb5:   store i32 0, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 31: TID: 0: INSTRID: 53: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %8 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 32: TID: 0: INSTRID: 54: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %9 = zext i32 %8 to i64                         ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 33: TID: 0: INSTRID: 55: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %10 = getelementptr inbounds [5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 %9 ; <void (i8*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 34: TID: 0: INSTRID: 56: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %11 = load void (i8*)** %10, align 8            ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 35: TID: 0: INSTRID: 57: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %12 = icmp ne void (i8*)* %11, null             ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 36: TID: 0: INSTRID: 58: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %13 = load i32* %i, align 4                     ; <i32> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 37: TID: 0: INSTRID: 59: TAKEN: INTRA_BR_WR_BETWEEN: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   br i1 %12, label %bb6, label %bb8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 38: TID: 0: INSTRID: 60: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %14 = zext i32 %13 to i64                       ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 39: TID: 0: INSTRID: 61: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %15 = getelementptr inbounds [5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 %14 ; <void (i8*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 40: TID: 0: INSTRID: 62: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %16 = load void (i8*)** %pcf_addr, align 8      ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 41: TID: 0: INSTRID: 63: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   store void (i8*)* %16, void (i8*)** %15, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 42: TID: 0: INSTRID: 64: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %17 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 43: TID: 0: INSTRID: 65: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %18 = zext i32 %17 to i64                       ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 44: TID: 0: INSTRID: 66: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %19 = getelementptr inbounds [5 x i8*]* @_ZL21cleanup_function_args, i64 0, i64 %18 ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 45: TID: 0: INSTRID: 67: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %20 = load i8** %arg_addr, align 8              ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 46: TID: 0: INSTRID: 68: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   store i8* %20, i8** %19, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 47: TID: 0: INSTRID: 69: TAKEN: INTRA_RET_BOTH: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 48: TID: 0: INSTRID: 125: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %4 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 49: TID: 0: INSTRID: 126: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %5 = getelementptr inbounds i8** %4, i64 0      ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 50: TID: 0: INSTRID: 127: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %6 = load i8** %5, align 1                      ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 51: TID: 0: INSTRID: 128: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z11add_cleanupPFvPvES_(void (i8*)* @_Z31cleanup_report_add_group_group2Pv, i8* %6) nounwind

// CHECK: IntraSlicer::calStat TAKEN: IDX: 52: TID: 0: INSTRID: 26: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   %pcf_addr = alloca void (i8*)*                  ; <void (i8*)**> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 53: TID: 0: INSTRID: 27: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   %arg_addr = alloca i8*                          ; <i8**> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 54: TID: 0: INSTRID: 28: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   %i = alloca i32                                 ; <i32*> [#uses=5]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 56: TID: 0: INSTRID: 30: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   store void (i8*)* %pcf, void (i8*)** %pcf_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 57: TID: 0: INSTRID: 31: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   store i8* %arg, i8** %arg_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 58: TID: 0: INSTRID: 32: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   %0 = load void (i8*)** %pcf_addr, align 8       ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 59: TID: 0: INSTRID: 33: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   %1 = icmp eq void (i8*)* %0, null               ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 60: TID: 0: INSTRID: 34: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: entry:   br i1 %1, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 61: TID: 0: INSTRID: 37: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb1:   %2 = load void (i8*)** getelementptr inbounds ([5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 3), align 8 ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 62: TID: 0: INSTRID: 38: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb1:   %3 = icmp ne void (i8*)* %2, null               ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 63: TID: 0: INSTRID: 39: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb1:   br i1 %3, label %bb2, label %bb3

// CHECK: IntraSlicer::calStat TAKEN: IDX: 64: TID: 0: INSTRID: 42: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb3:   %4 = load i32* @_ZL11cleanup_pid, align 4       ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 65: TID: 0: INSTRID: 43: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb3:   %5 = icmp eq i32 %4, 0                          ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 66: TID: 0: INSTRID: 44: TAKEN: INTRA_BR_WR_BETWEEN: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb3:   br i1 %5, label %bb4, label %bb5

// CHECK: IntraSlicer::calStat TAKEN: IDX: 67: TID: 0: INSTRID: 48: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb5:   store i32 0, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 69: TID: 0: INSTRID: 53: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %8 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 70: TID: 0: INSTRID: 54: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %9 = zext i32 %8 to i64                         ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 71: TID: 0: INSTRID: 55: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %10 = getelementptr inbounds [5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 %9 ; <void (i8*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 72: TID: 0: INSTRID: 56: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %11 = load void (i8*)** %10, align 8            ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 73: TID: 0: INSTRID: 57: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %12 = icmp ne void (i8*)* %11, null             ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 74: TID: 0: INSTRID: 58: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %13 = load i32* %i, align 4                     ; <i32> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 75: TID: 0: INSTRID: 59: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   br i1 %12, label %bb6, label %bb8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 76: TID: 0: INSTRID: 50: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb6:   %7 = add i32 %13, 1                             ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 77: TID: 0: INSTRID: 51: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb6:   store i32 %7, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 79: TID: 0: INSTRID: 53: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %8 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 80: TID: 0: INSTRID: 54: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %9 = zext i32 %8 to i64                         ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 81: TID: 0: INSTRID: 55: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %10 = getelementptr inbounds [5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 %9 ; <void (i8*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 82: TID: 0: INSTRID: 56: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %11 = load void (i8*)** %10, align 8            ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 83: TID: 0: INSTRID: 57: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %12 = icmp ne void (i8*)* %11, null             ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 84: TID: 0: INSTRID: 58: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   %13 = load i32* %i, align 4                     ; <i32> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 85: TID: 0: INSTRID: 59: TAKEN: INTRA_BR_WR_BETWEEN: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb7:   br i1 %12, label %bb6, label %bb8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 86: TID: 0: INSTRID: 60: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %14 = zext i32 %13 to i64                       ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 87: TID: 0: INSTRID: 61: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %15 = getelementptr inbounds [5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 %14 ; <void (i8*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 88: TID: 0: INSTRID: 62: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %16 = load void (i8*)** %pcf_addr, align 8      ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 89: TID: 0: INSTRID: 63: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   store void (i8*)* %16, void (i8*)** %15, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 90: TID: 0: INSTRID: 64: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %17 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 91: TID: 0: INSTRID: 65: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %18 = zext i32 %17 to i64                       ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 92: TID: 0: INSTRID: 66: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %19 = getelementptr inbounds [5 x i8*]* @_ZL21cleanup_function_args, i64 0, i64 %18 ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 93: TID: 0: INSTRID: 67: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   %20 = load i8** %arg_addr, align 8              ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 94: TID: 0: INSTRID: 68: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   store i8* %20, i8** %19, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 95: TID: 0: INSTRID: 69: TAKEN: INTRA_RET_BOTH: INSTR: F: _Z11add_cleanupPFvPvES_: BB: bb8:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 96: TID: 0: INSTRID: 129: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z11do_cleanupsv()

// CHECK: IntraSlicer::calStat TAKEN: IDX: 97: TID: 0: INSTRID: 70: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: entry:   %retval.0 = alloca i8                           ; <i8*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 98: TID: 0: INSTRID: 71: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: entry:   %i = alloca i32                                 ; <i32*> [#uses=7]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 100: TID: 0: INSTRID: 73: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: entry:   %0 = load void (i8*)** getelementptr inbounds ([5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 4), align 8 ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 101: TID: 0: INSTRID: 74: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: entry:   %1 = icmp ne void (i8*)* %0, null               ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 102: TID: 0: INSTRID: 75: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11do_cleanupsv: BB: entry:   br i1 %1, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 103: TID: 0: INSTRID: 78: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: _Z11do_cleanupsv: BB: bb1:   %2 = call i32 @getpid() nounwind                ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 104: TID: 0: INSTRID: 79: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb1:   %3 = load i32* @_ZL11cleanup_pid, align 4       ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 105: TID: 0: INSTRID: 80: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb1:   %4 = icmp ne i32 %2, %3                         ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 106: TID: 0: INSTRID: 81: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb1:   %5 = zext i1 %4 to i8                           ; <i8> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 107: TID: 0: INSTRID: 82: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11do_cleanupsv: BB: bb1:   store i8 %5, i8* %retval.0, align 1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 108: TID: 0: INSTRID: 83: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb1:   %6 = load i8* %retval.0, align 1                ; <i8> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 109: TID: 0: INSTRID: 84: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb1:   %toBool = icmp ne i8 %6, 0                      ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 110: TID: 0: INSTRID: 85: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11do_cleanupsv: BB: bb1:   br i1 %toBool, label %return, label %bb2

// CHECK: IntraSlicer::calStat TAKEN: IDX: 111: TID: 0: INSTRID: 86: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11do_cleanupsv: BB: bb2:   store i32 5, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 113: TID: 0: INSTRID: 88: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %7 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 114: TID: 0: INSTRID: 89: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %8 = sub i32 %7, 1                              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 115: TID: 0: INSTRID: 90: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   store i32 %8, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 116: TID: 0: INSTRID: 91: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %9 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 117: TID: 0: INSTRID: 92: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %10 = zext i32 %9 to i64                        ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 118: TID: 0: INSTRID: 93: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %11 = getelementptr inbounds [5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 %10 ; <void (i8*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 119: TID: 0: INSTRID: 94: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %12 = load void (i8*)** %11, align 8            ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 120: TID: 0: INSTRID: 95: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %13 = icmp ne void (i8*)* %12, null             ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 121: TID: 0: INSTRID: 96: TAKEN: INTRA_BR_WR_BETWEEN: INSTR: F: _Z11do_cleanupsv: BB: bb3:   br i1 %13, label %bb4, label %bb5

// CHECK: IntraSlicer::calStat TAKEN: IDX: 122: TID: 0: INSTRID: 110: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb5:   %24 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 123: TID: 0: INSTRID: 111: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb5:   %25 = icmp ne i32 %24, 0                        ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 124: TID: 0: INSTRID: 112: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11do_cleanupsv: BB: bb5:   br i1 %25, label %bb3, label %return

// CHECK: IntraSlicer::calStat TAKEN: IDX: 125: TID: 0: INSTRID: 88: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %7 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 126: TID: 0: INSTRID: 89: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %8 = sub i32 %7, 1                              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 127: TID: 0: INSTRID: 90: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   store i32 %8, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 128: TID: 0: INSTRID: 91: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %9 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 129: TID: 0: INSTRID: 92: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %10 = zext i32 %9 to i64                        ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 130: TID: 0: INSTRID: 93: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %11 = getelementptr inbounds [5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 %10 ; <void (i8*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 131: TID: 0: INSTRID: 94: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %12 = load void (i8*)** %11, align 8            ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 132: TID: 0: INSTRID: 95: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %13 = icmp ne void (i8*)* %12, null             ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 133: TID: 0: INSTRID: 96: TAKEN: INTRA_BR_WR_BETWEEN: INSTR: F: _Z11do_cleanupsv: BB: bb3:   br i1 %13, label %bb4, label %bb5

// CHECK: IntraSlicer::calStat TAKEN: IDX: 134: TID: 0: INSTRID: 110: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb5:   %24 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 135: TID: 0: INSTRID: 111: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb5:   %25 = icmp ne i32 %24, 0                        ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 136: TID: 0: INSTRID: 112: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11do_cleanupsv: BB: bb5:   br i1 %25, label %bb3, label %return

// CHECK: IntraSlicer::calStat TAKEN: IDX: 137: TID: 0: INSTRID: 88: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %7 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 138: TID: 0: INSTRID: 89: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %8 = sub i32 %7, 1                              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 139: TID: 0: INSTRID: 90: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   store i32 %8, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 140: TID: 0: INSTRID: 91: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %9 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 141: TID: 0: INSTRID: 92: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %10 = zext i32 %9 to i64                        ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 142: TID: 0: INSTRID: 93: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %11 = getelementptr inbounds [5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 %10 ; <void (i8*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 143: TID: 0: INSTRID: 94: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %12 = load void (i8*)** %11, align 8            ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 144: TID: 0: INSTRID: 95: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %13 = icmp ne void (i8*)* %12, null             ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 145: TID: 0: INSTRID: 96: TAKEN: INTRA_BR_WR_BETWEEN: INSTR: F: _Z11do_cleanupsv: BB: bb3:   br i1 %13, label %bb4, label %bb5

// CHECK: IntraSlicer::calStat TAKEN: IDX: 146: TID: 0: INSTRID: 110: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb5:   %24 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 147: TID: 0: INSTRID: 111: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb5:   %25 = icmp ne i32 %24, 0                        ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 148: TID: 0: INSTRID: 112: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11do_cleanupsv: BB: bb5:   br i1 %25, label %bb3, label %return

// CHECK: IntraSlicer::calStat TAKEN: IDX: 149: TID: 0: INSTRID: 88: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %7 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 150: TID: 0: INSTRID: 89: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %8 = sub i32 %7, 1                              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 151: TID: 0: INSTRID: 90: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   store i32 %8, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 152: TID: 0: INSTRID: 91: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %9 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 153: TID: 0: INSTRID: 92: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %10 = zext i32 %9 to i64                        ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 154: TID: 0: INSTRID: 93: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %11 = getelementptr inbounds [5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 %10 ; <void (i8*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 155: TID: 0: INSTRID: 94: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %12 = load void (i8*)** %11, align 8            ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 156: TID: 0: INSTRID: 95: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %13 = icmp ne void (i8*)* %12, null             ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 157: TID: 0: INSTRID: 96: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   br i1 %13, label %bb4, label %bb5

// CHECK: IntraSlicer::calStat TAKEN: IDX: 180: TID: 0: INSTRID: 106: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %22 = load i64* @V1, align 8                    ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 181: TID: 0: INSTRID: 107: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %23 = add nsw i64 %22, 888                      ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 182: TID: 0: INSTRID: 108: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11do_cleanupsv: BB: bb4:   store i64 %23, i64* @V1, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 184: TID: 0: INSTRID: 110: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb5:   %24 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 185: TID: 0: INSTRID: 111: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb5:   %25 = icmp ne i32 %24, 0                        ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 186: TID: 0: INSTRID: 112: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11do_cleanupsv: BB: bb5:   br i1 %25, label %bb3, label %return

// CHECK: IntraSlicer::calStat TAKEN: IDX: 187: TID: 0: INSTRID: 88: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %7 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 188: TID: 0: INSTRID: 89: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %8 = sub i32 %7, 1                              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 189: TID: 0: INSTRID: 90: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   store i32 %8, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 190: TID: 0: INSTRID: 91: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %9 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 191: TID: 0: INSTRID: 92: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %10 = zext i32 %9 to i64                        ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 192: TID: 0: INSTRID: 93: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %11 = getelementptr inbounds [5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 %10 ; <void (i8*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 193: TID: 0: INSTRID: 94: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %12 = load void (i8*)** %11, align 8            ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 194: TID: 0: INSTRID: 95: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   %13 = icmp ne void (i8*)* %12, null             ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 195: TID: 0: INSTRID: 96: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z11do_cleanupsv: BB: bb3:   br i1 %13, label %bb4, label %bb5

// CHECK: IntraSlicer::calStat TAKEN: IDX: 196: TID: 0: INSTRID: 97: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %14 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 197: TID: 0: INSTRID: 98: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %15 = zext i32 %14 to i64                       ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 198: TID: 0: INSTRID: 99: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %16 = getelementptr inbounds [5 x void (i8*)*]* @_ZL17cleanup_functions, i64 0, i64 %15 ; <void (i8*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 199: TID: 0: INSTRID: 100: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %17 = load void (i8*)** %16, align 8            ; <void (i8*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 200: TID: 0: INSTRID: 101: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %18 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 201: TID: 0: INSTRID: 102: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %19 = zext i32 %18 to i64                       ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 202: TID: 0: INSTRID: 103: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %20 = getelementptr inbounds [5 x i8*]* @_ZL21cleanup_function_args, i64 0, i64 %19 ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 203: TID: 0: INSTRID: 104: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %21 = load i8** %20, align 8                    ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 204: TID: 0: INSTRID: 105: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb4:   call void %17(i8* %21)

// CHECK: IntraSlicer::calStat TAKEN: IDX: 214: TID: 0: INSTRID: 22: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z30cleanup_report_add_group_groupPv: BB: entry:   %4 = load i64* @V1, align 8                     ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 215: TID: 0: INSTRID: 23: TAKEN: INTRA_NON_MEM: INSTR: F: _Z30cleanup_report_add_group_groupPv: BB: entry:   %5 = add nsw i64 %4, 1111                       ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 216: TID: 0: INSTRID: 24: TAKEN: INTRA_STORE_OW: INSTR: F: _Z30cleanup_report_add_group_groupPv: BB: entry:   store i64 %5, i64* @V1, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 217: TID: 0: INSTRID: 25: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z30cleanup_report_add_group_groupPv: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 218: TID: 0: INSTRID: 106: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %22 = load i64* @V1, align 8                    ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 219: TID: 0: INSTRID: 107: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb4:   %23 = add nsw i64 %22, 888                      ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 220: TID: 0: INSTRID: 108: TAKEN: INTRA_STORE_OW: INSTR: F: _Z11do_cleanupsv: BB: bb4:   store i64 %23, i64* @V1, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 222: TID: 0: INSTRID: 110: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z11do_cleanupsv: BB: bb5:   %24 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 223: TID: 0: INSTRID: 111: TAKEN: INTRA_NON_MEM: INSTR: F: _Z11do_cleanupsv: BB: bb5:   %25 = icmp ne i32 %24, 0                        ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 224: TID: 0: INSTRID: 112: TAKEN: INTRA_BR_WR_BETWEEN: INSTR: F: _Z11do_cleanupsv: BB: bb5:   br i1 %25, label %bb3, label %return

// CHECK: IntraSlicer::calStat TAKEN: IDX: 225: TID: 0: INSTRID: 113: TAKEN: INTRA_RET_BOTH: INSTR: F: _Z11do_cleanupsv: BB: return:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 229: TID: 0: INSTRID: 133: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %10 = load i64* @V1, align 8                    ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 230: TID: 0: INSTRID: 134: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %11 = trunc i64 %10 to i32                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 231: TID: 0: INSTRID: 135: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %11, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 232: TID: 0: INSTRID: 136: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %12 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 233: TID: 0: INSTRID: 137: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %12, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 234: TID: 0: INSTRID: 138: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %retval1 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 235: TID: 0: INSTRID: 139: TAKEN: TEST_TARGET: INSTR: F: main: BB: entry:   ret i32 %retval1


// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 236;  numTakenInstrs: 189;  numExedBrs: 28;  numTakenBrs: 21;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 134/140;  numTakenExtCalls/numExedExtCalls: 2/5;
