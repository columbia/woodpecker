//===-- fd.c --------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define _LARGEFILE64_SOURCE
#include "fd.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <termios.h>
#include <sys/select.h>
#include <klee/klee.h>

/* #define DEBUG */

void klee_warning(const char*);
void klee_warning_once(const char*);
int klee_get_errno(void);

#define FAKE_FS
#define FAKE_CWD
#define MEMFS_DEBUG
#define SYM_SPECIFIC

#ifdef FAKE_CWD
char *current_path = NULL;
int current_path_len = 0;
char *get_realpath(const char *rel_path);
#endif
exe_disk_con_file_t *enlarge_con_files();
int find_con_file(char *filename, exe_disk_con_file_t **con_file, exe_disk_con_file_t **free_file);
int make_con_inmem(int fd, exe_disk_con_file_t *con_file);
void free_con_file(exe_disk_con_file_t *con_file);

/* Returns pointer to the symbolic file structure is the pathname is symbolic */
static exe_disk_file_t *__get_sym_file(const char *pathname) {
  char c = pathname[0];
  unsigned i;

  if (c == 0 || pathname[1] != 0)
    return NULL;

  for (i=0; i<__exe_fs.n_sym_files; ++i) {
    if (c == 'A' + (char) i) {
      exe_disk_file_t *df = &__exe_fs.sym_files[i];
      if (df->stat->st_ino == 0)
        return NULL;
      return df;
    }
  }
  
  return NULL;
}

static void *__concretize_ptr(const void *p);
static size_t __concretize_size(size_t s);
static const char *__concretize_string(const char *s);

/* Returns pointer to the file entry for a valid fd */
static exe_file_t *__get_file(int fd) {
  if (fd>=0 && fd<MAX_FDS) {
    exe_file_t *f = &__exe_env.fds[fd];
    if (f->flags & eOpen)
      return f;
  }

  return 0;
}

int access(const char *pathname, int mode) {
  exe_disk_file_t *dfile = __get_sym_file(pathname);
  
  if (dfile) {
    /* XXX we should check against stat values but we also need to
       enforce in open and friends then. */
    return 0;
  } else {
#ifdef FAKE_CWD
    char *rpath = get_realpath(__concretize_string(pathname));
#else
    const char *rpath = __concretize_string(pathname);
#endif
    char *con_fullpath = malloc(1024);
    char *realpath_ret = realpath(rpath, con_fullpath); 
    if (!realpath_ret) {
        if (errno == ENOENT) {
#ifdef CWD_DEBUG
            printf("ignore realpath() = ENOENT: maybe new file\n");
#endif
        } else {
            free(con_fullpath);
#ifdef MEMFS_DEBUG
            printf("access(%s) failed: realpath() fail %d\n", rpath, errno);
#endif
#ifdef FAKE_CWD
            free(rpath);
#endif
            return -1;
        }
    }

    exe_disk_con_file_t *dcfile;

    find_con_file(con_fullpath, &dcfile, NULL);

    free(con_fullpath);

    if (dcfile) {
        // fake file record exists
#ifdef FAKE_CWD
        free(rpath);
#endif
#ifdef MEMFS_DEBUG
        printf("access(%s): return OK for fake file\n", rpath);
#endif
        return 0;
    }

    int r = syscall(__NR_access, rpath, mode);
    if (r == -1)
      errno = klee_get_errno();
#ifdef FAKE_CWD
    free(rpath);
#endif
    return r;
  } 
}

mode_t umask(mode_t mask) {  
  mode_t r = __exe_env.umask;
  __exe_env.umask = mask & 0777;
  return r;
}


/* Returns 1 if the process has the access rights specified by 'flags'
   to the file with stat 's'.  Returns 0 otherwise*/
static int has_permission(int flags, struct stat64 *s) {
  int write_access, read_access;
  mode_t mode = s->st_mode;
  
  if (flags & O_RDONLY || flags & O_RDWR)
    read_access = 1;
  else read_access = 0;

  if (flags & O_WRONLY || flags & O_RDWR)
    write_access = 1;
  else write_access = 0;

  /* XXX: We don't worry about process uid and gid for now. 
     We allow access if any user has access to the file. */
#if 0
  uid_t uid = s->st_uid;
  uid_t euid = geteuid();
  gid_t gid = s->st_gid;
  gid_t egid = getegid();
#endif  

  if (read_access && ((mode & S_IRUSR) | (mode & S_IRGRP) | (mode & S_IROTH)))
    return 0;

  if (write_access && !((mode & S_IWUSR) | (mode & S_IWGRP) | (mode & S_IWOTH)))
    return 0;

  return 1;
}


int __fd_open(const char *pathname, int flags, mode_t mode) {
  exe_disk_file_t *df;
  exe_file_t *f;
  int fd;

  for (fd = 0; fd < MAX_FDS; ++fd)
    if (!(__exe_env.fds[fd].flags & eOpen))
      break;
  if (fd == MAX_FDS) {
    errno = EMFILE;
    return -1;
  }
  
  f = &__exe_env.fds[fd];

  /* Should be the case if file was available, but just in case. */
  memset(f, 0, sizeof *f);

  df = __get_sym_file(pathname); 
  if (df) {    
    /* XXX Should check access against mode / stat / possible
       deletion. */
    f->dfile = df;
    
    if ((flags & O_CREAT) && (flags & O_EXCL)) {
      errno = EEXIST;
      return -1;
    }
    
    if ((flags & O_TRUNC) && (flags & O_RDONLY)) {
      /* The result of using O_TRUNC with O_RDONLY is undefined, so we
	 return error */
      fprintf(stderr, "Undefined call to open(): O_TRUNC | O_RDONLY\n");
      errno = EACCES;
      return -1;
    }

    if ((flags & O_EXCL) && !(flags & O_CREAT)) {
      /* The result of using O_EXCL without O_CREAT is undefined, so
	 we return error */
      fprintf(stderr, "Undefined call to open(): O_EXCL w/o O_RDONLY\n");
      errno = EACCES;
      return -1;
    }

    if (!has_permission(flags, df->stat)) {
	errno = EACCES;
	return -1;
    }
    else
      f->dfile->stat->st_mode = ((f->dfile->stat->st_mode & ~0777) |
				 (mode & ~__exe_env.umask));
    f->type = SYMBOLIC_FILE;
  } else {
    f->type = CONCRETE_FILE;
    const char *con_pathname = __concretize_string(pathname);
    if (*con_pathname == '\0') {
        errno = ENOENT;
        return -1;
    }
#ifdef MEMFS_DEBUG
    printf("open(%s)\n", con_pathname);
#endif
    if (!strcmp(con_pathname, "/dev/tty")) {
        errno = EPERM;
        return -1;
    }
    // realpath() allocates the buffer
    char *con_fullpath = malloc(1024);
    int i;
    char *realpath_ret = realpath(con_pathname, con_fullpath); 
    if (!realpath_ret) {
        if (errno == ENOENT) {
#ifdef CWD_DEBUG
            printf("ignore realpath() = ENOENT: maybe new file\n");
#endif
        } else {
            free(con_fullpath);
#ifdef MEMFS_DEBUG
            printf("open(%s) failed: realpath() fail %d\n", con_pathname, errno);
#endif
            return -1;
        }
    }

    exe_disk_con_file_t *con_file = NULL;
    exe_disk_con_file_t *free_file = NULL;
    find_con_file(con_fullpath, &con_file, &free_file);
    if (con_file == NULL) {
        if (free_file == NULL) {
            con_file = enlarge_con_files();
        } else {
            con_file = free_file;
        }
        con_file->refcnt = 0;
#ifdef FAKE_CWD
        char *rpath = get_realpath(__concretize_string(pathname));
#else
        const char *rpath = __concretize_string(pathname);
#endif
#ifdef MEMFS_DEBUG
        printf("creating record for file: %s @ %p\n", rpath, con_file);
#endif

        if (flags & O_CREAT) {
            if (flags & O_EXCL) {
                int tmpfd = syscall(__NR_open, rpath, O_RDONLY, mode);
                syscall(__NR_close, tmpfd);
                if (tmpfd >= 0) {
#ifdef FAKE_CWD
                    free(rpath);
#endif
                    errno = EEXIST;
                    return -1;
                }
            }
            con_file->fd = -1;
            con_file->state = CON_STATE_INMEM;
            con_file->buf_size = 0;
            con_file->mode = mode | S_IFREG;
        } else {
            //con_file->fd = syscall(__NR_open, rpath, flags, mode);
            con_file->fd = klee_sys_open(rpath, flags, mode);
            if (con_file->fd == -1) {
                // shall we restore the con file array?
                // anyway it will be useful later...
                errno = klee_get_errno();
#ifdef FAKE_CWD
                free(rpath);
#endif
                return -1;
            }
            con_file->state = CON_STATE_DIRECT;
            con_file->mode = 0;
        }
#ifdef FAKE_CWD
        free(rpath);
#endif
        con_file->name = malloc(MAX_LINKS * sizeof(char*));
        con_file->name[0] = con_fullpath;
        for (i=1; i<MAX_LINKS; i++) con_file->name[i] = NULL;
        con_file->size = 0;
        con_file->contents = NULL;
        con_file->stat = NULL;

#ifdef SYM_SPECIFIC
        if (con_file->fd >= 0 && __exe_env.sym_specific) {
            if (strstr(con_fullpath, "CVS/") ||
                    strstr(con_fullpath, "CVSROOT/") ||
                    (__exe_env.etc_symbolic && (strstr(con_fullpath, "/etc/passwd") ||
                                                strstr(con_fullpath, "/etc/group") ||
                                                strstr(con_fullpath, "/etc/shadow") ||
                                                strstr(con_fullpath, "/etc/gshadow"))) ||
                    strstr(con_fullpath, ".git/config")) {
#ifdef MEMFS_DEBUG
                printf("symbolic file: %s\n", con_fullpath);
#endif
                off_t file_size;
                if (__exe_env.symbolic_size == 0)
                    file_size = syscall(__NR_lseek, con_file->fd, 0, SEEK_END);
                else
                    file_size = __exe_env.symbolic_size;
                if (file_size >= 0) {
#ifdef MEMFS_DEBUG
                    printf("matched. size: %ld\n", file_size);
#endif
                    klee_sys_close(con_file->fd);
                    con_file->state = CON_STATE_INMEM;
                    con_file->size = file_size;
                    con_file->buf_size = file_size;
                    con_file->contents = malloc(file_size);
                    klee_make_symbolic(con_file->contents, file_size, con_fullpath);
                }
            }
        }
#endif
    } else if (flags & O_CREAT) {
        errno = EEXIST;
        return -1;
    }

    f->fd = con_file->fd;
    f->dcfile = con_file;
    con_file->refcnt++;
  }
  
  f->flags = eOpen;
  if ((flags & O_ACCMODE) == O_RDONLY) {
    f->flags |= eReadable;
  } else if ((flags & O_ACCMODE) == O_WRONLY) {
    f->flags |= eWriteable;
  } else { /* XXX What actually happens here if != O_RDWR. */
    f->flags |= eReadable | eWriteable;
  }
  
#ifdef MEMFS_DEBUG
//    printf("open ret = %d\n", fd);
#endif
  errno = 0;
  return fd;
}

