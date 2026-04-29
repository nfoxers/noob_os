#ifndef FSTYPE_H
#define FSTYPE_H

#define FT_DEVT 0x01

#include <ams/sys/types.h>

void mount_root(dev_t blkdev, const char *fstype);

#endif