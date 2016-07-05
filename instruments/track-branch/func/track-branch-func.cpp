
//#define _GNU_SOURCE
#define _LARGEFILE_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define PROJECT_TAG "DIRECT_SYM"

#ifdef __cplusplus
extern "C" {
#endif

static void *libHandle = NULL;
static int (*fp_fprintf) ( FILE * stream, const char * format, ... );

void dsym_track_branch(int instrId) {
  if (!libHandle) {
    libHandle = dlopen ("/lib/libc.so.6", RTLD_NOW);
    fp_fprintf = (int (*) ( FILE * stream, const char * format, ... ))dlsym(libHandle, "fprintf");
  }
  assert(libHandle && fp_fprintf);
  fp_fprintf(stderr, "%s-BR: INSTR-ID: %d\n", PROJECT_TAG, instrId);
}

#ifdef __cplusplus
}
#endif