int close(int fd) {
  static int n_calls = 0;
  exe_file_t *f;
  int r = 0;
  
  n_calls++;  

  f = __get_file(fd);
  if (!f) {
    errno = EBADF;
    return -1;
  } 

  if (__exe_fs.max_failures && *__exe_fs.close_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }

#if 0
  if (!f->dfile) {
    /* if a concrete fd */
    r = syscall(__NR_close, f->fd);
  }
  else r = 0;
#endif
  if (f->type == CONCRETE_FILE) {
      f->dcfile->refcnt--;

#ifdef AGGRESIVE_CLOSE
      if (f->dcfile->refcnt == 0) {
          if (f->dcfile->state == CON_STATE_DIRECT || f->dcfile->state == CON_STATE_FORCE_DIRECT) {
              klee_sys_close(f->dcfile->fd);
              f->dcfile->state = CON_STATE_FREE;
              int i;
              for (i=0; i<MAX_LINKS; i++) {
                  if (f->dcfile->name[i]) {
                      free(f->dcfile->name[i]);
                      f->dcfile->name[i] = NULL;
                  }
              }
              free(f->dcfile->name);
              f->dcfile->name = NULL;
          }
      }
#endif
  }

  memset(f, 0, sizeof *f);
  
  return r;
}

ssize_t read(int fd, void *buf, size_t count) {
  static int n_calls = 0;
  exe_file_t *f;

  n_calls++;

  if (count == 0) 
    return 0;

  if (buf == NULL) {
    errno = EFAULT;
    return -1;
  }
  
  f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }  

  if (__exe_fs.max_failures && *__exe_fs.read_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }
  
  if (f->type == CONCRETE_FILE) {
#ifdef MEMFS_DEBUG
    printf("reading from CONCRETE file\n");
#endif
    if (f->dcfile->state == CON_STATE_STD) {
        printf("cannot read from CONCRETE STDIN!\n");
        errno = EIO;
        return -1;
    }
    /* concrete file */
    int r;
    buf = __concretize_ptr(buf);
    count = __concretize_size(count);
    /* XXX In terms of looking for bugs we really should do this check
       before concretization, at least once the routine has been fixed
       to properly work with symbolics. */
    klee_check_memory_access(buf, count);
    assert(f->dcfile);

#ifdef MEMFS_DEBUG
    printf("fd: %d dcfile: %p size: %d buf: %d\n", f->fd, f->dcfile, f->dcfile->size, f->dcfile->buf_size);
#endif

    if (f->dcfile->state == CON_STATE_DIRECT || f->dcfile->state == CON_STATE_FORCE_DIRECT) {
#ifdef MEMFS_DEBUG
        printf("performing DIRECT read\n");
#endif
        if (f->fd == 0)
            r = syscall(__NR_read, f->fd, buf, count);
        else
            r = syscall(__NR_pread64, f->fd, buf, count, (off64_t) f->off);
    } else {
#ifdef MEMFS_DEBUG
        printf("performing IN_MEM read\n");
#endif
        assert(f->dcfile->state == CON_STATE_INMEM);

        if (S_ISDIR(f->dcfile->mode)) {
            errno = EISDIR;
            return -1;
        }

        if (f->off < f->dcfile->size) {
            // something available
            if (f->off + count > f->dcfile->size) {
                count = f->dcfile->size - f->off;
            }
            memcpy(buf, f->dcfile->contents + f->off, count);
            r = count;
        } else {
            r = 0;
        }
    }

    if (r == -1) {
      errno = klee_get_errno();
      return -1;
    }
    
    if (f->fd != 0)
      f->off += r;
    return r;
  }
  else {
    assert(f->off >= 0);
    if (((off64_t)f->dfile->size) < f->off)
      return 0;

    /* symbolic file */
    if (f->off + count > f->dfile->size) {
      count = f->dfile->size - f->off;
    }
    
    memcpy(buf, f->dfile->contents + f->off, count);
    f->off += count;
    
    return count;
  }
}


