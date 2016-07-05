#include <stdio.h>
#include <memory.h>
#include <assert.h>
#define LOOP 5
#define BASE 48

char x[LOOP];
int y = 0;

int main(int argc, char *argv[]) {
  int i;
  int cnt = 0;

  if (argc != 2) {
    return 0;
  }

  if (strlen(argv[1]) != LOOP) {
    return 0;
  } 

  // Irrelevant loop.
  for (i = 0; i < LOOP; i++) {
    int j;
    for (j = 0; j < LOOP; j++) {
      int k;
      for (k = 0; k < LOOP; k++) {
        int m = 0;
        for (m = 0; m < LOOP; m++) {
          int n = 0;
          for (n = 0; n < LOOP; n++) {
            y += argv[1][(i+j+k+m+n)%LOOP];
          }
        }
      }
    }
  }

  // Necessary loop.
  for (i = 0; i < LOOP; i++) {
    if (argv[1][i] == (char)(i+BASE)) {
      cnt++;
    }
  }

  // Critical branch.
  if (cnt == LOOP) {
    assert(0);
  }

  fprintf(stderr, "RETURN OK %d, y %d\n", cnt, y);
  return 0;
}

// Testing commands:
// RUN: %srcroot/test/path-slicer/build.sh %s
// RUN: %kleebindir/klee %s.bc 01234 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:
// CHECK: IntraSlicer::calStat STATISTICS: numExedInstrs:

