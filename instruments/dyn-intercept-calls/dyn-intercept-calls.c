/*
 * Use LD_PRELOAD to intercept some important libc function calls for diagnosing x86 programs.
 */

#define _GNU_SOURCE
//#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#define PROJECT_TAG "DIRECT_SYM"
#define RESOLVE(x)	if (!fp_##x && !(fp_##x = dlsym(RTLD_NEXT, #x))) { fprintf(stderr, #x"() not found!\n"); exit(-1); }
#define DEBUGF(a...)	if (debug) { fprintf(stderr, "%s: %s(): ", PROJECT_TAG, __FUNCTION__); fprintf(stderr, ##a); }


// Debug flag.
static int debug = 1;


static FILE *(*fp_fopen)(const char *, const char *);
FILE *fopen(const char *filename, const char *type) {
  RESOLVE(fopen);
  FILE *fp = fp_fopen(filename, type);
  DEBUGF("fp = %p, filename = %s, type = %s.\n\n", (void *)fp, filename, type);
  return fp;
}


static FILE *(*fp_fopen64)(const char *filename, const char *type);
FILE *fopen64(const char *filename, const char *type) {
  RESOLVE(fopen64);
  FILE *fp = fp_fopen64(filename, type);
  DEBUGF("fp = %p, filename = %s, type = %s.\n\n", (void *)fp, filename, type);
  return fp;
}


static int (*fp_fclose)(FILE *);
int fclose(FILE *stream) {
  int fd = fileno(stream);
  RESOLVE(fclose);
  int result = fp_fclose(stream);
  DEBUGF("fp = %p, fd = %d, result = %d.\n\n", (void *)stream, fd, result);
  return result;
}


static int (*fp_fflush) ( FILE * stream );
int fflush ( FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fflush);
  int result = fp_fflush(stream);
  DEBUGF("fp = %p, fd = %d, result = %d.\n\n", (void *)stream, fd, result);
  return result;
}


static int (*fp_fflush_unlocked) ( FILE * stream );
int fflush_unlocked ( FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fflush_unlocked);
  int result = fp_fflush_unlocked(stream);
  DEBUGF("fp = %p, fd = %d, result = %d.\n\n", (void *)stream, fd, result);
  return result;
}


static int (*fp_feof) ( FILE * stream );
int feof ( FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(feof);
  int result = fp_feof(stream);
  DEBUGF("fp = %p, fd = %d, result = %d.\n\n", (void *)stream, fd, result);
  return result;
}


static int (*fp_feof_unlocked) ( FILE * stream );
int feof_unlocked ( FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(feof_unlocked);
  int result = fp_feof_unlocked(stream);
  DEBUGF("fp = %p, fd = %d, result = %d.\n\n", (void *)stream, fd, result);
  return result;
}


static int (*fp_ferror)( FILE * stream );
int ferror ( FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(ferror);
  int result = fp_ferror(stream);
  DEBUGF("fp = %p, fd = %d, result = %d.\n\n", (void *)stream, fd, result);
  return result;
}


static int (*fp_ferror_unlocked)( FILE * stream );
int ferror_unlocked ( FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(ferror_unlocked);
  int result = fp_ferror_unlocked(stream);
  DEBUGF("fp = %p, fd = %d, result = %d.\n\n", (void *)stream, fd, result);
  return result;
}


static size_t (*fp_fwrite) ( const void * ptr, size_t size, size_t count, FILE * stream );
size_t fwrite ( const void * ptr, size_t size, size_t count, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fwrite);
  int result = fp_fwrite(ptr, size, count, stream);
  DEBUGF("fp = %p, fd = %d, ptr = %p, size = %u, count = %u, result = %d.\n\n",
    (void *)stream, fd, (void *)ptr, (unsigned)size, (unsigned)count, result);
  return result;
}


static size_t (*fp_fwrite_unlocked) ( const void * ptr, size_t size, size_t count, FILE * stream );
size_t fwrite_unlocked ( const void * ptr, size_t size, size_t count, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fwrite_unlocked);
  int result = fp_fwrite_unlocked(ptr, size, count, stream);
  DEBUGF("fp = %p, fd = %d, ptr = %p, size = %u, count = %u, result = %d.\n\n",
    (void *)stream, fd, (void *)ptr, (unsigned)size, (unsigned)count, result);
  return result;
}


static size_t (*fp_fread) ( void * ptr, size_t size, size_t count, FILE * stream );
size_t fread ( void * ptr, size_t size, size_t count, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fread);
  int result = fp_fread(ptr, size, count, stream);
  DEBUGF("fp = %p, fd = %d, ptr = %p, size = %u, count = %u, result = %d.\n\n",
    (void *)stream, fd, (void *)ptr, (unsigned)size, (unsigned)count, result);
  return result;
}


static size_t (*fp_fread_unlocked) ( void * ptr, size_t size, size_t count, FILE * stream );
size_t fread_unlocked ( void * ptr, size_t size, size_t count, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fread_unlocked);
  int result = fp_fread_unlocked(ptr, size, count, stream);
  DEBUGF("fp = %p, fd = %d, ptr = %p, size = %u, count = %u, result = %d.\n\n",
    (void *)stream, fd, (void *)ptr, (unsigned)size, (unsigned)count, result);
  return result;
}


