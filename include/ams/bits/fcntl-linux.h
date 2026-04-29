/* O_*, F_*, FD_* bit values for Linux.
   Copyright (C) 2001-2024 Free Software Foundation, Inc.
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

/* This file contains shared definitions between Linux architectures
   and is included by <bits/fcntl.h> to declare them.  The various
   #ifndef cases allow the architecture specific file to define those
   values with different values.

   A minimal <bits/fcntl.h> contains just:

   struct flock {...}
   #ifdef __USE_LARGEFILE64
   struct flock64 {...}
   #endif
   #include <bits/fcntl-linux.h>
*/

/* open/fcntl.  */
#define O_ACCMODE 0003
#define O_RDONLY  00
#define O_WRONLY  01
#define O_RDWR    02
#ifndef O_CREAT
#define O_CREAT 0100 /* Not fcntl.  */
#endif
#ifndef O_EXCL
#define O_EXCL 0200 /* Not fcntl.  */
#endif
#ifndef O_NOCTTY
#define O_NOCTTY 0400 /* Not fcntl.  */
#endif
#ifndef O_TRUNC
#define O_TRUNC 01000 /* Not fcntl.  */
#endif
#ifndef O_APPEND
#define O_APPEND 02000
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 04000
#endif
#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif
#ifndef O_SYNC
#define O_SYNC 04010000
#endif
#define O_FSYNC O_SYNC
#ifndef O_ASYNC
#define O_ASYNC 020000
#endif
#ifndef __O_LARGEFILE
#define __O_LARGEFILE 0100000
#endif

#ifndef __O_DIRECTORY
#define __O_DIRECTORY 0200000
#endif
#ifndef __O_NOFOLLOW
#define __O_NOFOLLOW 0400000
#endif
#ifndef __O_CLOEXEC
#define __O_CLOEXEC 02000000
#endif
#ifndef __O_DIRECT
#define __O_DIRECT 040000
#endif
#ifndef __O_NOATIME
#define __O_NOATIME 01000000
#endif
#ifndef __O_PATH
#define __O_PATH 010000000
#endif
#ifndef __O_DSYNC
#define __O_DSYNC 010000
#endif
#ifndef __O_TMPFILE
#define __O_TMPFILE (020000000 | __O_DIRECTORY)
#endif

#define O_ISR(x)  (((x) & O_ACCMODE) == O_RDONLY)
#define O_ISW(x)  (((x) & O_ACCMODE) == O_WRONLY)
#define O_ISRW(x) (((x) & O_ACCMODE) == O_RDWR)

#ifndef F_GETLK
#define F_GETLK  5 /* Get record locking info.  */
#define F_SETLK  6 /* Set record locking info (non-blocking).  */
#define F_SETLKW 7 /* Set record locking info (blocking).  */
#endif
#ifndef F_GETLK64
#define F_GETLK64  12 /* Get record locking info.  */
#define F_SETLK64  13 /* Set record locking info (non-blocking).  */
#define F_SETLKW64 14 /* Set record locking info (blocking).  */
#endif

/* Values for the second argument to `fcntl'.  */
#define F_DUPFD 0 /* Duplicate file descriptor.  */
#define F_GETFD 1 /* Get file descriptor flags.  */
#define F_SETFD 2 /* Set file descriptor flags.  */
#define F_GETFL 3 /* Get file status flags.  */
#define F_SETFL 4 /* Set file status flags.  */

#ifndef __F_SETOWN
#define __F_SETOWN 8
#define __F_GETOWN 9
#endif

#ifndef __F_SETSIG
#define __F_SETSIG 10 /* Set number of signal to be sent.  */
#define __F_GETSIG 11 /* Get number of signal to be sent.  */
#endif
#ifndef __F_SETOWN_EX
#define __F_SETOWN_EX 15 /* Get owner (thread receiving SIGIO).  */
#define __F_GETOWN_EX 16 /* Set owner (thread receiving SIGIO).  */
#endif

/* For F_[GET|SET]FD.  */
#define FD_CLOEXEC 1 /* Actually anything with low bit set goes */

#ifndef F_RDLCK
/* For posix fcntl() and `l_type' field of a `struct flock' for lockf().  */
#define F_RDLCK 0 /* Read lock.  */
#define F_WRLCK 1 /* Write lock.  */
#define F_UNLCK 2 /* Remove lock.  */
#endif

/* For old implementation of BSD flock.  */
#ifndef F_EXLCK
#define F_EXLCK 4 /* or 3 */
#define F_SHLCK 8 /* or 4 */
#endif