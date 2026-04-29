#ifndef MOUNT_H
#define MOUNT_H

struct mount {
  struct super_block *sb;
  struct mount       *parent;
  struct inode       *mountpt;
};

#endif