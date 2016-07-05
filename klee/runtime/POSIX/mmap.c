#define _LARGEFILE64_SOURCE
#include "fd.h"

#include <klee/klee.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define MMAP_PAGE_SIZE 4096

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    fprintf(stderr, "mmap(%lx, %ld, %d, %x, %d, %ld)\n", (long unsigned int)addr, len, prot, flags, fildes, off);
    if (flags & MAP_SHARED)
    {
        fprintf(stderr, "Warning: Shared mmap\n");
        // XXX: not implemented
        errno = ENOTSUP;
        return MAP_FAILED;
    }

    if (flags & MAP_FIXED)
    {
        // fixed map, either fail or at addr
        fprintf(stderr, "Warning: Fixed mmap\n");
        // XXX: not implemented
        errno = ENOTSUP;
        return MAP_FAILED;
    }

    char *mem = malloc(len);
    if (!mem)
    {
        errno = ENOMEM;
        return MAP_FAILED;
    }

    if (flags & MAP_ANONYMOUS)
    {
        // anonymous mapping
        // nothing to do...
    } else {
        // XXX: should be atomic!
        off_t curpos = lseek(fildes, 0, SEEK_CUR);
        lseek(fildes, off, SEEK_SET);
        ssize_t ret = read(fildes, mem, len);
        int read_errno = errno;
        lseek(fildes, curpos, SEEK_SET);
        errno = read_errno;

        if (ret == -1)
        {
            return MAP_FAILED;
        }
    }

    return mem;
}

void *mmap64(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    // XXX: what the ...
    return mmap(addr, len, prot, flags, fildes, off * MMAP_PAGE_SIZE);
}

int munmap(void* addr, size_t len)
{
    fprintf(stderr, "munmap(%lx, %ld)", (unsigned long)addr, len);
    if ((len != 0) || ((size_t)addr % MMAP_PAGE_SIZE))
    {
        errno = EINVAL;
        return -1;
    }
    free(addr);
    return 0;
}
