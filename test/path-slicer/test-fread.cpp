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

// RUN: %kleebindir/klee --use-one-checker=OpenClose --use-path-slicer=1 \
// RUN: --emit-all-errors --max-time 10 --libc=uclibc --posix-runtime --init-env \
// RUN: %s.bc --sym-args 1 10 10 --sym-files 2 2000 --max-fail 1 2> %s.output

// RUN: cat %s.output | FileCheck %s


// Expected results:
// CHECK: KLEE: ERROR: checker reported error


