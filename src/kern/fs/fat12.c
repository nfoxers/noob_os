#include "fs/fat12.h"
#include "driver/time.h"
#include "fs/vfs.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "video/printf.h"
#include "video/video.h"
#include <lib/ctype.h>
#include <lib/errno.h>
#include <stdint.h>

#define BS ((struct bootsect *)0x7c00)

struct fat12info {
  uint16_t fat_start;
  uint16_t fat_addr;
  uint16_t fat_sectors;

  uint16_t         root_start;
  struct direntry *root_addr;
  uint16_t         root_sectors;

  uint16_t data_start;
  uint16_t data_addr;
  uint16_t data_sectors;
};

struct fat12info fsinfo = {0};

extern struct inode root_dir;
extern struct proc *volatile p_curproc;

#define ofile p_curproc->p_user->u_ofile

void set_vfs(struct inode *);

void init_fs() {
  print_init("fat", "initializing filesystem driver...", 0);

  if (BS->bootsig != 0xaa55) {
    printk("mismatching boot signatures, either corrupt memory or idk\n");
    return;
  }

  fsinfo.fat_sectors = BS->fats * BS->spf;
  fsinfo.fat_start   = BS->sec_reserved;
  fsinfo.fat_addr    = fsinfo.fat_start * 512 + 0x7c00;

  fsinfo.root_start   = fsinfo.fat_sectors + fsinfo.fat_start;
  fsinfo.root_addr    = (struct direntry *)(fsinfo.root_start * 512 + 0x7c00);
  fsinfo.root_sectors = (32 * BS->roots + BS->bps - 1) / BS->bps;

  fsinfo.data_start   = fsinfo.root_start + fsinfo.root_sectors;
  fsinfo.data_addr    = fsinfo.data_start * 512 + 0x7c00;
  fsinfo.data_sectors = BS->total_sec - fsinfo.data_start;

  root_dir.type       = INODE_DIR;
  root_dir.cluster0   = 0;
  root_dir.entaddr    = fsinfo.root_addr;
  root_dir.size       = 0;
  root_dir.permission = 0766;

  set_vfs(&root_dir);

  memcpy(&p_curproc->p_user->u_cdir, &root_dir, sizeof(struct inode));

  for (int i = 0; i < NOFILE; i++) {
    ofile[i] = 0;
  }
}

void tolowers(char *c) {
  while (*c) {
    *c = tolower(*c);
    c++;
  }
}

int fat_name_match(struct direntry *restrict e, const char *restrict name) {
  char fatname[13];
  char fname[9];
  char ext[4];

  snprintkf(fname, sizeof(fname), "%.8s", e->fname);
  snprintkf(ext, sizeof(ext), "%.3s", e->ext);

  for (int i = 7; i >= 0 && fname[i] == ' '; i--)
    fname[i] = 0;

  for (int i = 2; i >= 0 && ext[i] == ' '; i--)
    ext[i] = 0;

  if (ext[0])
    snprintkf(fatname, sizeof(fatname), "%s.%s", fname, ext);
  else
    snprintkf(fatname, sizeof(fatname), "%s", fname);

  char *tmp = strdup(name);
  tolowers(tmp);
  tolowers(fatname);

  int ret = !strcmp(fatname, tmp);
  free(tmp);
  return ret;
}

void set_perms(struct inode *in, uint8_t fatt) {
  in->permission = 0666;
  if (fatt & FAT_RDONLY)
    in->permission &= ~(PRM_W * (PRM_USR | PRM_GRP | PRM_OTH));
}

int fat_lookup_from(const char *restrict path, const struct inode *restrict from, struct inode *restrict buf);

struct inode *finddir_fat(struct inode *in, const char *path) {
  struct inode *inode = malloc(sizeof(struct inode));

  if (fat_lookup_from(path, in, inode)) {
    free(inode);
    errno = ENOENT;
    return NULL;
  }

  return inode;
}

inline void *get_addr(uint16_t lc) {
  return (void *)(fsinfo.data_addr + (lc - 2) * 1024);
}

