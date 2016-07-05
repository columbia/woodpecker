#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <error.h>

int globalVar = 0;

#ifdef __cplusplus
extern "C" {
#endif

void *
xmalloc (size_t bytes)
{
    char *cp;

    /* Parts of CVS try to xmalloc zero bytes and then free it.  Some
       systems have a malloc which returns NULL for zero byte
       allocations but a free which can't handle NULL, so compensate. */
    if (bytes == 0)
	bytes = 1;

    cp = (char *)malloc (bytes);
    if (cp == NULL)
    {
	char buf[80];
	sprintf (buf, "out of memory; can not allocate %lu bytes",
		 (unsigned long) bytes);
	error (1, 0, buf);
    }
    return (cp);
}

//strcat_filename_onto_homedir
char * strct_filename_onto_homedir (const char * dir, const char * file ) {
  char *path = (char *)xmalloc (strlen (dir) + 1 + strlen(file) + 1);
  sprintf (path, "%s\\%s", dir, file);
  return path;
}

void read_cvsrc(int argc, const char *argv[]) {
  if (argc > 4)
    globalVar++;
  strct_filename_onto_homedir(argv[0], argv[1]);
}

#ifdef __cplusplus
}
#endif

/* Slicing target: the last return instruction, including the returned value.
    Testcase purpose 1: the not executed branch will modify shared variable; make 
    sure our algorithm will take this branch.
    Testcase purpose 2: make sure whether the return instruction and the call 
    instruction to foo() will be taken.
*/
int main (int argc, const char *argv[]) {
  fprintf(stderr, "argv[0] = (%s)\n", argv[0]);
  read_cvsrc(argc, argv);
  return globalVar;
}

/*  Testing purpose: TBD.
*/

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc
// RUN: %kleebindir/klee --use-one-checker=Assert --use-path-slicer=1 %s.bc aaa 2> %s.output
// RUN: cat %s.output | FileCheck %s

// Expected results:
//// CHECK: STAT
// I have been able to reproduce this error if I rename "strct_filename_onto_homedir" to be
// "strcat_filename_onto_homedir". the problem is probably at the bc2bdd summary of "strcat"...

