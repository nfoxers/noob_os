#include "fs/vfs.h"
#include "mem/mem.h"
#include "fs/fat12.h"
#include "video/printf.h"
#include <stddef.h>
#include <stdint.h>

#define NOFS 10

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

struct inode rootfs[NOFS+1] = {
  {0, 2, INODE_DIR, 0755,
  0, 0,
  ROOT_ADDR+1, {finddir_fat, 0, 0, opendir_fat, closedir_fat}, {0, 0, 0, 0}}, 

  {0, 0, INODE_DIR, 0755,
  0, 0,
ROOT_ADDR, {finddir_fat, 0, 0, opendir_fat, closedir_fat}, {}},
  {0, 0, INODE_DIR, 0755,
  0, 0,
ROOT_ADDR, {finddir_fat, 0, 0, opendir_fat, closedir_fat}, {}},
  {0, 0, INODE_DIR, 0777,
  0, 0,
ROOT_ADDR, {finddir_fat, 0, 0, opendir_fat, closedir_fat}, {}},
  {0, 0, INODE_DIR, 0755,
  0, 0,
ROOT_ADDR, {finddir_fat, 0, 0, opendir_fat, closedir_fat}, {}},

  {0, 0, INODE_DIR, 0755,
  0, 0,
0, {}, {}},
  {0, 0, INODE_DIR, 0755,
  0, 0,
0, {}, {}},

};

char *rootname[NOFS+1] = {
  "home", "bin", "etc", "tmp", "var", "dev", "proc",
  NULL
};

DIR rootdirstream[] = {
  {7, INODE_DIR, 0, &rootfs[0], "home"},
  {0, INODE_DIR, 0, &rootfs[1], "bin"},
  {0, INODE_DIR, 0, &rootfs[2], "etc"},
  {0, INODE_DIR, 0, &rootfs[3], "tmp"},
  {0, INODE_DIR, 0, &rootfs[4], "var"},

  {0, INODE_DIR, 0, &rootfs[5], "dev"},
  {0, INODE_DIR, 0, &rootfs[6], "proc"}
};

struct inode *lookup_root(struct inode *dir, const char *name) {
  (void)dir;
  struct inode *in = malloc(sizeof(struct inode));
  //printkf("done malloced\n");
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

int closedir_root(struct inode *dir, DIR *d) {
  (void)dir; // we do nothing as everything is static
  (void)d;
  return 0;
}

int find_home(struct inode *buf);
int fat_lookup_from(const char *restrict path, const struct inode *restrict from, struct inode *restrict buf);

void init_rootfs() {
  print_init("fs", "initializing rootfs...", 0);
  for(uint32_t i = 0; i < sizeof(rootdirstream)/sizeof(rootdirstream[0])-2; i++) {
    struct inode buf;

    if(fat_lookup_from(rootname[i], NULL, &buf)) {
      printkf("rootfs: can't find %s\n", rootname[i]);
      continue;
    }

    memcpy(&rootfs[i], &buf, sizeof(struct inode));
  }
}
