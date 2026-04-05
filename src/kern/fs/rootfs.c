#include "fs/vfs.h"
#include "mem/mem.h"
#include "fs/fat12.h"

/* we set up /home & /dev */

struct inode *lookup_root(struct inode *dir, const char *name);
DIR *opendir_root(struct inode *dir);

struct inode *finddir_fat(struct inode *in, const char *path);
DIR *opendir_fat(struct inode *in);

struct inode rootinode = {
  0, 0, INODE_DIR, 0766,
  0, 0,
  0, {lookup_root, 0, 0, opendir_root, 0}, {0, 0, 0, 0}
};

struct inode const rootfs[] = {
  {0, 2, INODE_DIR, 0766,
  0, 0,
  ROOT_ADDR+1, {finddir_fat, 0, 0, opendir_fat, 0}, {0, 0, 0, 0}}, 

  {0, 0, INODE_DIR, 0766,
  0, 0,
0, {}, {}}
};

const char *rootname[] = {
  "home", "dev"
};

const DIR rootdirstream[] = {
  {2, 0, INODE_DIR, "home"},
  {0, 0, INODE_DIR, "dev"}
};

struct inode *lookup_root(struct inode *dir, const char *name) {
  (void)dir;
  struct inode *in = malloc(sizeof(struct inode));
  for(uint32_t i = 0; i < sizeof(rootname)/sizeof(rootname[0]); i++) {
    if(!strcmp(name, rootname[i])) {
      memcpy(in, &rootfs[i], sizeof(rootfs[0]));
      return in;
    }
  }
  return NULL;
}

DIR *opendir_root(struct inode *dir) {
  (void)dir;
  return (DIR*)rootdirstream;
}

