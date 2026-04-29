/* Definition for struct stat.
   Copyright (C) 2020-2024 Free Software Foundation, Inc.
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
   License along with the GNU C Library.  If not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef _BITS_STRUCT_STAT_H
#define _BITS_STRUCT_STAT_H 1

#include "ams/bits/types.h" /* For __mode_t and __dev_t.  */
#include "ams/bits/types/time.h"

/* The Single Unix specification says that some more types are
   available here.  */

__BEGIN_DECLS

#ifndef __dev_t_defined
typedef __dev_t dev_t;
#define __dev_t_defined
#endif

#ifndef __gid_t_defined
typedef __gid_t gid_t;
#define __gid_t_defined
#endif

#ifndef __ino_t_defined
typedef __ino_t ino_t;
#define __ino_t_defined
#endif

#ifndef __mode_t_defined
typedef __mode_t mode_t;
#define __mode_t_defined
#endif

#ifndef __nlink_t_defined
typedef __nlink_t nlink_t;
#define __nlink_t_defined
#endif

#ifndef __off_t_defined
typedef __off_t off_t;
#define __off_t_defined
#endif

#ifndef __uid_t_defined
typedef __uid_t uid_t;
#define __uid_t_defined
#endif

struct stat {

  __dev_t st_dev; /* Device.  */

  unsigned short int __pad1;

  __ino_t st_ino; /* File serial number.	*/

  __ino_t __st_ino; /* 32bit file serial number.	*/

  __mode_t  st_mode;  /* File mode.  */
  __nlink_t st_nlink; /* Link count.  */

  __uid_t st_uid; /* User ID of the file's owner.	*/
  __gid_t st_gid; /* Group ID of the file's group.*/

  __dev_t st_rdev; /* Device number, if device.  */

  unsigned short int __pad2;

  __off_t st_size; /* Size of file, in bytes.  */

  __blksize_t st_blksize; /* Optimal block size for I/O.  */

  __blkcnt_t st_blocks; /* Number 512-byte blocks allocated. */
};

struct statfs
  {
    __fsword_t f_type;
    __fsword_t f_bsize;

    __fsblkcnt_t f_blocks;
    __fsblkcnt_t f_bfree;
    __fsblkcnt_t f_bavail;
    __fsfilcnt_t f_files;
    __fsfilcnt_t f_ffree;

    __fsid_t f_fsid;
    __fsword_t f_namelen;
    __fsword_t f_frsize;
    __fsword_t f_flags;
    __fsword_t f_spare[4];
  };

/* Tell code we have these members.  */
#define _STATBUF_ST_BLKSIZE
#define _STATBUF_ST_RDEV
/* Nanosecond resolution time values are supported.  */
#define _STATBUF_ST_NSEC

#endif /* _BITS_STRUCT_STAT_H  */