void fat2normal(const char *fname, const char *ext, char *buf) {
  if (!fname || !ext || !buf)
    return;
  int count = 0;
  while (count++ < 8 && (*fname && *fname != ' ')) {
    *buf++ = *fname++;
  }
  if (*ext == ' ') {
    *buf &= 0;
    return;
  }
  *buf++ = '.';
  count  = 0;
  while (count++ < 3 && (*ext && *ext != ' ')) {
    *buf++ = *ext++;
  }
  *buf = 0;
}

/* forward declarations */

struct file_ops  fat_fops;
struct inode_ops fat_iops;

int fat_read(struct inode *in, void *buf, size_t off, size_t count);

/* syscall functions */

DIR *opendir_fat(struct inode *in) {
  if (!in || in->type != INODE_DIR)
    return NULL;

  DIR *d = malloc(sizeof(DIR) * DT_MAXDIR);

  struct direntry *dr = in->entaddr == root_dir.entaddr ? root_dir.entaddr : (struct direntry *)get_addr(in->entaddr->low_cluster);

  // printkf("entaddr: %x\n", dr);

  int i;
  for (i = 0; i < DT_MAXDIR; i++, dr++) {
    if (!dr->fname[0])
      break;
    if ((uint8_t)dr->fname[0] == 0xe5)
      continue;

    d[i].size = dr->size;
    // snprintkf(d[i].data, DIRENT_MAXSIZ, "% 8.8s % 3.3s", dr->fname, dr->ext);
    d[i].type = dr->fatt & FAT_SUBDIR ? INODE_DIR : INODE_FILE;

    char buf[20];
    fat2normal(dr->fname, dr->ext, buf);
    strncpy(d[i].data, buf, sizeof buf);

    d[i].in         = malloc(sizeof(struct inode));
    struct inode *k = finddir_fat(in, buf);

    if (!k) {
      free(d[i].in);
      d[i].in = NULL;
      printkf("unable to find %s\n", buf);
      continue;
    }

    memcpy(d[i].in, k, sizeof(struct inode));
    free(k);
  }

  d[0].count = i;
  /*
  d[0].in = malloc(sizeof(struct inode));
  memcpy(d[0].in, in, sizeof(struct inode));
  */
  return d;
}

int closedir_fat(struct inode *dir, DIR *d) {
  (void)dir;

  for (int i = 0; i < d->count; i++) {
    d[i].in ? free(d[i].in) : (void)0;
  }

  free(d);
  return 0;
}

int create_fat(struct inode *dir, const char *name, uint16_t flg) {
  (void)dir;
  (void)name;
  (void)flg;
  return -ENOSYS;
}

int unlink_fat(struct inode *dir, const char *name) {
  struct inode buf;
  if (fat_lookup_from(name, dir, &buf)) {
    return -ENOENT;
  }

  buf.entaddr->fname[0] = 0xe5;

  return 0;
}

int open_fat(struct inode *dir, struct file *f) {
  (void)dir;
  (void)f;

  return -ENOSYS;
}

int close_fat(struct file *f) {
  (void)f;

  return -ENOSYS;
}

int read_fat(struct file *f, void *buf, size_t siz) {
  (void)f;
  (void)buf;
  (void)siz;

  if (!(f->inode->permission & S_IRUSR))
    return -EPERM;
  size_t ret = fat_read(f->inode, buf, f->position, siz);
  f->position += ret;

  return ret;
}

int write_fat(struct file *f, const void *buf, size_t siz) {
  (void)f;
  (void)buf;
  (void)siz;
  return -ENOSYS;
}

inline void set_vfs(struct inode *in) {
  in->ops = &fat_iops;
  in->fops = &fat_fops;
}