ssize_t write(int fd, const void *buf, size_t count) {
  static int n_calls = 0;
  exe_file_t *f;

  n_calls++;

  f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (__exe_fs.max_failures && *__exe_fs.write_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }

  if (f->type == CONCRETE_FILE) {
    int r;

    buf = __concretize_ptr(buf);
    count = __concretize_size(count);
    /* XXX In terms of looking for bugs we really should do this check
       before concretization, at least once the routine has been fixed
       to properly work with symbolics. */
    klee_check_memory_access(buf, count);
    // maybe we should drop output to stdout & stderr?
    if (f->dcfile->state == CON_STATE_STD)
      r = syscall(__NR_write, f->fd, buf, count);
    else if (f->dcfile->state == CON_STATE_FORCE_DIRECT) {
      r = count;
    } else {
        if (f->dcfile->state == CON_STATE_DIRECT) {
            // was readonly: now we need to read all
            r = make_con_inmem(f->fd, f->dcfile);
            if (r) {
                return count;
            }
        }
        assert(f->dcfile->state == CON_STATE_INMEM);

        if (S_ISDIR(f->dcfile->mode)) {
            errno = EISDIR;
            return -1;
        }

        unsigned int last_pos = f->off + count;
        if (last_pos > f->dcfile->buf_size) {
            // buffer too small
            off_t new_buf_size = f->dcfile->buf_size == 0 ? 32 : 2 * f->dcfile->buf_size;
            while (new_buf_size < last_pos) new_buf_size *= 2;

            char *new_buf = malloc(new_buf_size);
            if (f->dcfile->contents) {
                memcpy(new_buf, f->dcfile->contents, f->dcfile->size);
                free(f->dcfile->contents);
            }
            // clear the new part
            memset(new_buf + f->dcfile->size, 0, new_buf_size - f->dcfile->size);
            f->dcfile->contents = new_buf;
            f->dcfile->buf_size = new_buf_size;
        }

        assert(last_pos <= f->dcfile->buf_size);

#ifdef MEMFS_DEBUG
        printf("off: %ld count: %ld size: %d bufsize: %d\n", f->off, count, f->dcfile->size, f->dcfile->buf_size);
#endif
        memcpy(f->dcfile->contents + f->off, buf, count);
        if (last_pos > f->dcfile->size)
            f->dcfile->size = last_pos;
        r = count;
//        r = syscall(__NR_pwrite64, f->fd, buf, count, (off64_t) f->off);
    }
    
    if (r == -1) {
      errno = klee_get_errno();
      return -1;
    }
    
    assert(r >= 0);
    if (f->fd != 1 && f->fd != 2)
      f->off += r;

    return r;
  }
  else {
    /* symbolic file */    
    size_t actual_count = 0;
    if (f->off + count <= f->dfile->size)
      actual_count = count;
    else {
      if (__exe_env.save_all_writes)
	assert(0);
      else {
	if (f->off < (off64_t) f->dfile->size)
	  actual_count = f->dfile->size - f->off;	
      }
    }
    
    if (actual_count)
      memcpy(f->dfile->contents + f->off, buf, actual_count);
    
    if (count != actual_count)
      fprintf(stderr, "WARNING: write() ignores bytes.\n");

    if (f->dfile == __exe_fs.sym_stdout)
      __exe_fs.stdout_writes += actual_count;

    f->off += count;
    return count;
  }
}


off64_t __fd_lseek(int fd, off64_t offset, int whence) {
  off64_t new_off;
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (f->type == CONCRETE_FILE) {
    /* We could always do SEEK_SET then whence, but this causes
       troubles with directories since we play nasty tricks with the
       offset, and the OS doesn't want us to randomly seek
       directories. We could detect if it is a directory and correct
       the offset, but really directories should only be SEEK_SET, so
       this solves the problem. */
    if (f->dcfile->state == CON_STATE_DIRECT || f->dcfile->state == CON_STATE_FORCE_DIRECT) {
        // in fact, no need to do real seek...
        if (whence == SEEK_SET) {
            new_off = syscall(__NR_lseek, f->fd, (int) offset, SEEK_SET);
        } else {
            new_off = syscall(__NR_lseek, f->fd, (int) f->off, SEEK_SET);

            /* If we can't seek to start off, just return same error.
               Probably ESPIPE. */
            if (new_off != -1) {
                assert(new_off == f->off);
                new_off = syscall(__NR_lseek, f->fd, (int) offset, whence);
            }
        }
    } else {
        switch(whence) {
            case SEEK_SET:
                new_off = offset;
                break;
            case SEEK_CUR:
                new_off = f->off + offset;
                break;
            case SEEK_END:
                new_off = f->dcfile->size + offset;
                break;
            default:
                errno = EINVAL;
                return (off_t) -1;
        }
    }

    if (new_off == -1) {
      errno = klee_get_errno();
      return -1;
    }

    f->off = new_off;
    return new_off;
  }
  
  switch (whence) {
  case SEEK_SET: new_off = offset; break;
  case SEEK_CUR: new_off = f->off + offset; break;
  case SEEK_END: new_off = f->dfile->size + offset; break;
  default: {
    errno = EINVAL;
    return (off64_t) -1;
  }
  }

  if (new_off < 0) {
    errno = EINVAL;
    return (off64_t) -1;
  }
    
  f->off = new_off;
  return f->off;
}

int __fd_stat(const char *path, struct stat64 *buf) {  
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    memcpy(buf, dfile->stat, sizeof(*dfile->stat));
    return 0;
  } 

  {
    memset(buf, 0, sizeof(struct stat64));
#ifdef FAKE_CWD
    char *rpath = get_realpath(__concretize_string(path));
#else
    const char *rpath = __concretize_string(path);
#endif
#ifdef STAT_DEBUG
    printf("stat(%s)\n", rpath);
#endif
#if __WORDSIZE == 64
    int r = syscall(__NR_stat, rpath, buf);
#else
    int r = syscall(__NR_stat64, rpath, buf);
#endif
    if (r == -1)
      errno = klee_get_errno();

    int lasterr = errno;

    char con_fullpath[1024];
    if (!realpath(rpath, con_fullpath)) {
        if (errno != ENOENT) {
#ifdef FAKE_CWD
            free(rpath);
#endif
            errno = lasterr;
            return r;
        }
    }

    int i;
    exe_disk_con_file_t *con_file = NULL;
    find_con_file(con_fullpath, &con_file, NULL);
    // fake links...
    if (con_file) {
        i=0;
        int j;
        for (j=0; j<MAX_LINKS; j++) {
            if (con_file->name[j]) i++;
        }
        if (r >= 0) {
            if (i > 1)
                buf->st_nlink += (i-1);
            if (con_file->mode) {
                buf->st_mode = con_file->mode;
            }
        } else {
            // fake file
            r = 0;
            buf->st_nlink = i;
            buf->st_size = con_file->size;
            buf->st_blocks = (con_file->buf_size - 1)/ 512 + 1;
            buf->st_blksize = 512;
            buf->st_uid = getuid();
            buf->st_gid = getgid();
            buf->st_mode = con_file->mode;
        }
    }

#ifdef FAKE_CWD
    free(rpath);
#endif
    errno = lasterr;
    return r;
  }
}

int __fd_lstat(const char *path, struct stat64 *buf) {
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    memcpy(buf, dfile->stat, sizeof(*dfile->stat));
    return 0;
  } 

  {    
    memset(buf, 0, sizeof(struct stat64));
#ifdef FAKE_CWD
    char *rpath = get_realpath(__concretize_string(path));
#else
    const char *rpath = __concretize_string(path);
#endif
#ifdef STAT_DEBUG
    printf("lstat(%s)\n", rpath);
#endif
#if __WORDSIZE == 64
    int r = syscall(__NR_lstat, rpath, buf);
#else
    int r = syscall(__NR_lstat64, rpath, buf);
#endif
    if (r == -1)
      errno = klee_get_errno();

    exe_disk_con_file_t *con_file = NULL;
    find_con_file(rpath, &con_file, NULL);
    // fake links...
    if (con_file) {
#ifdef STAT_DEBUG
        printf("found record for %s\n", rpath);
#endif
        int i=0;
        int j;
        for (j=0; j<MAX_LINKS; j++) {
            if (con_file->name[j]) i++;
        }
        if (r >= 0) {
            if (i > 1)
                buf->st_nlink += (i-1);
            if (con_file->mode) {
                buf->st_mode = con_file->mode;
            }
        } else {
            // real fake file
            r = 0;
            buf->st_nlink = i;
            buf->st_size = con_file->size;
            buf->st_blocks = (con_file->buf_size - 1) / 512 + 1;
            buf->st_blksize = 512;
            buf->st_uid = getuid();
            buf->st_gid = getgid();
            buf->st_mode = con_file->mode;
        }
    }

#ifdef FAKE_CWD
    free(rpath);
#endif
#ifdef STAT_DEBUG
    printf("lstat() ret: %d\n", r);
#endif
    return r;
  }
}

