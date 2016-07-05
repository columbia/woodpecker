#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

long V0 = 0;
long V1 = 0;
long V2 = 0;

typedef void (*COMP_PROC)(int, char **, long *);

void bar1(int argc, char **argv, long *data) {
  (*data) += 222201;
}

void bar2(int argc, char **argv, long *data) {
  (*data) += 222233;
}

struct ptr_struct {
  COMP_PROC fp;
  long *data;
};

struct ptr_struct *table = NULL;
int curSize = 0;
int curCnt = 0;


struct ptr_struct *getBlock(int argc, char *argv[]) {
  if (curCnt + 1 > curSize) {
    const int block = 5;
    struct ptr_struct *temp =
			(struct ptr_struct *)realloc(table, (curSize+block)*sizeof(struct ptr_struct));
/*			(struct ptr_struct *)malloc((curSize+block)*sizeof(struct ptr_struct));
    memcpy(temp, table, curSize*sizeof(struct ptr_struct));*/
    curSize += block;
    table = temp;
  }
  return &table[curCnt];
}

void addFunc(int argc, char **argv, COMP_PROC fp, long *data) {
  struct ptr_struct *curBlock = getBlock(argc, argv);
  curBlock->fp = fp;
  curBlock->data = data;
  curCnt++;
}

void callFunc(int argc, char **argv) {
  for (int i = 0; i < curCnt; i++) {
    COMP_PROC curFp = table[i].fp;
    curFp(argc, argv, table[i].data);
  }
}

