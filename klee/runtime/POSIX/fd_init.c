//===-- fd_init.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include "fd.h"
#include <klee/klee.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>


exe_file_system_t __exe_fs;

/* NOTE: It is important that these are statically initialized
   correctly, since things that run before main may hit them given the
   current way things are linked. */

/* XXX Technically these flags are initialized w.o.r. to the
   environment we are actually running in. We could patch them in
   klee_init_fds, but we still have the problem that uclibc calls
   prior to main will get the wrong data. Not such a big deal since we
   mostly care about sym case anyway. */


exe_sym_env_t __exe_env = { 
  {{ 0, eOpen | eReadable, 0, 0, CONCRETE_FILE}, 
   { 1, eOpen | eWriteable, 0, 0, CONCRETE_FILE}, 
   { 2, eOpen | eWriteable, 0, 0, CONCRETE_FILE}},
  022,
  0,
  0
};

static void __create_new_dfile(exe_disk_file_t *dfile, unsigned size, 
                               const char *name, struct stat64 *defaults) {
  struct stat64 *s = malloc(sizeof(*s));
  const char *sp;
  char sname[64];
  for (sp=name; *sp; ++sp)
    sname[sp-name] = *sp;
  memcpy(&sname[sp-name], "-stat", 6);

  assert(size);

  dfile->size = size;
  dfile->contents = malloc(dfile->size);
  klee_make_symbolic(dfile->contents, dfile->size, name);
  
  klee_make_symbolic(s, sizeof(*s), sname);

  /* For broken tests */
  if (!klee_is_symbolic(s->st_ino) && 
      (s->st_ino & 0x7FFFFFFF) == 0)
    s->st_ino = defaults->st_ino;
  
  /* Important since we copy this out through getdents, and readdir
     will otherwise skip this entry. For same reason need to make sure
     it fits in low bits. */
  klee_assume((s->st_ino & 0x7FFFFFFF) != 0);

  /* uclibc opendir uses this as its buffer size, try to keep
     reasonable. */
  klee_assume((s->st_blksize & ~0xFFFF) == 0);
  // XXX: workaround cat's concretize symbolic size problem
  // cat use insize in malloc, insize comes from input's block size
  // and if input is symbolic, block size is symbolic
  // but insize = MAX(32768, block size)
  // so assume blksize <= 32768 is enough
  // last statement limits it to be <65536
  // so... what do you want for 32768 < blksize < 65536 ?
  klee_assume(s->st_blksize <= 32768);

  klee_prefer_cex(s, !(s->st_mode & ~(S_IFMT | 0777)));
  klee_prefer_cex(s, s->st_dev == defaults->st_dev);
  klee_prefer_cex(s, s->st_rdev == defaults->st_rdev);
  klee_prefer_cex(s, (s->st_mode&0700) == 0600);
  klee_prefer_cex(s, (s->st_mode&0070) == 0020);
  klee_prefer_cex(s, (s->st_mode&0007) == 0002);
  klee_prefer_cex(s, (s->st_mode&S_IFMT) == S_IFREG);
  klee_prefer_cex(s, s->st_nlink == 1);
  klee_prefer_cex(s, s->st_uid == defaults->st_uid);
  klee_prefer_cex(s, s->st_gid == defaults->st_gid);
  klee_prefer_cex(s, s->st_blksize == 4096);
  klee_prefer_cex(s, s->st_atime == defaults->st_atime);
  klee_prefer_cex(s, s->st_mtime == defaults->st_mtime);
  klee_prefer_cex(s, s->st_ctime == defaults->st_ctime);

  s->st_size = dfile->size;
  s->st_blocks = 8;
  dfile->stat = s;
}

static unsigned __sym_uint32(const char *name) {
  unsigned x;
  klee_make_symbolic(&x, sizeof x, name);
  return x;
}

static void setup_std_fd(int std_fd, int int_fd, const char *path, int flag) {
  int i;
  __exe_env.fds[std_fd].dcfile = &__exe_fs.con_files[int_fd];
  __exe_fs.con_files[int_fd].refcnt = 1;
  __exe_fs.con_files[int_fd].fd = klee_sys_open(path, flag, 0);
  __exe_fs.con_files[int_fd].state = CON_STATE_STD;
  __exe_fs.con_files[int_fd].name = malloc(MAX_LINKS * sizeof(char*));
  __exe_fs.con_files[int_fd].name[0] = strdup(path);
  for (i=1; i<MAX_LINKS; i++) __exe_fs.con_files[int_fd].name[i] = NULL;
  __exe_fs.con_files[int_fd].size = 0;
  __exe_fs.con_files[int_fd].contents = NULL;
  __exe_fs.con_files[int_fd].stat = NULL;
}

