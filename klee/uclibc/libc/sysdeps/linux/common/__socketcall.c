/* vi: set sw=4 ts=4: */
/*
 * __socketcall() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <sys/syscall.h>

int syscall(int number, ...);

#ifdef __NR_socketcall
#define __NR___socketcall __NR_socketcall
int __socketcall(int __call, unsigned long *__args) {
return syscall(SYS_socketcall, __call, __args);
}
/*attribute_hidden;
//_syscall2(int, __socketcall, int, call, unsigned long *, args);
syscall(__socketcall, call, args);*/
#endif