int main (int argc, char *argv[]) {
  /* All the three should be taken because realloc() modeled assignment which wrapped them up into one bdd. */
  addFunc(argc, argv, bar1, &V1); 
  addFunc(argc, argv, bar2, &V0);
  addFunc(argc, argv, bar1, &V2);
	/*addFunc(argc, argv, bar1, &V0);
	addFunc(argc, argv, bar1, &V0);addFunc(argc, argv, bar1, &V0);*/
	
	
  callFunc(argc, argv);
  fprintf(stderr, "V0 %ld, V1 %ld, V2 %ld, curCnt %d, curSize %d\n", V0, V1, V2, curCnt, curSize);
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


// CHECK: IntraSlicer::calStat TAKEN: IDX: 0: TID: 0: INSTRID: 126: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=5]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 1: TID: 0: INSTRID: 127: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=5]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 2: TID: 0: INSTRID: 128: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %retval = alloca i32                            ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 3: TID: 0: INSTRID: 129: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %0 = alloca i32                                 ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 5: TID: 0: INSTRID: 131: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 6: TID: 0: INSTRID: 132: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 7: TID: 0: INSTRID: 133: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %1 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 8: TID: 0: INSTRID: 134: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %2 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 9: TID: 0: INSTRID: 135: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z7addFunciPPcPFviS0_PlES1_(i32 %1, i8** %2, void (i32, i8**, i64*)* @_Z4bar1iPPcPl, i64* @V1) nounwind

// CHECK: IntraSlicer::calStat TAKEN: IDX: 10: TID: 0: INSTRID: 100: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 11: TID: 0: INSTRID: 101: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 12: TID: 0: INSTRID: 102: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %fp_addr = alloca void (i32, i8**, i64*)*       ; <void (i32, i8**, i64*)**> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 13: TID: 0: INSTRID: 103: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %data_addr = alloca i64*                        ; <i64**> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 16: TID: 0: INSTRID: 106: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 17: TID: 0: INSTRID: 107: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 18: TID: 0: INSTRID: 108: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store void (i32, i8**, i64*)* %fp, void (i32, i8**, i64*)** %fp_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 19: TID: 0: INSTRID: 109: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i64* %data, i64** %data_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 20: TID: 0: INSTRID: 110: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %0 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 21: TID: 0: INSTRID: 111: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %1 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 22: TID: 0: INSTRID: 112: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %2 = call %struct.ptr_struct* @_Z8getBlockiPPc(i32 %0, i8** %1) nounwind ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 28: TID: 0: INSTRID: 66: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: entry:   %temp = alloca %struct.ptr_struct*              ; <%struct.ptr_struct**> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 32: TID: 0: INSTRID: 70: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8getBlockiPPc: BB: entry:   %1 = load i32* @curCnt, align 4                 ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 33: TID: 0: INSTRID: 71: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: entry:   %2 = add nsw i32 %1, 1                          ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 34: TID: 0: INSTRID: 72: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8getBlockiPPc: BB: entry:   %3 = load i32* @curSize, align 4                ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 35: TID: 0: INSTRID: 73: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: entry:   %4 = icmp sgt i32 %2, %3                        ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 36: TID: 0: INSTRID: 74: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z8getBlockiPPc: BB: entry:   br i1 %4, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 38: TID: 0: INSTRID: 76: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8getBlockiPPc: BB: bb:   %5 = load i32* @curSize, align 4                ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 39: TID: 0: INSTRID: 77: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: bb:   %6 = add nsw i32 %5, 5                          ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 40: TID: 0: INSTRID: 78: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: bb:   %7 = sext i32 %6 to i64                         ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 41: TID: 0: INSTRID: 79: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: bb:   %8 = mul i64 %7, 16                             ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 42: TID: 0: INSTRID: 80: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8getBlockiPPc: BB: bb:   %9 = load %struct.ptr_struct** @table, align 8  ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 43: TID: 0: INSTRID: 81: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: bb:   %10 = bitcast %struct.ptr_struct* %9 to i8*     ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 44: TID: 0: INSTRID: 82: TAKEN: INTRA_EXT_CALL_REG_OW: INSTR: F: _Z8getBlockiPPc: BB: bb:   %11 = call i8* @realloc(i8* %10, i64 %8) nounwind ; <i8*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 45: TID: 0: INSTRID: 83: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: bb:   %12 = bitcast i8* %11 to %struct.ptr_struct*    ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 46: TID: 0: INSTRID: 84: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8getBlockiPPc: BB: bb:   store %struct.ptr_struct* %12, %struct.ptr_struct** %temp, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 47: TID: 0: INSTRID: 85: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8getBlockiPPc: BB: bb:   %13 = load i32* @curSize, align 4               ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 48: TID: 0: INSTRID: 86: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: bb:   %14 = add nsw i32 %13, 5                        ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 49: TID: 0: INSTRID: 87: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8getBlockiPPc: BB: bb:   store i32 %14, i32* @curSize, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 50: TID: 0: INSTRID: 88: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8getBlockiPPc: BB: bb:   %15 = load %struct.ptr_struct** %temp, align 8  ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 51: TID: 0: INSTRID: 89: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8getBlockiPPc: BB: bb:   store %struct.ptr_struct* %15, %struct.ptr_struct** @table, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 61: TID: 0: INSTRID: 99: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z8getBlockiPPc: BB: bb1:   ret %struct.ptr_struct* %retval2

// CHECK: IntraSlicer::calStat TAKEN: IDX: 62: TID: 0: INSTRID: 113: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store %struct.ptr_struct* %2, %struct.ptr_struct** %curBlock, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 63: TID: 0: INSTRID: 114: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %3 = load %struct.ptr_struct** %curBlock, align 8 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 64: TID: 0: INSTRID: 115: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %4 = getelementptr inbounds %struct.ptr_struct* %3, i32 0, i32 0 ; <void (i32, i8**, i64*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 65: TID: 0: INSTRID: 116: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %5 = load void (i32, i8**, i64*)** %fp_addr, align 8 ; <void (i32, i8**, i64*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 66: TID: 0: INSTRID: 117: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store void (i32, i8**, i64*)* %5, void (i32, i8**, i64*)** %4, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 67: TID: 0: INSTRID: 118: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %6 = load %struct.ptr_struct** %curBlock, align 8 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 68: TID: 0: INSTRID: 119: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %7 = getelementptr inbounds %struct.ptr_struct* %6, i32 0, i32 1 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 69: TID: 0: INSTRID: 120: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %8 = load i64** %data_addr, align 8             ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 70: TID: 0: INSTRID: 121: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i64* %8, i64** %7, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 71: TID: 0: INSTRID: 122: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %9 = load i32* @curCnt, align 4                 ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 72: TID: 0: INSTRID: 123: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %10 = add nsw i32 %9, 1                         ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 73: TID: 0: INSTRID: 124: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i32 %10, i32* @curCnt, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 74: TID: 0: INSTRID: 125: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 75: TID: 0: INSTRID: 136: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %3 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 76: TID: 0: INSTRID: 137: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %4 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 77: TID: 0: INSTRID: 138: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z7addFunciPPcPFviS0_PlES1_(i32 %3, i8** %4, void (i32, i8**, i64*)* @_Z4bar2iPPcPl, i64* @V0) nounwind

// CHECK: IntraSlicer::calStat TAKEN: IDX: 78: TID: 0: INSTRID: 100: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 79: TID: 0: INSTRID: 101: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 80: TID: 0: INSTRID: 102: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %fp_addr = alloca void (i32, i8**, i64*)*       ; <void (i32, i8**, i64*)**> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 81: TID: 0: INSTRID: 103: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %data_addr = alloca i64*                        ; <i64**> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 84: TID: 0: INSTRID: 106: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 85: TID: 0: INSTRID: 107: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 86: TID: 0: INSTRID: 108: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store void (i32, i8**, i64*)* %fp, void (i32, i8**, i64*)** %fp_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 87: TID: 0: INSTRID: 109: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i64* %data, i64** %data_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 88: TID: 0: INSTRID: 110: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %0 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 89: TID: 0: INSTRID: 111: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %1 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 90: TID: 0: INSTRID: 112: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %2 = call %struct.ptr_struct* @_Z8getBlockiPPc(i32 %0, i8** %1) nounwind ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 100: TID: 0: INSTRID: 70: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8getBlockiPPc: BB: entry:   %1 = load i32* @curCnt, align 4                 ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 101: TID: 0: INSTRID: 71: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: entry:   %2 = add nsw i32 %1, 1                          ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 102: TID: 0: INSTRID: 72: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8getBlockiPPc: BB: entry:   %3 = load i32* @curSize, align 4                ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 103: TID: 0: INSTRID: 73: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: entry:   %4 = icmp sgt i32 %2, %3                        ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 104: TID: 0: INSTRID: 74: TAKEN: INTRA_BR_WR_BETWEEN: INSTR: F: _Z8getBlockiPPc: BB: entry:   br i1 %4, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 113: TID: 0: INSTRID: 99: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z8getBlockiPPc: BB: bb1:   ret %struct.ptr_struct* %retval2

// CHECK: IntraSlicer::calStat TAKEN: IDX: 114: TID: 0: INSTRID: 113: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store %struct.ptr_struct* %2, %struct.ptr_struct** %curBlock, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 115: TID: 0: INSTRID: 114: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %3 = load %struct.ptr_struct** %curBlock, align 8 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 116: TID: 0: INSTRID: 115: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %4 = getelementptr inbounds %struct.ptr_struct* %3, i32 0, i32 0 ; <void (i32, i8**, i64*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 117: TID: 0: INSTRID: 116: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %5 = load void (i32, i8**, i64*)** %fp_addr, align 8 ; <void (i32, i8**, i64*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 118: TID: 0: INSTRID: 117: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store void (i32, i8**, i64*)* %5, void (i32, i8**, i64*)** %4, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 119: TID: 0: INSTRID: 118: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %6 = load %struct.ptr_struct** %curBlock, align 8 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 120: TID: 0: INSTRID: 119: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %7 = getelementptr inbounds %struct.ptr_struct* %6, i32 0, i32 1 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 121: TID: 0: INSTRID: 120: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %8 = load i64** %data_addr, align 8             ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 122: TID: 0: INSTRID: 121: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i64* %8, i64** %7, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 123: TID: 0: INSTRID: 122: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %9 = load i32* @curCnt, align 4                 ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 124: TID: 0: INSTRID: 123: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %10 = add nsw i32 %9, 1                         ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 125: TID: 0: INSTRID: 124: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i32 %10, i32* @curCnt, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 126: TID: 0: INSTRID: 125: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 127: TID: 0: INSTRID: 139: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %5 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 128: TID: 0: INSTRID: 140: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %6 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 129: TID: 0: INSTRID: 141: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z7addFunciPPcPFviS0_PlES1_(i32 %5, i8** %6, void (i32, i8**, i64*)* @_Z4bar1iPPcPl, i64* @V2) nounwind

// CHECK: IntraSlicer::calStat TAKEN: IDX: 130: TID: 0: INSTRID: 100: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 131: TID: 0: INSTRID: 101: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 132: TID: 0: INSTRID: 102: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %fp_addr = alloca void (i32, i8**, i64*)*       ; <void (i32, i8**, i64*)**> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 133: TID: 0: INSTRID: 103: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %data_addr = alloca i64*                        ; <i64**> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 136: TID: 0: INSTRID: 106: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 137: TID: 0: INSTRID: 107: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 138: TID: 0: INSTRID: 108: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store void (i32, i8**, i64*)* %fp, void (i32, i8**, i64*)** %fp_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 139: TID: 0: INSTRID: 109: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i64* %data, i64** %data_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 140: TID: 0: INSTRID: 110: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %0 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 141: TID: 0: INSTRID: 111: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %1 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 142: TID: 0: INSTRID: 112: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %2 = call %struct.ptr_struct* @_Z8getBlockiPPc(i32 %0, i8** %1) nounwind ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 152: TID: 0: INSTRID: 70: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8getBlockiPPc: BB: entry:   %1 = load i32* @curCnt, align 4                 ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 153: TID: 0: INSTRID: 71: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: entry:   %2 = add nsw i32 %1, 1                          ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 154: TID: 0: INSTRID: 72: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8getBlockiPPc: BB: entry:   %3 = load i32* @curSize, align 4                ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 155: TID: 0: INSTRID: 73: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8getBlockiPPc: BB: entry:   %4 = icmp sgt i32 %2, %3                        ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 156: TID: 0: INSTRID: 74: TAKEN: INTRA_BR_WR_BETWEEN: INSTR: F: _Z8getBlockiPPc: BB: entry:   br i1 %4, label %bb, label %bb1

// CHECK: IntraSlicer::calStat TAKEN: IDX: 165: TID: 0: INSTRID: 99: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z8getBlockiPPc: BB: bb1:   ret %struct.ptr_struct* %retval2

// CHECK: IntraSlicer::calStat TAKEN: IDX: 166: TID: 0: INSTRID: 113: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store %struct.ptr_struct* %2, %struct.ptr_struct** %curBlock, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 167: TID: 0: INSTRID: 114: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %3 = load %struct.ptr_struct** %curBlock, align 8 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 168: TID: 0: INSTRID: 115: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %4 = getelementptr inbounds %struct.ptr_struct* %3, i32 0, i32 0 ; <void (i32, i8**, i64*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 169: TID: 0: INSTRID: 116: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %5 = load void (i32, i8**, i64*)** %fp_addr, align 8 ; <void (i32, i8**, i64*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 170: TID: 0: INSTRID: 117: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store void (i32, i8**, i64*)* %5, void (i32, i8**, i64*)** %4, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 171: TID: 0: INSTRID: 118: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %6 = load %struct.ptr_struct** %curBlock, align 8 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 172: TID: 0: INSTRID: 119: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %7 = getelementptr inbounds %struct.ptr_struct* %6, i32 0, i32 1 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 173: TID: 0: INSTRID: 120: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %8 = load i64** %data_addr, align 8             ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 174: TID: 0: INSTRID: 121: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i64* %8, i64** %7, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 175: TID: 0: INSTRID: 122: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %9 = load i32* @curCnt, align 4                 ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 176: TID: 0: INSTRID: 123: TAKEN: INTRA_NON_MEM: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   %10 = add nsw i32 %9, 1                         ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 177: TID: 0: INSTRID: 124: TAKEN: INTRA_STORE_OW: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   store i32 %10, i32* @curCnt, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 178: TID: 0: INSTRID: 125: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z7addFunciPPcPFviS0_PlES1_: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 179: TID: 0: INSTRID: 142: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %7 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 180: TID: 0: INSTRID: 143: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %8 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 181: TID: 0: INSTRID: 144: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   call void @_Z8callFunciPPc(i32 %7, i8** %8)

// CHECK: IntraSlicer::calStat TAKEN: IDX: 182: TID: 0: INSTRID: 26: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: entry:   %argc_addr = alloca i32                         ; <i32*> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 183: TID: 0: INSTRID: 27: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: entry:   %argv_addr = alloca i8**                        ; <i8***> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 184: TID: 0: INSTRID: 28: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: entry:   %i = alloca i32                                 ; <i32*> [#uses=6]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 185: TID: 0: INSTRID: 29: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: entry:   %curFp = alloca void (i32, i8**, i64*)*         ; <void (i32, i8**, i64*)**> [#uses=2]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 187: TID: 0: INSTRID: 31: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8callFunciPPc: BB: entry:   store i32 %argc, i32* %argc_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 188: TID: 0: INSTRID: 32: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8callFunciPPc: BB: entry:   store i8** %argv, i8*** %argv_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 189: TID: 0: INSTRID: 33: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8callFunciPPc: BB: entry:   store i32 0, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 191: TID: 0: INSTRID: 56: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb1:   %17 = load i32* @curCnt, align 4                ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 192: TID: 0: INSTRID: 57: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb1:   %18 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 193: TID: 0: INSTRID: 58: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb1:   %19 = icmp slt i32 %18, %17                     ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 194: TID: 0: INSTRID: 59: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z8callFunciPPc: BB: bb1:   br i1 %19, label %bb, label %return

// CHECK: IntraSlicer::calStat TAKEN: IDX: 195: TID: 0: INSTRID: 35: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %0 = load %struct.ptr_struct** @table, align 8  ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 196: TID: 0: INSTRID: 36: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %1 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 197: TID: 0: INSTRID: 37: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %2 = sext i32 %1 to i64                         ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 198: TID: 0: INSTRID: 38: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %3 = getelementptr inbounds %struct.ptr_struct* %0, i64 %2 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 199: TID: 0: INSTRID: 39: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %4 = getelementptr inbounds %struct.ptr_struct* %3, i32 0, i32 0 ; <void (i32, i8**, i64*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 200: TID: 0: INSTRID: 40: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %5 = load void (i32, i8**, i64*)** %4, align 8  ; <void (i32, i8**, i64*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 201: TID: 0: INSTRID: 41: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   store void (i32, i8**, i64*)* %5, void (i32, i8**, i64*)** %curFp, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 202: TID: 0: INSTRID: 42: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %6 = load %struct.ptr_struct** @table, align 8  ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 203: TID: 0: INSTRID: 43: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %7 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 204: TID: 0: INSTRID: 44: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %8 = sext i32 %7 to i64                         ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 205: TID: 0: INSTRID: 45: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %9 = getelementptr inbounds %struct.ptr_struct* %6, i64 %8 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 206: TID: 0: INSTRID: 46: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %10 = getelementptr inbounds %struct.ptr_struct* %9, i32 0, i32 1 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 207: TID: 0: INSTRID: 47: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %11 = load i64** %10, align 8                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 208: TID: 0: INSTRID: 48: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %12 = load void (i32, i8**, i64*)** %curFp, align 8 ; <void (i32, i8**, i64*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 209: TID: 0: INSTRID: 49: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %13 = load i32* %argc_addr, align 4             ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 210: TID: 0: INSTRID: 50: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %14 = load i8*** %argv_addr, align 8            ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 211: TID: 0: INSTRID: 51: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   call void %12(i32 %13, i8** %14, i64* %11)

// CHECK: IntraSlicer::calStat TAKEN: IDX: 214: TID: 0: INSTRID: 2: TAKEN: INTRA_NON_MEM: INSTR: F: _Z4bar1iPPcPl: BB: entry:   %data_addr = alloca i64*                        ; <i64**> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 218: TID: 0: INSTRID: 6: TAKEN: INTRA_STORE_OW: INSTR: F: _Z4bar1iPPcPl: BB: entry:   store i64* %data, i64** %data_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 219: TID: 0: INSTRID: 7: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z4bar1iPPcPl: BB: entry:   %0 = load i64** %data_addr, align 8             ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 220: TID: 0: INSTRID: 8: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z4bar1iPPcPl: BB: entry:   %1 = load i64* %0, align 8                      ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 221: TID: 0: INSTRID: 9: TAKEN: INTRA_NON_MEM: INSTR: F: _Z4bar1iPPcPl: BB: entry:   %2 = add nsw i64 %1, 222201                     ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 222: TID: 0: INSTRID: 10: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z4bar1iPPcPl: BB: entry:   %3 = load i64** %data_addr, align 8             ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 223: TID: 0: INSTRID: 11: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z4bar1iPPcPl: BB: entry:   store i64 %2, i64* %3, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 224: TID: 0: INSTRID: 12: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z4bar1iPPcPl: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 225: TID: 0: INSTRID: 52: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %15 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 226: TID: 0: INSTRID: 53: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %16 = add nsw i32 %15, 1                        ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 227: TID: 0: INSTRID: 54: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   store i32 %16, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 229: TID: 0: INSTRID: 56: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb1:   %17 = load i32* @curCnt, align 4                ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 230: TID: 0: INSTRID: 57: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb1:   %18 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 231: TID: 0: INSTRID: 58: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb1:   %19 = icmp slt i32 %18, %17                     ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 232: TID: 0: INSTRID: 59: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z8callFunciPPc: BB: bb1:   br i1 %19, label %bb, label %return

// CHECK: IntraSlicer::calStat TAKEN: IDX: 233: TID: 0: INSTRID: 35: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %0 = load %struct.ptr_struct** @table, align 8  ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 234: TID: 0: INSTRID: 36: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %1 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 235: TID: 0: INSTRID: 37: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %2 = sext i32 %1 to i64                         ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 236: TID: 0: INSTRID: 38: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %3 = getelementptr inbounds %struct.ptr_struct* %0, i64 %2 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 237: TID: 0: INSTRID: 39: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %4 = getelementptr inbounds %struct.ptr_struct* %3, i32 0, i32 0 ; <void (i32, i8**, i64*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 238: TID: 0: INSTRID: 40: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %5 = load void (i32, i8**, i64*)** %4, align 8  ; <void (i32, i8**, i64*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 239: TID: 0: INSTRID: 41: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   store void (i32, i8**, i64*)* %5, void (i32, i8**, i64*)** %curFp, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 240: TID: 0: INSTRID: 42: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %6 = load %struct.ptr_struct** @table, align 8  ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 241: TID: 0: INSTRID: 43: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %7 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 242: TID: 0: INSTRID: 44: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %8 = sext i32 %7 to i64                         ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 243: TID: 0: INSTRID: 45: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %9 = getelementptr inbounds %struct.ptr_struct* %6, i64 %8 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 244: TID: 0: INSTRID: 46: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %10 = getelementptr inbounds %struct.ptr_struct* %9, i32 0, i32 1 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 245: TID: 0: INSTRID: 47: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %11 = load i64** %10, align 8                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 246: TID: 0: INSTRID: 48: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %12 = load void (i32, i8**, i64*)** %curFp, align 8 ; <void (i32, i8**, i64*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 247: TID: 0: INSTRID: 49: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %13 = load i32* %argc_addr, align 4             ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 248: TID: 0: INSTRID: 50: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %14 = load i8*** %argv_addr, align 8            ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 249: TID: 0: INSTRID: 51: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   call void %12(i32 %13, i8** %14, i64* %11)

// CHECK: IntraSlicer::calStat TAKEN: IDX: 252: TID: 0: INSTRID: 15: TAKEN: INTRA_NON_MEM: INSTR: F: _Z4bar2iPPcPl: BB: entry:   %data_addr = alloca i64*                        ; <i64**> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 256: TID: 0: INSTRID: 19: TAKEN: INTRA_STORE_OW: INSTR: F: _Z4bar2iPPcPl: BB: entry:   store i64* %data, i64** %data_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 257: TID: 0: INSTRID: 20: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z4bar2iPPcPl: BB: entry:   %0 = load i64** %data_addr, align 8             ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 258: TID: 0: INSTRID: 21: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z4bar2iPPcPl: BB: entry:   %1 = load i64* %0, align 8                      ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 259: TID: 0: INSTRID: 22: TAKEN: INTRA_NON_MEM: INSTR: F: _Z4bar2iPPcPl: BB: entry:   %2 = add nsw i64 %1, 222233                     ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 260: TID: 0: INSTRID: 23: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z4bar2iPPcPl: BB: entry:   %3 = load i64** %data_addr, align 8             ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 261: TID: 0: INSTRID: 24: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z4bar2iPPcPl: BB: entry:   store i64 %2, i64* %3, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 262: TID: 0: INSTRID: 25: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z4bar2iPPcPl: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 263: TID: 0: INSTRID: 52: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %15 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 264: TID: 0: INSTRID: 53: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %16 = add nsw i32 %15, 1                        ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 265: TID: 0: INSTRID: 54: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   store i32 %16, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 267: TID: 0: INSTRID: 56: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb1:   %17 = load i32* @curCnt, align 4                ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 268: TID: 0: INSTRID: 57: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb1:   %18 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 269: TID: 0: INSTRID: 58: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb1:   %19 = icmp slt i32 %18, %17                     ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 270: TID: 0: INSTRID: 59: TAKEN: INTRA_BR_N_POSTDOM: INSTR: F: _Z8callFunciPPc: BB: bb1:   br i1 %19, label %bb, label %return

// CHECK: IntraSlicer::calStat TAKEN: IDX: 271: TID: 0: INSTRID: 35: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %0 = load %struct.ptr_struct** @table, align 8  ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 272: TID: 0: INSTRID: 36: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %1 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 273: TID: 0: INSTRID: 37: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %2 = sext i32 %1 to i64                         ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 274: TID: 0: INSTRID: 38: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %3 = getelementptr inbounds %struct.ptr_struct* %0, i64 %2 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 275: TID: 0: INSTRID: 39: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %4 = getelementptr inbounds %struct.ptr_struct* %3, i32 0, i32 0 ; <void (i32, i8**, i64*)**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 276: TID: 0: INSTRID: 40: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %5 = load void (i32, i8**, i64*)** %4, align 8  ; <void (i32, i8**, i64*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 277: TID: 0: INSTRID: 41: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   store void (i32, i8**, i64*)* %5, void (i32, i8**, i64*)** %curFp, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 278: TID: 0: INSTRID: 42: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %6 = load %struct.ptr_struct** @table, align 8  ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 279: TID: 0: INSTRID: 43: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %7 = load i32* %i, align 4                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 280: TID: 0: INSTRID: 44: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %8 = sext i32 %7 to i64                         ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 281: TID: 0: INSTRID: 45: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %9 = getelementptr inbounds %struct.ptr_struct* %6, i64 %8 ; <%struct.ptr_struct*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 282: TID: 0: INSTRID: 46: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %10 = getelementptr inbounds %struct.ptr_struct* %9, i32 0, i32 1 ; <i64**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 283: TID: 0: INSTRID: 47: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %11 = load i64** %10, align 8                   ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 284: TID: 0: INSTRID: 48: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %12 = load void (i32, i8**, i64*)** %curFp, align 8 ; <void (i32, i8**, i64*)*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 285: TID: 0: INSTRID: 49: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %13 = load i32* %argc_addr, align 4             ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 286: TID: 0: INSTRID: 50: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %14 = load i8*** %argv_addr, align 8            ; <i8**> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 287: TID: 0: INSTRID: 51: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   call void %12(i32 %13, i8** %14, i64* %11)

// CHECK: IntraSlicer::calStat TAKEN: IDX: 290: TID: 0: INSTRID: 2: TAKEN: INTRA_NON_MEM: INSTR: F: _Z4bar1iPPcPl: BB: entry:   %data_addr = alloca i64*                        ; <i64**> [#uses=3]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 294: TID: 0: INSTRID: 6: TAKEN: INTRA_STORE_OW: INSTR: F: _Z4bar1iPPcPl: BB: entry:   store i64* %data, i64** %data_addr

// CHECK: IntraSlicer::calStat TAKEN: IDX: 295: TID: 0: INSTRID: 7: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z4bar1iPPcPl: BB: entry:   %0 = load i64** %data_addr, align 8             ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 296: TID: 0: INSTRID: 8: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z4bar1iPPcPl: BB: entry:   %1 = load i64* %0, align 8                      ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 297: TID: 0: INSTRID: 9: TAKEN: INTRA_NON_MEM: INSTR: F: _Z4bar1iPPcPl: BB: entry:   %2 = add nsw i64 %1, 222201                     ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 298: TID: 0: INSTRID: 10: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z4bar1iPPcPl: BB: entry:   %3 = load i64** %data_addr, align 8             ; <i64*> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 299: TID: 0: INSTRID: 11: TAKEN: INTRA_STORE_ALIAS: INSTR: F: _Z4bar1iPPcPl: BB: entry:   store i64 %2, i64* %3, align 8

// CHECK: IntraSlicer::calStat TAKEN: IDX: 300: TID: 0: INSTRID: 12: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z4bar1iPPcPl: BB: entry:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 301: TID: 0: INSTRID: 52: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   %15 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 302: TID: 0: INSTRID: 53: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb:   %16 = add nsw i32 %15, 1                        ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 303: TID: 0: INSTRID: 54: TAKEN: INTRA_STORE_OW: INSTR: F: _Z8callFunciPPc: BB: bb:   store i32 %16, i32* %i, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 305: TID: 0: INSTRID: 56: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb1:   %17 = load i32* @curCnt, align 4                ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 306: TID: 0: INSTRID: 57: TAKEN: INTRA_LOAD_OW: INSTR: F: _Z8callFunciPPc: BB: bb1:   %18 = load i32* %i, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 307: TID: 0: INSTRID: 58: TAKEN: INTRA_NON_MEM: INSTR: F: _Z8callFunciPPc: BB: bb1:   %19 = icmp slt i32 %18, %17                     ; <i1> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 308: TID: 0: INSTRID: 59: TAKEN: INTRA_BR_WR_BETWEEN: INSTR: F: _Z8callFunciPPc: BB: bb1:   br i1 %19, label %bb, label %return

// CHECK: IntraSlicer::calStat TAKEN: IDX: 309: TID: 0: INSTRID: 60: TAKEN: INTRA_RET_WRITE_FUNC: INSTR: F: _Z8callFunciPPc: BB: return:   ret void

// CHECK: IntraSlicer::calStat TAKEN: IDX: 317: TID: 0: INSTRID: 152: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %16 = load i64* @V1, align 8                    ; <i64> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 318: TID: 0: INSTRID: 153: TAKEN: INTRA_NON_MEM: INSTR: F: main: BB: entry:   %17 = trunc i64 %16 to i32                      ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 319: TID: 0: INSTRID: 154: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %17, i32* %0, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 320: TID: 0: INSTRID: 155: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %18 = load i32* %0, align 4                     ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 321: TID: 0: INSTRID: 156: TAKEN: INTRA_STORE_OW: INSTR: F: main: BB: entry:   store i32 %18, i32* %retval, align 4

// CHECK: IntraSlicer::calStat TAKEN: IDX: 322: TID: 0: INSTRID: 157: TAKEN: INTRA_LOAD_OW: INSTR: F: main: BB: entry:   %retval1 = load i32* %retval                    ; <i32> [#uses=1]

// CHECK: IntraSlicer::calStat TAKEN: IDX: 323: TID: 0: INSTRID: 158: TAKEN: TEST_TARGET: INSTR: F: main: BB: entry:   ret i32 %retval1


// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs: 324;  numTakenInstrs: 238;  numExedBrs: 12;  numTakenBrs: 7;  numExedSymBrs: 0;  numTakenSymBrs: 0; StaticExed/Static Instrs: 159/159;  numTakenExtCalls/numExedExtCalls: 1/2;