/* n_files: number of symbolic input files, excluding stdin
   file_length: size in bytes of each symbolic file, including stdin
   sym_stdout_flag: 1 if stdout should be symbolic, 0 otherwise
   save_all_writes_flag: 1 if all writes are executed as expected, 0 if 
                         writes past the initial file size are discarded 
			 (file offset is always incremented)
   max_failures: maximum number of system call failures */
void klee_init_fds(unsigned n_files, unsigned file_length, 
		   int sym_stdout_flag, int save_all_writes_flag,
		   unsigned max_failures, unsigned max_malloc_fail,
           int etc_symbolic, int symbolic_size, int sym_specific) {
  unsigned k;
  char name[7] = "?-data";
  struct stat64 s;

  stat64(".", &s);

  __exe_fs.n_sym_files = n_files;
  __exe_fs.sym_files = malloc(sizeof(*__exe_fs.sym_files) * n_files);
  for (k=0; k < n_files; k++) {
    name[0] = 'A' + k;
    __create_new_dfile(&__exe_fs.sym_files[k], file_length, name, &s);
  }
  
  __exe_fs.n_con_files = INITIAL_CON_FILES;
  __exe_fs.con_files = malloc(sizeof(exe_disk_con_file_t) * __exe_fs.n_con_files);
  for (k=0; k<__exe_fs.n_con_files; k++) {
      __exe_fs.con_files[k].state = CON_STATE_FREE;
  }

  /* setting symbolic stdin */
  if (file_length) {
    __exe_fs.sym_stdin = malloc(sizeof(*__exe_fs.sym_stdin));
    __create_new_dfile(__exe_fs.sym_stdin, file_length, "stdin", &s);
    __exe_env.fds[0].dfile = __exe_fs.sym_stdin;
    __exe_env.fds[0].type = SYMBOLIC_FILE;
  } else {
      setup_std_fd(0, 0, "/dev/stdin", O_RDONLY);
      __exe_fs.sym_stdin = NULL;
  }

  __exe_fs.max_failures = max_failures;
  if (__exe_fs.max_failures) {
    __exe_fs.read_fail = malloc(sizeof(*__exe_fs.read_fail));
    __exe_fs.write_fail = malloc(sizeof(*__exe_fs.write_fail));
    __exe_fs.close_fail = malloc(sizeof(*__exe_fs.close_fail));
    __exe_fs.ftruncate_fail = malloc(sizeof(*__exe_fs.ftruncate_fail));
    __exe_fs.getcwd_fail = malloc(sizeof(*__exe_fs.getcwd_fail));

    klee_make_symbolic(__exe_fs.read_fail, sizeof(*__exe_fs.read_fail), "read_fail");
    klee_make_symbolic(__exe_fs.write_fail, sizeof(*__exe_fs.write_fail), "write_fail");
    klee_make_symbolic(__exe_fs.close_fail, sizeof(*__exe_fs.close_fail), "close_fail");
    klee_make_symbolic(__exe_fs.ftruncate_fail, sizeof(*__exe_fs.ftruncate_fail), "ftruncate_fail");
    klee_make_symbolic(__exe_fs.getcwd_fail, sizeof(*__exe_fs.getcwd_fail), "getcwd_fail");
  }

  __exe_fs.max_malloc_fail = max_malloc_fail;
  if (__exe_fs.max_malloc_fail) {
    __exe_fs.malloc_fail = malloc(sizeof(*__exe_fs.malloc_fail));
    klee_make_symbolic(__exe_fs.malloc_fail, sizeof(*__exe_fs.malloc_fail), "malloc_fail");
    klee_notify_counter(*__exe_fs.malloc_fail, "malloc_fail");
  }

  /* setting symbolic stdout */
  if (sym_stdout_flag) {
    __exe_fs.sym_stdout = malloc(sizeof(*__exe_fs.sym_stdout));
    __create_new_dfile(__exe_fs.sym_stdout, 1024, "stdout", &s);
    __exe_env.fds[1].dfile = __exe_fs.sym_stdout;
    __exe_env.fds[1].type = SYMBOLIC_FILE;
    __exe_fs.stdout_writes = 0;
  }
  else {
      setup_std_fd(1, 1, "/dev/stdout", O_WRONLY);
      __exe_fs.sym_stdout = NULL;
  }
  setup_std_fd(2, 2, "/dev/stderr", O_WRONLY);
  
  __exe_env.etc_symbolic = etc_symbolic;
  __exe_env.symbolic_size = symbolic_size;
  __exe_env.sym_specific = sym_specific;
  __exe_env.save_all_writes = save_all_writes_flag;
  __exe_env.version = __sym_uint32("model_version");
  klee_assume(__exe_env.version == 1);
}
