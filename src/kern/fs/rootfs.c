#include "fs/vfs.h"
#include "mem/mem.h"
#include "fs/fat12.h"
#include "video/printf.h"

/* we set up /, /home, & /dev */

struct inode *lookup_root(struct inode *dir, const char *name);
DIR *opendir_root(struct inode *dir);
int closedir_root(struct inode *dir, DIR *d);

struct inode *finddir_fat(struct inode *in, const char *path);
DIR *opendir_fat(struct inode *in);
int closedir_fat(struct inode *in, DIR *d);

struct inode rootinode = {
  0, 0, INODE_DIR, 0766,
  0, 0,
  0, {lookup_root, 0, 0, opendir_root, closedir_root}, {0, 0, 0, 0}
};

struct inode rootfs[] = {
  {0, 2, INODE_DIR, 0766,
  0, 0,
  ROOT_ADDR+1, {finddir_fat, 0, 0, opendir_fat, closedir_fat}, {0, 0, 0, 0}}, 

  {0, 0, INODE_DIR, 0766,
  0, 0,
0, {}, {}}
};

char *rootname[] = {
  "home", "dev"
};

DIR rootdirstream[] = {
  {2, INODE_DIR, 0, &rootfs[0], "home"},
  {0, INODE_DIR, 0, &rootfs[1], "dev"}
};

struct inode *lookup_root(struct inode *dir, const char *name) {
  (void)dir;
  struct inode *in = malloc(sizeof(struct inode));
  printkf("done malloced\n");
  for(uint32_t i = 0; i < sizeof(rootname)/sizeof(rootname[0]); i++) {
    if(!strcmp(name, rootname[i])) {
      memcpy(in, &rootfs[i], sizeof(rootfs[0]));
      return in;
    }
  }
  return NULL;
}

DIR *opendir_root(struct inode *dir) {
  printkf("root call\n");
  (void)dir;
  return (DIR*)rootdirstream;
}

int closedir_root(struct inode *dir, DIR *d) {
  printkf("root ccall\n");
  (void)dir;
  (void)d;
  return 0;
}

int find_home(struct inode *buf);

void init_rootfs() {
  struct inode hom;
  if(find_home(&hom)) {
    printkf("rfs: can't find home\n");
    return;
  }
  memcpy(&rootfs[0], &hom, sizeof(struct inode));
}
