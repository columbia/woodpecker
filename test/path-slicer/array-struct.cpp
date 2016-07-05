#include <stdio.h>
#include <memory.h>
#include <assert.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

long V0 = 0;
long V1 = 0;
long V2 = 0;

struct cmd_struct {
  const char *cmd;
  int (*fn)(int, const char **, const char *);
};

int cmd_add(int argc, const char **argv, const char *prefix_) {
  V0 += 1001;
  assert(argc != 5);
  return 0;
}

int cmd_annotate(int argc, const char **argv, const char *prefix_) {
  V1 += 10001;
  assert(argc != 3);
  return 0;
}

/*int cmd_apply(int argc, const char **argv, const char *prefix_) {
  V2 += 100001;
  assert(argc < 4);
  return 0;
}*/

static void handle_internal_command(int argc, const char *argv[])
{
  const char *cmd = argv[1];
  static struct cmd_struct commands[] = {
    { "aa", cmd_add}
  };

  int i = 0;
  //for (; i < ARRAY_SIZE(commands); i++) {
    struct cmd_struct *p = commands+i;
    //if (strcmp(p->cmd, cmd))
      //continue;
    //p->fn = cmd_add;
    p->fn(argc, argv, p->cmd);
    //cmd_add(argc, argv, p->cmd);
    //return;
  //}
}

int main (int argc, const char *argv[]) {
  //handle_internal_command(argc, argv);
  long *arr[2][1];// = {{&V0}, {&V2}};
  long *brr[2][1];
  arr[0][0] = &V0;
  arr[1][0] = &V2;
  memcpy(brr, arr, sizeof(arr));
  *(arr[0][0]) = 101010;
  fprintf(stderr, "V0 %ld, V1 %ld, V2 %ld\n", V0, V1, V2);
  return (int)(V0 + V1);
}

/*  Testing purpose: This testcase has reproduced the bc2bdd problem: can not capture function pointer assignment in global scope.
    The taken slice does not contain any instruction in the bar1() function, because the bdd of "globalVar" has missed.
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc

// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 --max-time 10 --libc=uclibc --posix-runtime --init-env \
// RUN: %s.bc --sym-args 0 1 2 --sym-files 2 8 --max-fail 1 2> %s.output

// RUN: cat %s.output | FileCheck %s

// Expected results:

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 2: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 3: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 4: TID: 0: INSTRID: 4: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %arr = alloca [2 x [1 x i64*]]                  ; <[2 x [1 x i64*]]*> [#uses=4]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 9: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %1 = getelementptr inbounds [2 x [1 x i64*]]* %arr, i64 0, i64 0 ; <[1 x i64*]*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 10: TID: 0: INSTRID: 10: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %2 = getelementptr inbounds [1 x i64*]* %1, i64 0, i64 0 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 11: TAKEN: INTRA_STORE_ALIAS: INSTR: F: __user_main: BB: entry:   store i64* @V0, i64** %2, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 12: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %3 = getelementptr inbounds [2 x [1 x i64*]]* %arr, i64 0, i64 1 ; <[1 x i64*]*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 13: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %4 = getelementptr inbounds [1 x i64*]* %3, i64 0, i64 0 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 14: TID: 0: INSTRID: 14: TAKEN: INTRA_STORE_ALIAS: INSTR: F: __user_main: BB: entry:   store i64* @V2, i64** %4, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 20: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %10 = getelementptr inbounds [2 x [1 x i64*]]* %arr, i64 0, i64 0 ; <[1 x i64*]*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 21: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %11 = getelementptr inbounds [1 x i64*]* %10, i64 0, i64 0 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 22: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %12 = load i64** %11, align 8                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 23: TID: 0: INSTRID: 23: TAKEN: INTRA_STORE_ALIAS: INSTR: F: __user_main: BB: entry:   store i64 101010, i64* %12, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 29: TID: 0: INSTRID: 29: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %18 = load i64* @V0, align 8                    ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 30: TID: 0: INSTRID: 30: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %19 = trunc i64 %18 to i32                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 31: TID: 0: INSTRID: 31: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %20 = load i64* @V1, align 8                    ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 32: TID: 0: INSTRID: 32: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %21 = trunc i64 %20 to i32                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 33: TID: 0: INSTRID: 33: TAKEN: INTRA_NON_MEM: INSTR: F: __user_main: BB: entry:   %22 = add i32 %19, %21                          ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 34: TID: 0: INSTRID: 34: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: entry:   store i32 %22, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 35: TID: 0: INSTRID: 35: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %23 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 36: TID: 0: INSTRID: 36: TAKEN: INTRA_STORE_OW: INSTR: F: __user_main: BB: entry:   store i32 %23, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 37: TID: 0: INSTRID: 37: TAKEN: INTRA_LOAD_OW: INSTR: F: __user_main: BB: entry:   %retval1 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 38: TID: 0: INSTRID: 38: TAKEN: TEST_TARGET: INSTR: F: __user_main: BB: entry:   ret i32 %retval1



// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 39;  numTakenInstrs: 23;  numExedBrs: 0;  numTakenBrs: 0;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 39/83;  numTakenExtCalls/numExedExtCalls: 0/2;