int fstatat(int dirfd, const char *path, struct stat *buf, int flags) {
  exe_file_t *f = __get_file(dirfd);

  if (!f) {
      errno = EBADF;
      return -1;
  }

  if (f->type == SYMBOLIC_FILE) {
      // no symbolic dir!
      errno = ENOTDIR;
      return -1;
  }

  assert(f->type == CONCRETE_FILE);
  if (path[0] == '/') {
      // absolute path!
      // XXX: I should not ignore 'flags'
      return stat(path, buf);
  } else {
      // relative path!
      struct stat st;
      int ret = fstat(dirfd, &st);
      if (ret != 0) {
          return ret;
      }

      if (!S_ISDIR(st.st_mode)) {
          errno = ENOTDIR;
          return -1;
      }

      int i;
      for (i=0; i<MAX_LINKS; i++) {
          if (f->dcfile->name[i]) {
              char *fullpath = malloc(strlen(f->dcfile->name[i]) + 1 + strlen(path) + 1);
              strcpy(fullpath, f->dcfile->name[i]);
              strcat(fullpath, "/");
              strcat(fullpath, path);
              ret = stat(fullpath, buf);
              free(fullpath);
              return ret;
          }
      }

      // what the hell... no name?
      errno = EBADF;
      return -1;
  }
}

