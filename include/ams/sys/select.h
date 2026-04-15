/* `fd_set' type and related macros, and `select'/`pselect' declarations.
   Copyright (C) 1996-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

/*	POSIX 1003.1g: 6.2 Select from File Descriptor Sets <sys/select.h>  */

#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H 1

#include <features.h>

/* Get definition of needed basic types.  */
#include "ams/bits/types.h"

/* Get __FD_* definitions.  */

/* Get sigset_t.  */
#ifndef ____sigset_t_defined
#define ____sigset_t_defined

#define _SIGSET_NWORDS (1024 / (8 * sizeof(unsigned long int)))
typedef struct
{
  unsigned long int __val[_SIGSET_NWORDS];
} __sigset_t;

#endif

typedef __sigset_t sigset_t;

/* Get definition of timer specification structures.  */
#ifdef __USE_TIME_BITS64
typedef __time64_t time_t;
#else
typedef __time_t time_t;
#endif
struct timeval {
  __time_t      tv_sec;  /* Seconds.  */
  __suseconds_t tv_usec; /* Microseconds.  */
};

#ifdef __USE_XOPEN2K
struct timespec {
  __time_t tv_sec; /* Seconds.  */
  __syscall_slong_t tv_nsec; /* Nanoseconds.  */

};
#endif

#ifndef __suseconds_t_defined
typedef __suseconds_t suseconds_t;
#define __suseconds_t_defined
#endif

#endif /* sys/select.h */