static int (*fp_fputc) ( int character, FILE * stream );
int fputc ( int character, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fputc);
  int result = fp_fputc(character, stream);
  DEBUGF("fp = %p, fd = %d, character = %d, result = %d.\n\n",
    (void *)stream, fd, character, result);
  return result;
}


static int (*fp_fputc_unlocked) ( int character, FILE * stream );
int fputc_unlocked ( int character, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fputc_unlocked);
  int result = fp_fputc_unlocked(character, stream);
  DEBUGF("fp = %p, fd = %d, character = %d, result = %d.\n\n",
    (void *)stream, fd, character, result);
  return result;
}


static int (*fp_fgetc) ( FILE * stream );
int fgetc ( FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fgetc);
  int result = fp_fgetc(stream);
  DEBUGF("fp = %p, fd = %d, result = %d.\n\n", (void *)stream, fd, result);
  return result;
}


static int (*fp_fgetc_unlocked) ( FILE * stream );
int fgetc_unlocked ( FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fgetc_unlocked);
  int result = fp_fgetc_unlocked(stream);
  DEBUGF("fp = %p, fd = %d, result = %d.\n\n", (void *)stream, fd, result);
  return result;
}


static int (*fp_getc) ( FILE * stream );
int getc ( FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(getc);
  int result = fp_getc(stream);
  DEBUGF("fp = %p, fd = %d, result = %d. (EOF = %d)\n\n", (void *)stream, fd, result, EOF);
  return result;
}


static int (*fp_getc_unlocked) ( FILE * stream );
int getc_unlocked ( FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(getc_unlocked);
  int result = fp_getc_unlocked(stream);
  DEBUGF("fp = %p, fd = %d, result = %d. (EOF = %d)\n\n", (void *)stream, fd, result, EOF);
  return result;
}


static int (*fp_ungetc) ( int character, FILE * stream );
int ungetc ( int character, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(ungetc);
  int result = fp_ungetc(character, stream);
  DEBUGF("fp = %p, fd = %d, character = %d, result = %d.\n\n",
    (void *)stream, fd, character, result);
  return result;
}


static int (*fp_ungetc_unlocked) ( int character, FILE * stream );
int ungetc_unlocked ( int character, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(ungetc_unlocked);
  int result = fp_ungetc_unlocked(character, stream);
  DEBUGF("fp = %p, fd = %d, character = %d, result = %d.\n\n",
    (void *)stream, fd, character, result);
  return result;
}


static int (*fp_fputs) ( const char * str, FILE * stream );
int fputs ( const char * str, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fputs);
  int result = fp_fputs(str, stream);
  DEBUGF("fp = %p, fd = %d, str = %s, result = %d.\n\n",
    (void *)stream, fd, str, result);
  return result;  
}


static int (*fp_fputs_unlocked) ( const char * str, FILE * stream );
int fputs_unlocked ( const char * str, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fputs_unlocked);
  int result = fp_fputs_unlocked(str, stream);
  DEBUGF("fp = %p, fd = %d, str = %s, result = %d.\n\n",
    (void *)stream, fd, str, result);
  return result;  
}


static char * (*fp_fgets) ( char * str, int num, FILE * stream );
char * fgets ( char * str, int num, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fgets);
  char *ret = fp_fgets(str, num, stream);
  DEBUGF("fp = %p, fd = %d, str = %s, num = %d, result str = %s.\n\n",
    (void *)stream, fd, str, num, ret);
  return ret;  
}


static char * (*fp_fgets_unlocked) ( char * str, int num, FILE * stream );
char * fgets_unlocked ( char * str, int num, FILE * stream ) {
  int fd = fileno(stream);
  RESOLVE(fgets_unlocked);
  char *ret = fp_fgets_unlocked(str, num, stream);
  DEBUGF("fp = %p, fd = %d, str = %s, num = %d, result str = %s.\n\n",
    (void *)stream, fd, str, num, ret);
  return ret;  
}


static int (*fp_fsync)(int fildes);
int fsync(int fildes) {
  RESOLVE(fsync);
  int result = fp_fsync(fildes);
  DEBUGF("fd = %d, result = %d.\n\n", fildes, result);
  return result;
}


static int (*fp_rename)(const char *oldStr, const char *newStr);
int rename(const char *oldStr, const char *newStr) {
  RESOLVE(rename);
  int result = fp_rename(oldStr, newStr);
  DEBUGF("old = %s, new = %s, result = %d.\n\n", oldStr, newStr, result);
  return result;
}


static void (*fp_exit)(int status) __attribute__ ((noreturn));
void exit(int status) {
  DEBUGF("status = %d.\n\n", status);
  RESOLVE(exit);
  fp_exit(status);
}


static void (*fp__exit)(int status) __attribute__ ((noreturn));
void _exit(int status) {
  DEBUGF("status = %d.\n\n", status);
  RESOLVE(_exit);
  fp__exit(status);
}


