/*
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#if 0		/*  Heming commented this in order that per thread errno can work. */
#include "internal_errno.h"
#include <stdio.h>

/* psm: moved to bits/errno.h: libc_hidden_proto(__errno_location) */
libc_hidden_proto(__errno_location)
int * weak_const_function __errno_location (void)
{
  fprintf(stderr, "TERN __errno_location weak_const_function %p\n", (void *)&errno);
    return &errno;
}
#ifdef IS_IN_libc /* not really need, only to keep in sync w/ libc_hidden_proto */
libc_hidden_weak(__errno_location)
#endif
#endif
