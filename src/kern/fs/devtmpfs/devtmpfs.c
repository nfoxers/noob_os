#include "ams/sys/stat.h"
#include "fs/devfs.h"
#include "fs/vfs.h"
#include "video/printf.h"
#include "video/video.h"
#include <dev/block_dev.h>
#include <driver/disk/ide.h>
#include <fs/ext2.h>
#include <mem/mem.h>
#include <proc/proc.h>
#include <fs/fs.h>

// todo: implement devtmpfs

struct file_system_type devtmpfs_fs_type;

int devtmpfs_fill_super(struct super_block *sb, void *data) {
  (void)sb;
  (void)data;
  return 0;
}