int fat_lookup_from(const char *restrict path, const struct inode *restrict from, struct inode *restrict buf) {
  char *pth = strdup(path);

  char *sv;
  char *tok = strtok_r(pth, "/", &sv);

  struct direntry *dr;
  int              entries = 32;
  int              found   = 0;
  int              cluster = 0;

  if (!from) {
    cluster = 0x7fffff00;
    dr      = fsinfo.root_addr;
  }

      //printkf("cl: %d\n", cluster);

  while (tok) {
    found = 0;

    if (cluster == 0) {
      //printkf("en: %p\n", from->entaddr);
      dr      = get_addr(from->entaddr->low_cluster);
      entries = 32;
    } else if (cluster == 0x7fffff00) {
      // dr      = get_addr(from->entaddr->low_cluster);
      entries = 32;
    } else {
      dr      = get_addr(cluster);
      entries = 1024 / sizeof(struct direntry);
    }
    //printkf("cl: %d\n", cluster);
    // printkf("der: %p\n", dr - 0x7c00);
    for (int i = 0; i < entries; i++) {
      if (dr[i].fname[0] == 0x00)
        break;
      if ((uint8_t)dr[i].fname[0] == 0xE5)
        continue;
      if (fat_name_match(&dr[i], tok)) {
        found++;
        if (dr[i].fatt & FAT_SUBDIR) {
          cluster   = dr[i].low_cluster;
          buf->type = INODE_DIR;
        } else {
          buf->type = INODE_FILE;
        }

        buf->cluster0 = dr[i].low_cluster;
        buf->entaddr  = &dr[i];
        //printkf("ent: %p\n", buf->entaddr);
        buf->size     = dr[i].size;

        set_perms(buf, dr[i].fatt);
        set_vfs(buf);

        break;
      }
    }

    if (!found) {
      free(pth);
      return 1;
    }

    tok = strtok_r(NULL, "/", &sv);
  }

  free(pth);
  return 0;
}

struct inode_ops fat_iops = {
  .opendir = opendir_fat,
  .closedir = closedir_fat,
  .lookup = finddir_fat,
  .unlink = unlink_fat
};

struct file_ops fat_fops = {
    .open  = 0,
    .close = 0,
    .read  = read_fat,
    .write = write_fat};

/* inner filesystem */

uint16_t fattab_read(uint16_t cluster) {
  uint8_t *fat        = (uint8_t *)(uintptr_t)fsinfo.fat_addr;
  uint16_t fat_offset = cluster + (cluster / 2);
  uint16_t ent_offset = fat_offset % 1024;

  uint16_t val = *(uint16_t *)&fat[ent_offset];
  if (val != *(uint16_t *)&fat[ent_offset + 1024]) {
    printkf("warn: two fat tables are different at offset %x\n", ent_offset);
  }

  val = (cluster & 1) ? val >> 4 : val & 0xfff;

  return val;
}

void fattab_write(uint16_t cluster, uint16_t next) {
  uint8_t *fat        = (uint8_t *)(uintptr_t)fsinfo.fat_addr;
  uint16_t fat_offset = cluster + (cluster / 2);
  uint16_t ent_offset = fat_offset % 1024;

  next &= 0x0FFF;

  uint16_t tmp = *(uint16_t *)&fat[ent_offset];

  if (cluster & 1) {
    tmp = (tmp & 0x000F) | (next << 4);
  } else {
    tmp = (tmp & 0xF000) | next;
  }

  *(uint16_t *)&fat[ent_offset]        = tmp;
  *(uint16_t *)&fat[ent_offset + 1024] = tmp;
}

int fat_read(struct inode *in, void *buf, size_t off, size_t count) {
  if (!in || !buf)
    return 0;

  uint16_t cluster = in->entaddr->low_cluster;
  size_t   read    = 0;

  cluster = off > 1024 ? fattab_read(cluster) : cluster;
  off     = off % 1024;

  while (cluster < 0xff8) {
    void *addr = get_addr(cluster);

    size_t dcount = 1024 - off;
    dcount        = dcount > count ? count : dcount;

    // printkf("dcount: %d\n", dcount);

    memcpy(buf, addr + off, dcount);

    if (dcount > count) {
      printkf("fat read: overflow!\n");
      break;
    }

    count -= dcount;
    read += dcount;

    off     = 0;
    cluster = fattab_read(cluster);
  }

  return read;
}