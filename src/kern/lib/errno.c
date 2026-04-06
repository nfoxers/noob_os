#include "lib/errno.h"
#include "stddef.h"

#include "video/printf.h"

#include "stdint.h"

int errno;

const char *_estr[] = {
    "ok",
    "Operation not permitted",
    "No such file or directory",
    "No such process",
    "Interrupted system call",
    "Input/output error",
    "No such device or address",
    "Argument list too long",
    "Exec format error",
    "Bad file descriptor",
    "No child processes",
    "Resource temporarily unavailable",
    "Cannot allocate memory",
    "Permission denied",
    "Bad address",
    "Block device required",
    "Device or resource busy",
    "File exists",
    "Invalid cross-device link",
    "No such device",
    "Not a directory",
    "Is a directory",
    "Invalid argument",
    "Too many open files in system",
    "Too many open files",
    "Inappropriate ioctl for device",
    "Text file busy",
    "File too large",
    "No space left on device",
    "Illegal seek",
    "Read-only file system",
    "Too many links",
    "Broken pipe",
    "Numerical argument out of domain",
    "Numerical result out of range",
    "Resource deadlock avoided",
    "File name too long",
    "No locks available",
    "Function not implemented",
    "Directory not empty",
};

const char *strerror(int err) {
  if((unsigned int)err < sizeof(_estr)/sizeof(_estr[0])) return _estr[err];
  return NULL;
}

void perror(const char *s) {
  if(!s || !*s) {
    return;
  }

  printkf("%s: %s\n", s, strerror(errno));
}