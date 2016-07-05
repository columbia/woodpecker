/*
 * Copyright (C) 2002     Manuel Novoa III
 * Copyright (C) 2000-2005 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include "_string.h"
#include "stdio.h"

#ifdef WANT_WIDE
# define Wstrlen wcslen
#else
# define Wstrlen strlen
#endif

libc_hidden_proto(Wstrlen)

size_t Wstrlen(const Wchar *s)
{
//	fprintf(stderr, "uclibc heming strlen %p\n", s);
/*	register const Wchar *p;

	for (p=s ; *p ; p++);

	return p - s;
*/

  const Wchar *p = s;
  while (*p)
    ++p;
  return p - s;

}
libc_hidden_def(Wstrlen)