int chdir(const char *path) {
  exe_disk_file_t *dfile = __get_sym_file(path);

  if (dfile) {
    /* XXX incorrect */
    klee_warning("symbolic file, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }

  {
#ifdef FAKE_CWD
    char *old_current = current_path;
    current_path = get_realpath(__concretize_string(path));
    current_path_len = strlen(current_path);
    if (old_current)
        free(old_current);
    return 0;
#else
    int r = syscall(__NR_chdir, __concretize_string(path));
    if (r == -1)
      errno = klee_get_errno();
    return r;
#endif
  }
}

int fchdir(int fd) {
  exe_file_t *f = __get_file(fd);
  
  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (f->type == SYMBOLIC_FILE) {
    klee_warning("symbolic file, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  } else
#ifdef FAKE_CWD
  {
    assert(f->dcfile);
    assert(f->type == CONCRETE_FILE);
    char *old_current = current_path;
    int i;
    current_path = NULL;
#ifdef MEMFS_DEBUG
    printf("trying to fchdir() %d @ %p\n", fd, f->dcfile);
#endif
    for (i=0; i<MAX_LINKS; i++) {
        if (f->dcfile->name[i])
        {
            current_path = strdup(f->dcfile->name[i]);
            break;
        }
    }
    if (!current_path) {
        printf("fchdir(): error! fd has no name!\n");
        current_path = old_current;
        return -1;
    }
    current_path_len = strlen(current_path);
    if (old_current)
        free(old_current);
    return 0;
  }
#else
  {
    int r = syscall(__NR_fchdir, f->fd);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
#endif
}

/* Sets mode and or errno and return appropriate result. */
static int __df_chmod(exe_disk_file_t *df, mode_t mode) {
  if (geteuid() == df->stat->st_uid) {
    if (getgid() != df->stat->st_gid)
      mode &= ~ S_ISGID;
    df->stat->st_mode = ((df->stat->st_mode & ~07777) | 
                         (mode & 07777));
    return 0;
  } else {
    errno = EPERM;
    return -1;
  }
}

int chmod(const char *path, mode_t mode) {
  static int n_calls = 0;

  exe_disk_file_t *dfile = __get_sym_file(path);

  n_calls++;
  if (__exe_fs.max_failures && *__exe_fs.chmod_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }

  if (dfile) {
    return __df_chmod(dfile, mode);
  } else {
#ifdef FAKE_CWD
    char rpath[1024];
    char *cret = realpath(__concretize_string(path), rpath);
    if (!cret && errno != ENOENT) {
        return -1;
    }
    exe_disk_con_file_t *con_file;
    find_con_file(rpath, &con_file, NULL);
    if (con_file) {
        struct stat st;
        stat(rpath, &st);
        con_file->mode = (st.st_mode & ~0777) | mode;
        errno = 0;
        return 0;
    }
    int r = syscall(__NR_chmod, rpath, mode);
#else
    int r = syscall(__NR_chmod, __concretize_string(path), mode);
#endif
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
}

int fchmod(int fd, mode_t mode) {
  static int n_calls = 0;

  exe_file_t *f = __get_file(fd);
  
  if (!f) {
    errno = EBADF;
    return -1;
  }

  n_calls++;
  if (__exe_fs.max_failures && *__exe_fs.fchmod_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }

  if (f->type == SYMBOLIC_FILE) {
    return __df_chmod(f->dfile, mode);
  } else {
    if (f->fd < 0) {
      f->dcfile->mode = (f->dcfile->mode & ~0777) | mode;
      errno = 0;
      return 0;
    }
    int r = syscall(__NR_fchmod, f->fd, mode);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }  
}

int fchmodat(int dirfd, const char *path, mode_t mode, int flags) {
    printf("fchmodat(): ignore. fd: %d", dirfd);
    return 0;
}

static int __df_chown(exe_disk_file_t *df, uid_t owner, gid_t group) {
  klee_warning("symbolic file, ignoring (EPERM)");
  errno = EPERM;
  return -1;  
}

int chown(const char *path, uid_t owner, gid_t group) {
  exe_disk_file_t *df = __get_sym_file(path);

  if (df) {
    return __df_chown(df, owner, group);
  } else {
#ifdef FAKE_CWD
    char *rpath = get_realpath(__concretize_string(path));
    int r = syscall(__NR_chown, rpath, owner, group);
#else
    int r = syscall(__NR_chown, __concretize_string(path), owner, group);
#endif
    if (r == -1)
      errno = klee_get_errno();
#ifdef FAKE_CWD
    free(rpath);
#endif
    return r;
  }
}

int fchown(int fd, uid_t owner, gid_t group) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (f->type == SYMBOLIC_FILE) {
    return __df_chown(f->dfile, owner, group);
  } else {
    int r = syscall(__NR_fchown, fd, owner, group);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
}

int lchown(const char *path, uid_t owner, gid_t group) {
  /* XXX Ignores 'l' part */
  exe_disk_file_t *df = __get_sym_file(path);

  if (df) {
    return __df_chown(df, owner, group);
  } else {
#ifdef FAKE_CWD
    char *rpath = get_realpath(__concretize_string(path));
    int r = syscall(__NR_chown, rpath, owner, group);
#else
    int r = syscall(__NR_chown, __concretize_string(path), owner, group);
#endif
    if (r == -1)
      errno = klee_get_errno();
#ifdef FAKE_CWD
    free(rpath);
#endif
    return r;
  }
}

int __fd_fstat(int fd, struct stat64 *buf) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }
  
  if (f->type == CONCRETE_FILE) {
#if __WORDSIZE == 64
    int r = syscall(__NR_fstat, f->fd, buf);
#else
    int r = syscall(__NR_fstat64, f->fd, buf);
#endif
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
  
  memcpy(buf, f->dfile->stat, sizeof(*f->dfile->stat));
  return 0;
}

int __fd_ftruncate(int fd, off64_t length) {
  static int n_calls = 0;
  exe_file_t *f = __get_file(fd);

  n_calls++;

  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (__exe_fs.max_failures && *__exe_fs.ftruncate_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }
  
  if (f->type == SYMBOLIC_FILE) {
    klee_warning("symbolic file, ignoring (EIO)");
    errno = EIO;
    return -1;
  } else {
#ifdef FAKE_FS
    assert(f->dcfile);
    if (length > f->dcfile->size) {
        // increase file length
        if (f->dcfile->buf_size < length) {
            // buf too small
            char *old_contents = f->dcfile->contents;
            unsigned old_bufsize = f->dcfile->buf_size;
            if (f->dcfile->buf_size == 0) {
                f->dcfile->buf_size = 32;
            }
            while (f->dcfile->buf_size < length) f->dcfile->buf_size *= 2;
            f->dcfile->contents = malloc(f->dcfile->buf_size);
            if (old_bufsize)
                memcpy(f->dcfile->contents, old_contents, old_bufsize);
            memset(f->dcfile->contents + old_bufsize, 0, f->dcfile->buf_size - old_bufsize);

            free(old_contents);
        }
    } // else: just decrease the file size

    f->dcfile->size = length;
    return 0;
#else // FAKE_FS
#if __WORDSIZE == 64
    int r = syscall(__NR_ftruncate, f->fd, length);
#else
    int r = syscall(__NR_ftruncate64, f->fd, length);
#endif
    if (r == -1)
      errno = klee_get_errno();
    return r;
#endif // FAKE_FS
  }  
}

int __fd_getdents(unsigned int fd, struct dirent64 *dirp, unsigned int count) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }
  
  if (f->type == SYMBOLIC_FILE) {
    klee_warning("symbolic file, ignoring (EINVAL)");
    errno = EINVAL;
    return -1;
  } else {
    if ((unsigned long) f->off < 4096u) {
      /* Return our dirents */
      unsigned i, pad, bytes=0;

      /* What happens for bad offsets? */
      i = f->off / sizeof(*dirp);
      if (((off64_t) (i * sizeof(*dirp)) != f->off) ||
          i > __exe_fs.n_sym_files) {
        errno = EINVAL;
        return -1;
      } 
      for (; i<__exe_fs.n_sym_files; ++i) {
        exe_disk_file_t *df = &__exe_fs.sym_files[i];
        dirp->d_ino = df->stat->st_ino;
        dirp->d_reclen = sizeof(*dirp);
        dirp->d_type = IFTODT(df->stat->st_mode);
        dirp->d_name[0] = 'A' + i;
        dirp->d_name[1] = '\0';
        dirp->d_off = (i+1) * sizeof(*dirp);
        bytes += dirp->d_reclen;
        ++dirp;
      }
      
      /* Fake jump to OS records by a "deleted" file. */
      pad = count>=4096 ? 4096 : count;
      dirp->d_ino = 0;
      dirp->d_reclen = pad - bytes;
      dirp->d_type = DT_UNKNOWN;
      dirp->d_name[0] = '\0';
      dirp->d_off = 4096;
      bytes += dirp->d_reclen;
      f->off = pad;
      return bytes;
    } else {
      unsigned os_pos = f->off - 4096;
      int res, s;

      /* For reasons which I really don't understand, if I don't
         memset this then sometimes the kernel returns d_ino==0 for
         some valid entries? Am I crazy? Can writeback possibly be
         failing? 
      
         Even more bizarre, interchanging the memset and the seek also
         case strange behavior. Really should be debugged properly. */
      memset(dirp, 0, count);
      s = syscall(__NR_lseek, f->fd, (int) os_pos, SEEK_SET);
      assert(s != (off64_t) -1);
      res = syscall(__NR_getdents64, f->fd, dirp, count);
      if (res == -1) {
        errno = klee_get_errno();
      } else {
        int pos = 0;

        f->off = syscall(__NR_lseek, f->fd, 0, SEEK_CUR) + 4096;

        /* Patch offsets */
        
        while (pos < res) {
          struct dirent64 *dp = (struct dirent64*) ((char*) dirp + pos);
          dp->d_off += 4096;
          pos += dp->d_reclen;
        }
      }
      return res;
    }
  }
}

#if __WORDSIZE == 64
int ioctl(int fd, unsigned long int request, ...) {
#else
int ioctl(int fd, unsigned long request, ...) {
#endif
  exe_file_t *f = __get_file(fd);
  va_list ap;
  void *buf;

#if 0
  printf("In ioctl(%d, ...)\n", fd);
#endif

  if (!f) {
    errno = EBADF;
    return -1;
  }
  
  va_start(ap, request);
  buf = va_arg(ap, void*);
  va_end(ap);
  
  if (f->type == SYMBOLIC_FILE) {
    struct stat *stat = (struct stat*) f->dfile->stat;

    switch (request) {
    case TCGETS: {      
      struct termios *ts = buf;

      klee_warning_once("(TCGETS) symbolic file, incomplete model");

      /* XXX need more data, this is ok but still not good enough */
      if (S_ISCHR(stat->st_mode)) {
        /* Just copied from my system, munged to match what fields
           uclibc thinks are there. */
        ts->c_iflag = 27906;
        ts->c_oflag = 5;
        ts->c_cflag = 1215;
        ts->c_lflag = 35287;
        ts->c_line = 0;
        ts->c_cc[0] = '\x03';
        ts->c_cc[1] = '\x1c';
        ts->c_cc[2] = '\x7f';
        ts->c_cc[3] = '\x15';
        ts->c_cc[4] = '\x04';
        ts->c_cc[5] = '\x00';
        ts->c_cc[6] = '\x01';
        ts->c_cc[7] = '\xff';
        ts->c_cc[8] = '\x11';
        ts->c_cc[9] = '\x13';
        ts->c_cc[10] = '\x1a';
        ts->c_cc[11] = '\xff';
        ts->c_cc[12] = '\x12';
        ts->c_cc[13] = '\x0f';
        ts->c_cc[14] = '\x17';
        ts->c_cc[15] = '\x16';
        ts->c_cc[16] = '\xff';
        ts->c_cc[17] = '\x0';
        ts->c_cc[18] = '\x0';
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case TCSETS: {
      /* const struct termios *ts = buf; */
      klee_warning_once("(TCSETS) symbolic file, silently ignoring");
      if (S_ISCHR(stat->st_mode)) {
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case TCSETSW: {
      /* const struct termios *ts = buf; */
      klee_warning_once("(TCSETSW) symbolic file, silently ignoring");
      if (fd==0) {
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case TCSETSF: {
      /* const struct termios *ts = buf; */
      klee_warning_once("(TCSETSF) symbolic file, silently ignoring");
      if (S_ISCHR(stat->st_mode)) {        
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case TIOCGWINSZ: {
      struct winsize *ws = buf;
      ws->ws_row = 24;
      ws->ws_col = 80;
      klee_warning_once("(TIOCGWINSZ) symbolic file, incomplete model");
      if (S_ISCHR(stat->st_mode)) {
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case TIOCSWINSZ: {
      /* const struct winsize *ws = buf; */
      klee_warning_once("(TIOCSWINSZ) symbolic file, ignoring (EINVAL)");
      if (S_ISCHR(stat->st_mode)) {
        errno = EINVAL;
        return -1;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case FIONREAD: {
      int *res = buf;
      klee_warning_once("(FIONREAD) symbolic file, incomplete model");
      if (S_ISCHR(stat->st_mode)) {
        if (f->off < (off64_t) f->dfile->size) {
          *res = f->dfile->size - f->off;
        } else {
          *res = 0;
        }
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case MTIOCGET: {
      klee_warning("(MTIOCGET) symbolic file, ignoring (EINVAL)");
      errno = EINVAL;
      return -1;
    }
    default:
      klee_warning("symbolic file, ignoring (EINVAL)");
      errno = EINVAL;
      return -1;
    }
  } else {
    int r = syscall(__NR_ioctl, f->fd, request, buf );
    if (r == -1) 
      errno = klee_get_errno();
    return r;
  }
}

int fcntl(int fd, int cmd, ...) {
  exe_file_t *f = __get_file(fd);
  va_list ap;
  unsigned arg; /* 32 bit assumption (int/ptr) */

  if (!f) {
    errno = EBADF;
    return -1;
  }
  
  if (cmd==F_GETFD || cmd==F_GETFL || cmd==F_GETOWN || cmd==F_GETSIG ||
      cmd==F_GETLEASE || cmd==F_NOTIFY) {
    arg = 0;
  } else {
    va_start(ap, cmd);
    arg = va_arg(ap, int);
    va_end(ap);
  }

  if (f->type == SYMBOLIC_FILE) {
    switch(cmd) {
    case F_GETFD: {
      int flags = 0;
      if (f->flags & eCloseOnExec)
        flags |= FD_CLOEXEC;
      return flags;
    } 
    case F_SETFD: {
      f->flags &= ~eCloseOnExec;
      if (arg & FD_CLOEXEC)
        f->flags |= eCloseOnExec;
      return 0;
    }
    case F_SETFL:
    case F_GETFL: {
      /* XXX (CrC): This should return the status flags: O_APPEND,
	 O_ASYNC, O_DIRECT, O_NOATIME, O_NONBLOCK.  As of now, we
	 discard these flags during open().  We should save them and
	 return them here.  These same flags can be set by F_SETFL,
	 which we could also handle properly. 
      */
      return 0;
    }
    default:
      klee_warning("symbolic file, ignoring (EINVAL)");
      errno = EINVAL;
      return -1;
    }
  } else {
    int r = syscall(__NR_fcntl, f->fd, cmd, arg );
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
}

int __fd_statfs(const char *path, struct statfs *buf) {
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    /* XXX incorrect */
    klee_warning("symbolic file, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }

  {
#ifdef FAKE_CWD
    char *rpath = get_realpath(__concretize_string(path));
    int r = syscall(__NR_statfs, rpath, buf);
#else
    int r = syscall(__NR_statfs, __concretize_string(path), buf);
#endif
    if (r == -1)
      errno = klee_get_errno();
#ifdef FAKE_CWD
    free(rpath);
#endif
    return r;
  }
}

int fstatfs(int fd, struct statfs *buf) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }
  
  if (f->type == SYMBOLIC_FILE) {
    klee_warning("symbolic file, ignoring (EBADF)");
    errno = EBADF;
    return -1;
  } else {
    int r = syscall(__NR_fstatfs, f->fd, buf);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
}

int fsync(int fd) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  } else if (f->type == CONCRETE_FILE) {
    return 0;
  } else {
      // anyway the file is either not modificated
      // or modified in the memory
    return 0;
#if 0
    int r = syscall(__NR_fsync, f->fd);
    if (r == -1)
      errno = klee_get_errno();
    return r;
#endif
  }
}

int dup2(int oldfd, int newfd) {
  exe_file_t *f = __get_file(oldfd);

  if (!f || !(newfd>=0 && newfd<MAX_FDS)) {
    errno = EBADF;
    return -1;
  } else {
    exe_file_t *f2 = &__exe_env.fds[newfd];
    if (f2->flags & eOpen) close(newfd);

    /* XXX Incorrect, really we need another data structure for open
       files */
    *f2 = *f;
    if (f->type == CONCRETE_FILE) {
        f->dcfile->refcnt++;
    }

    f2->flags &= ~eCloseOnExec;
      
    /* I'm not sure it is wise, but we can get away with not dup'ng
       the OS fd, since actually that will in many cases effect the
       sharing of the open file (and the process should never have
       access to it). */

    return newfd;
  }
}

int dup(int oldfd) {
  exe_file_t *f = __get_file(oldfd);
  if (!f) {
    errno = EBADF;
    return -1;
  } else {
    int fd;
    for (fd = 0; fd < MAX_FDS; ++fd)
      if (!(__exe_env.fds[fd].flags & eOpen))
        break;
    if (fd == MAX_FDS) {
      errno = EMFILE;
      return -1;
    } else {
      return dup2(oldfd, fd);
    }
  }
}

int rmdir(const char *pathname) {
  exe_disk_file_t *dfile = __get_sym_file(pathname);
  if (dfile) {
    /* XXX check access */ 
    if (S_ISDIR(dfile->stat->st_mode)) {
      dfile->stat->st_ino = 0;
      return 0;
    } else {
      errno = ENOTDIR;
      return -1;
    }
  }

  char rpath[1024];
  char *cret = realpath(pathname, rpath);
  if (!cret && errno != ENOENT) {
      printf("rmdir(): realpath err %d\n", errno);
      return -1;
  }

  exe_disk_con_file_t *con_file = NULL;
  find_con_file(rpath, &con_file, NULL);
  if (con_file) {
      free_con_file(con_file);
      return 0;
  }

  klee_warning("ignoring, fake success");
  return 0; // may still be used by other states
#if 0
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
#endif
}

int mkdir(const char* pathname, mode_t mode) {
  const char *path = __concretize_string(pathname);
  int ret;
#ifdef FAKE_CWD
  char rpath[1024];
  char *cret = realpath(path, rpath);
  if (!cret) {
#ifdef MKDIR_DEBUG
      printf("realpath() err: %d\n", errno);
#endif
      if (errno == ENOENT) {
          // ignore it
      } else {
#ifdef MKDIR_DEBUG
          printf("mkdir() error: %d\n", errno);
#endif
          return -1;
      }
  }

#ifdef MKDIR_DEBUG
  printf("mkdir %s -> %s %s\n", path, rpath, cret);
#endif

  struct stat sb;
  ret = lstat(rpath, &sb);
  if (ret == 0) {
      // is a file or is a dir
#ifdef MKDIR_DEBUG
      printf("mkdir() error: already exists\n");
#endif
      errno = EEXIST;
      return -1;
  }

//  int ret = syscall(__NR_mkdir, rpath, mode);
  exe_disk_con_file_t *con_file = NULL;
  exe_disk_con_file_t *free_file = NULL;
  find_con_file(rpath, &con_file, &free_file);
  if (con_file == NULL) {
      if (free_file == NULL) {
          con_file = enlarge_con_files();
      } else {
          con_file = free_file;
      }

#ifdef MEMFS_DEBUG
      printf("creating record for directory: %s @ %p\n", rpath, con_file);
#endif

      con_file->fd = -1;
      con_file->state = CON_STATE_INMEM;
      con_file->mode = mode | S_IFDIR;

      con_file->name = malloc(MAX_LINKS * sizeof(char*));
      con_file->name[0] = strdup(rpath);
      int i;
      for (i=1; i<MAX_LINKS; i++) con_file->name[i] = NULL;
      con_file->size = 512;
      con_file->contents = NULL;
      con_file->buf_size = 0;
      con_file->stat = NULL;
  } else {
      // directory/file exists!
      // should be detected by lstat()
  }
  errno = 0;
  return 0;
#else
  ret = syscall(__NR_mkdir, path, mode);
#endif
  if (ret) {
      errno = klee_get_errno();
      if (errno == EEXIST) {
          // may be created in other states
          errno = 0;
          ret = 0;
      }
  }
  return ret;
}
  

int unlink(const char *pathname) {
  exe_disk_file_t *dfile = __get_sym_file(pathname);
  if (dfile) {
    /* XXX check access */ 
    if (S_ISREG(dfile->stat->st_mode)) {
      dfile->stat->st_ino = 0;
      return 0;
    } else if (S_ISDIR(dfile->stat->st_mode)) {
      errno = EISDIR;
      return -1;
    } else {
      errno = EPERM;
      return -1;
    }
  }

  char *path = __concretize_string(pathname);
  char rpath[1024];
  char *cret = realpath(path, rpath);
  if (!cret && errno != ENOENT) {
      printf("unlink() fail: realpath err %d\n", errno);
      return -1;
  }

  exe_disk_con_file_t *con_file;
  find_con_file(rpath, &con_file, NULL);
  if (con_file) {
      free_con_file(con_file);
      errno = 0;
      return 0;
  }

  klee_warning("ignoring (EPERM)");
  errno = 0;
  return -1;
}

ssize_t readlink(const char *path, char *buf, size_t bufsize) {
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    /* XXX We need to get the sym file name really, but since we don't
       handle paths anyway... */
    if (S_ISLNK(dfile->stat->st_mode)) {
      buf[0] = path[0];
      if (bufsize>1) buf[1] = '.';
      if (bufsize>2) buf[2] = 'l';
      if (bufsize>3) buf[3] = 'n';
      if (bufsize>4) buf[4] = 'k';
      return (bufsize>5) ? 5 : bufsize;
    } else {
      errno = EINVAL;
      return -1;
    }
  } else {
#ifdef FAKE_CWD
    char *rpath = get_realpath(path);
#else
    char *rpath = path;
#endif

    exe_disk_con_file_t *con_file;
    find_con_file(rpath, &con_file, NULL);
    if (con_file) {
        // record
        if (S_ISDIR(con_file->mode) || S_ISREG(con_file->mode)) {
            errno = EINVAL;
            return -1;
        }
    }
    int r = syscall(__NR_readlink, rpath, buf, bufsize);
    if (r == -1)
      errno = klee_get_errno();
#ifdef FAKE_CWD
    free(rpath);
#endif
    return r;
  }
}

#undef FD_SET
#undef FD_CLR
#undef FD_ISSET
#undef FD_ZERO
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	memset((char *)(p), '\0', sizeof(*(p)))
int select(int nfds, fd_set *read, fd_set *write,
           fd_set *except, struct timeval *timeout) {
  fd_set in_read, in_write, in_except, os_read, os_write, os_except;
  int i, count = 0, os_nfds = 0;

  if (read) {
    in_read = *read;
    FD_ZERO(read);
  } else {
    FD_ZERO(&in_read);
  }

  if (write) {
    in_write = *write;
    FD_ZERO(write);
  } else {
    FD_ZERO(&in_write);
  }
   
  if (except) {
    in_except = *except;
    FD_ZERO(except);
  } else {
    FD_ZERO(&in_except);
  }

  FD_ZERO(&os_read);
  FD_ZERO(&os_write);
  FD_ZERO(&os_except);

  /* Check for symbolic stuff */
  for (i=0; i<nfds; i++) {    
    if (FD_ISSET(i, &in_read) || FD_ISSET(i, &in_write) || FD_ISSET(i, &in_except)) {
      exe_file_t *f = __get_file(i);
      if (!f) {
        errno = EBADF;
        return -1;
      } else if (f->type == SYMBOLIC_FILE) {
        /* Operations on this fd will never block... */
        if (FD_ISSET(i, &in_read)) FD_SET(i, read);
        if (FD_ISSET(i, &in_write)) FD_SET(i, write);
        if (FD_ISSET(i, &in_except)) FD_SET(i, except);
        ++count;
      } else {
        if (FD_ISSET(i, &in_read)) FD_SET(f->fd, &os_read);
        if (FD_ISSET(i, &in_write)) FD_SET(f->fd, &os_write);
        if (FD_ISSET(i, &in_except)) FD_SET(f->fd, &os_except);
        if (f->fd >= os_nfds) os_nfds = f->fd + 1;
      }
    }
  }

  if (os_nfds > 0) {
    /* Never allow blocking select. This is broken but what else can
       we do. */
    struct timeval tv = { 0, 0 };    
    int r = syscall(__NR_select, os_nfds, 
                    &os_read, &os_write, &os_except, &tv);
    
    if (r == -1) {
      /* If no symbolic results, return error. Otherwise we will
         silently ignore the OS error. */
      if (!count) {
        errno = klee_get_errno();
        return -1;
      }
    } else {
      count += r;

      /* Translate resulting sets back */
      for (i=0; i<nfds; i++) {
        exe_file_t *f = __get_file(i);
        if (f && !f->dfile) {
          if (read && FD_ISSET(f->fd, &os_read)) FD_SET(i, read);
          if (write && FD_ISSET(f->fd, &os_write)) FD_SET(i, write);
          if (except && FD_ISSET(f->fd, &os_except)) FD_SET(i, except);
        }
      }
    }
  }

  return count;
}

/*** Library functions ***/

char *get_current_path(char *buf, size_t size) {
#ifdef FAKE_CWD
  if (!current_path)
  {
      current_path = malloc(PATH_MAX);
      int r = syscall(__NR_getcwd, current_path, PATH_MAX);
      if (r == -1) {
          errno = klee_get_errno();
          // what the hell!
          // can't get current directory!
          free(current_path);
          current_path = NULL;
          return NULL;
      }
      current_path_len = strlen(current_path);
  }

  // if size is 0: just update current path
  if (!size) return NULL;

  if (strlen(current_path) >= size) {
      errno = ERANGE;
      return NULL;
  }

  strncpy(buf, current_path, size);
  return buf;
#else
  int r = syscall(__NR_getcwd, buf, size);
  if (r == -1) {
    errno = klee_get_errno();
    return NULL;
  }
    
  return buf;
#endif
}


char *getcwd(char *buf, size_t size) {
  static int n_calls = 0;

  n_calls++;

  if (__exe_fs.max_failures && *__exe_fs.getcwd_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = ERANGE;
    return NULL;
  }

  if (!buf) {
    if (!size)
      size = 1024;
    buf = malloc(size);
  }
  
  buf = __concretize_ptr(buf);
  size = __concretize_size(size);
  /* XXX In terms of looking for bugs we really should do this check
     before concretization, at least once the routine has been fixed
     to properly work with symbolics. */
  klee_check_memory_access(buf, size);
  char *ret = get_current_path(buf, size);
#ifdef CWD_DEBUG
  printf("getcwd() = %s\n", ret);
#endif
  return ret;
}

/*** Helper functions ***/

#ifdef FAKE_CWD
char *get_realpath(const char *rel_path) {
    if (rel_path[0] == '/') {
        // absolute path
        return strdup(rel_path);
    }

    if (!current_path)
        get_current_path(NULL, 0);

    int rel_path_len = strlen(rel_path);
    char *buf = malloc(current_path_len + 1 + rel_path_len + 1);
    if (buf == NULL) {
        return NULL;
    }
#ifdef MEMFS_DEBUG
    printf("buf len: %d\n", current_path_len + rel_path_len + 2);
    printf("current_path: %s\n", current_path);
#endif
    strcpy(buf, current_path);
    strcat(buf, "/");
    strcat(buf, rel_path);
#ifdef CWD_DEBUG
    printf("rel path: %s -> abs path: %s\n", rel_path, buf);
#endif
//    printf("getting realpath\n");
//    klee_stack_trace();
//    char *ret = realpath(rel_path, buf);
//    printf("got realpath\n");
    return buf;
}
#endif

static void *__concretize_ptr(const void *p) {
  /* XXX 32-bit assumption */
  char *pc = (char*) klee_get_valuel((long) p);
  klee_assume(pc == p);
  return pc;
}

static size_t __concretize_size(size_t s) {
  size_t sc = klee_get_valuel((long)s);
  klee_assume(sc == s);
  return sc;
}

static const char *__concretize_string(const char *s) {
  char *sc = __concretize_ptr(s);
  unsigned i;

  for (i=0; ; ++i) {
    char c = *sc;
    if (!(i&(i-1))) {
      if (!c) {
        *sc++ = 0;
        break;
      } else if (c=='/') {
        *sc++ = '/';
      } 
    } else {
      char cc = (char) klee_get_valuel((long)c);
      klee_assume(cc == c);
      *sc++ = cc;
      if (!cc) break;
    }
  }

  return s;
}



/* Trivial model:
   if path is "/" (basically no change) accept, otherwise reject
*/
int chroot(const char *path) {
  if (path[0] == '\0') {
    errno = ENOENT;
    return -1;
  }
    
  if (path[0] == '/' && path[1] == '\0') {
    return 0;
  }
  
  klee_warning("ignoring (ENOENT)");
  errno = ENOENT;
  return -1;
}

int link(const char *oldpath, const char *newpath) {
    const char *con_oldpath = __concretize_string(oldpath);
    const char *con_newpath = __concretize_string(newpath);

    int r = access(newpath, F_OK);
//    int r = syscall(__NR_access, con_newpath, F_OK);
    if (r == 0) { // file exists!
        errno = EEXIST;
#ifdef MEMFS_DEBUG
        printf("link(): dest exists\n");
#endif
        return -1;
    }

    r = access(oldpath, F_OK);
//    r = syscall(__NR_access, con_oldpath, F_OK);
    if (r != 0) { // old file missing!
        errno = ENOENT; // XXX: right?
#ifdef MEMFS_DEBUG
        printf("link(): source does not exist\n");
#endif
        return -1;
    }

    if (__get_sym_file(con_oldpath) ||
            __get_sym_file(con_newpath)) {
        // symbolic file: don't allow
        errno = EPERM;
        return -1;
    }

    // realpath() allocates the buffer
    char *con_full_oldpath = malloc(1024);
    int i;
    char *realpath_ret = realpath(con_oldpath, con_full_oldpath); 
    if (!realpath_ret) {
        if (errno == ENOENT) {
#ifdef CWD_DEBUG
            printf("ignore realpath() = ENOENT: maybe new file\n");
#endif
        } else {
            free(con_full_oldpath);
#ifdef MEMFS_DEBUG
            printf("link(%s, %s) failed: realpath() fail %d\n", con_oldpath, con_newpath, errno);
#endif
            return -1;
        }
    }

    char *con_full_newpath = malloc(1024);
    realpath_ret = realpath(con_newpath, con_full_newpath);
    if (!realpath_ret) {
        if (errno == ENOENT) {
#ifdef CWD_DEBUG
            printf("ignore realpath() = ENOENT: maybe new file\n");
#endif
        } else {
            free(con_full_oldpath);
            free(con_full_newpath);
#ifdef MEMFS_DEBUG
            printf("link(%s, %s) failed: realpath() fail %d\n", con_oldpath, con_newpath, errno);
#endif
            return -1;
        }
    }

    exe_disk_con_file_t *con_file = NULL;
    exe_disk_con_file_t *free_file = NULL;
    find_con_file(con_full_oldpath, &con_file, &free_file);

#ifdef MEMFS_DEBUG
    printf("link(%s, %s) confile %p\n", con_oldpath, con_newpath, con_file);
#endif

    if (!con_file) {
        if (!free_file) {
            con_file = enlarge_con_files();
        } else {
            con_file = free_file;
        }

        con_file->fd = -1;
        con_file->state = CON_STATE_DIRECT;
        con_file->name = malloc(MAX_LINKS * sizeof(char*));
        con_file->name[0] = con_full_oldpath;
        con_file->name[1] = con_full_newpath;
        for (i=2; i<MAX_LINKS; i++) con_file->name[i] = NULL;
        con_file->size = 0;
        con_file->buf_size = 0;
        con_file->contents = NULL;
    } else {
        // existing, just add
        int found = 0;
        int j;
        for (j=0; j<MAX_LINKS; j++) {
            if (con_file->name[j] == NULL) {
                con_file->name[j] = con_full_newpath;
                found = 1;
                break;
            }
        }
        if (!found) {
            // no spce left...
            //
            errno = EMLINK;
            free(con_full_oldpath);
            free(con_full_newpath);
            return -1;
        }
        free(con_full_oldpath);
    }
    return 0;
}

exe_disk_con_file_t *enlarge_con_files() {
#ifdef MEMFS_DEBUG
    printf("memfs: enlarging con files\n");
#endif
    // no free slot: allocate a new array
    exe_disk_con_file_t *old_con_files = __exe_fs.con_files;
    exe_disk_con_file_t *new_con_files = malloc(sizeof(exe_disk_con_file_t) * __exe_fs.n_con_files * 2);
    memcpy(new_con_files, __exe_fs.con_files, sizeof(exe_disk_con_file_t) * __exe_fs.n_con_files);
    free(__exe_fs.con_files);
    __exe_fs.con_files = new_con_files;
    unsigned int i;
    for (i=__exe_fs.n_con_files; i<__exe_fs.n_con_files * 2; i++) {
        __exe_fs.con_files[i].state = CON_STATE_FREE;
    }
    exe_disk_con_file_t *result = &__exe_fs.con_files[__exe_fs.n_con_files];
    __exe_fs.n_con_files *= 2;

    for (i=0; i<MAX_FDS; i++) {
        if ((__exe_env.fds[i].type == CONCRETE_FILE) && (__exe_env.fds[i].flags & eOpen)) {
            __exe_env.fds[i].dcfile = __exe_fs.con_files + (__exe_env.fds[i].dcfile - old_con_files);
        }
    }

    return result;
}

int rename(const char *oldpath, const char *newpath) {
    const char *con_oldpath = __concretize_string(oldpath);
    const char *con_newpath = __concretize_string(newpath);
    char *con_full_oldpath = malloc(1024);
    char *realpath_ret = realpath(con_oldpath, con_full_oldpath); 
    if (!realpath_ret) {
        if (errno == ENOENT) {
        } else {
            free(con_full_oldpath);
#ifdef MEMFS_DEBUG
            printf("rename(%s, %s) failed: realpath() fail %d\n", con_oldpath, con_newpath, errno);
#endif
            return -1;
        }
    }

    char *con_full_newpath = malloc(1024);
    realpath_ret = realpath(con_newpath, con_full_newpath);
    if (!realpath_ret) {
        if (errno == ENOENT) {
        } else {
            free(con_full_oldpath);
            free(con_full_newpath);
#ifdef MEMFS_DEBUG
            printf("rename(%s, %s) failed: realpath() fail %d\n", con_oldpath, con_newpath, errno);
#endif
            return -1;
        }
    }

    exe_disk_con_file_t *con_file = NULL, *free_file = NULL;
    find_con_file(con_full_oldpath, &con_file, &free_file);
    if (con_file) {
        // already exists...
        int i;
        for (i=0; i<MAX_LINKS; i++) {
            if (con_file->name[i])
                if (!strcmp(con_full_oldpath, con_file->name[i])) {
                    con_file->name[i] = con_full_newpath;
                    break;
                }
        }
        free(con_full_oldpath);
    } else {
        // not touched...
        //

        if (free_file) {
            con_file = free_file;
        } else {
            con_file = enlarge_con_files();
        }

        int fd = syscall(__NR_open, con_full_oldpath, O_RDONLY, 0666);
        if (fd < 0) {
            printf("rename(%s, %s) failed: can't read original file. %d\n", con_oldpath, con_newpath, errno);
            free(con_full_oldpath);
            free(con_full_newpath);
            return -1;
        }

        if (make_con_inmem(fd, con_file)) return -1;

        syscall(__NR_close, fd);

        con_file->name = malloc(MAX_LINKS * sizeof(char*));
        con_file->name[0] = con_full_newpath;
        int i;
        for (i=1; i<MAX_LINKS; i++) con_file->name[i] = 0;
        con_file->fd = -1;
        free(con_full_oldpath);
    }
    return 0;
}

int make_con_inmem(int fd, exe_disk_con_file_t *con_file) {
#ifdef MEMFS_DEBUG
    printf("changing state from DIRECT to INMEM, dcfile %p\n", con_file);
#endif
    off_t file_size = syscall(__NR_lseek, fd, 0, SEEK_END);
    if (file_size == 0) {
        con_file->contents = NULL;
    } else if (file_size < 0) {
        // lseek failed....
        con_file->state = CON_STATE_FORCE_DIRECT;
#ifdef MEMFS_DEBUG
        printf("change failed! lseek error: %d\n", errno);
#endif
        return -1;
    } else {
        con_file->contents = malloc(file_size);
        if (!con_file->contents) {
#ifdef MEMFS_DEBUG
            printf("change failed! malloc error: %d\n", errno);
#endif
            con_file->state = CON_STATE_FORCE_DIRECT;
            return -1;
        }
        off_t read_len = syscall(__NR_pread64, fd, con_file->contents, file_size, 0);
        if (read_len != file_size) {
            errno = EIO;
#ifdef MEMFS_DEBUG
            printf("change failed! read %ld != expected %ld\n", read_len, file_size);
#endif
            free(con_file->contents);
            con_file->state = CON_STATE_FORCE_DIRECT;
            return -1;
        }
    }

    klee_sys_close(fd);
    con_file->size = file_size;
    con_file->buf_size = file_size;
    con_file->state = CON_STATE_INMEM;
    return 0;
}

int find_con_file(char *filename, exe_disk_con_file_t **con_file, exe_disk_con_file_t **free_file) {
    unsigned i;
    *con_file = NULL;
    for (i=0; i<__exe_fs.n_con_files; i++) {
        if (__exe_fs.con_files[i].state != CON_STATE_FREE) {
            int j;
            for (j=0; j<MAX_LINKS; j++) {
                if (__exe_fs.con_files[i].name[j])
                    if (!strcmp(filename, __exe_fs.con_files[i].name[j])) {
                        *con_file = &__exe_fs.con_files[i];
                        return 1;
                    }
            }
        } else {
            if (free_file)
                *free_file = &__exe_fs.con_files[i];
        }
    }
    return 0;
}

void free_con_file(exe_disk_con_file_t *con_file) {
    if (con_file->stat)
        free(con_file->stat);
    if (con_file->contents)
        free(con_file->contents);
    con_file->size = 0;
    con_file->buf_size = 0;
    con_file->mode = 0;
    if (con_file->name) {
        int i;
        for (i=0; i<MAX_LINKS; i++) {
            if (con_file->name[i])
                free(con_file->name[i]);
        }
    }
    free(con_file->name);
    con_file->name = NULL;
    con_file->fd = 0;
    con_file->state = CON_STATE_FREE;
}
