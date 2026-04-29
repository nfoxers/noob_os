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

static struct file_system_type fs_types = {.next = NULL};

/* file type utilities */

void register_fstype(struct file_system_type *fs_type) {
  if(!fs_type->name || !*fs_type->name) {
    printkf("register fs err: no name\n");
    return;
  }
  
  struct file_system_type *c = &fs_types;
  
  while(c->next) c = c->next;

  fs_type->next = NULL;
  c->next = fs_type;

}

extern struct file_system_type ext2_fs_type;

struct file_system_type *get_fstype(const char *name) {
  struct file_system_type *c = &fs_types;

  while(c->next) {
    c = c->next;
    if(c->name && !strcmp(c->name, name)) {
      return c;
    }
    
  }

  return NULL;
}

void register_fses() {
  register_fstype(&ext2_fs_type);
}

/* filesystem utilities */

void mount_root(dev_t blkdev, const char *fstype) {
  struct file_system_type *f = get_fstype(fstype);
  if(!f) {
    printkf("invalid fstype '%s'\n", fstype);
    return;
  }

  struct inode *i = f->mount(f, (void*) &blkdev, NULL, FT_DEVT);
  struct mount *mnt = malloc(sizeof(*mnt));

  mnt->sb = i->sb;
  
  imount(&rootnode, mnt);
}

// parses filesystem types and returns types
struct file_system_type *detect_fstype(struct block_dev *bd) {
  struct file_system_type *d = NULL;

  uint8_t *b = malloc(512 * 4);
  bread(bd, 0, 4, b);

  // ext
  struct ext2_superblock *sb = (struct ext2_superblock *)(b + 1024);
  if(sb->s_magic == 0xef53) {    
    d = get_fstype("ext2");
    goto done;
  }

  done:
  free(b);
  return d;
}