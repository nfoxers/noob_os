#include "fs/fat12.h"
#include "driver/time.h"
#include "fs/devfs.h"
#include "fs/vfs.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "video/printf.h"
#include "video/video.h"
#include <lib/ctype.h>
#include <lib/errno.h>
#include <stdint.h>

#define BS ((struct bootsect *)0x7c00)

#define FATROOT_DISABLE 1

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

struct super_block g_fat_sb;
struct fat_sb_info ramfat_info;

struct inode          rootnode;
struct fat_inode_info rootinfo;

extern struct super_block devblock;
extern struct inode       devnode;
extern struct mount       devmnt;

struct file_ops  fat_fops;
struct inode_ops fat_iops;
struct super_ops fat_sops;

extern struct proc *volatile p_curproc;

#define ofile p_curproc->p_user->u_ofile

void set_vfs(struct inode *);

void set_special();

void init_fs() {
  print_init("fat", "initializing filesystem driver...", 0);

  if (BS->bootsig != 0xaa55) {
    printk("mismatching boot signatures, either corrupt memory or idk\n");
    return;
  }
#if !FATROOT_DISABLE
  fsinfo.fat_sectors = BS->fats * BS->spf;
  fsinfo.fat_start   = BS->sec_reserved;
  fsinfo.fat_addr    = fsinfo.fat_start * 512 + 0x7c00;

  fsinfo.root_start   = fsinfo.fat_sectors + fsinfo.fat_start;
  fsinfo.root_addr    = (struct direntry *)(fsinfo.root_start * 512 + 0x7c00);
  fsinfo.root_sectors = (32 * BS->roots + BS->bps - 1) / BS->bps;

  fsinfo.data_start   = fsinfo.root_start + fsinfo.root_sectors;
  fsinfo.data_addr    = fsinfo.data_start * 512 + 0x7c00;
  fsinfo.data_sectors = BS->total_sec - fsinfo.data_start;

  ramfat_info.data_start = fsinfo.data_start;
  ramfat_info.dir_start  = fsinfo.root_start;
  ramfat_info.fat_start  = fsinfo.fat_start;

  g_fat_sb.s_dev   = 0;
  g_fat_sb.s_inext = 0;
  g_fat_sb.s_op    = &fat_sops;

  rootinfo.first_clust   = 0;
  rootinfo.loc.dir_clust = 0;
  rootinfo.loc.offset    = 0;

  rootnode.sb    = &g_fat_sb;
  rootnode.gid   = 0;
  rootnode.uid   = 0;
  rootnode.ino   = 0;
  rootnode.size  = 0;
  rootnode.mode  = S_IFDIR | 0766;
  rootnode.pdata = &rootinfo;
  rootnode.ops   = &fat_iops;
  rootnode.fops  = &fat_fops;

  set_vfs(&rootnode);
  set_dev(&g_fat_sb, &rootnode);

  set_special();

  memcpy(&p_curproc->p_user->u_cdir, &rootnode, sizeof(struct inode));

#endif

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
  in->mode &= ~0777;
  in->mode |= 0666;
  if (fatt & FAT_RDONLY)
    in->mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
}

fsino_t fat_lookup_from(const char *restrict path, const struct inode *restrict from);

ino_t lookup_fat(struct inode *in, const char *path) {
  fsino_t fsino = 0;

  if ((int)(fsino = fat_lookup_from(path, in)) == -1) {
    errno = ENOENT;
    return -1;
  }

  return fsino;
}

void *get_addr(uint16_t lc) {
  if (lc <= 2)
    return fsinfo.root_addr;
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

struct direntry *get_dirent(struct fat_dirloc *dl) {
  return (struct direntry *)((char *)get_addr(dl->dir_clust) + dl->offset);
}

/* forward declarations */

int fat_read(struct inode *in, void *buf, size_t off, size_t count);

/* syscall functions */

DIR *opendir_fat(struct inode *in) {
  if (!in || !S_ISDIR(in->mode))
    return NULL;

  DIR             *d     = malloc(sizeof(DIR) * DT_MAXDIR);
  struct direntry *dr    = get_addr(((struct fat_inode_info *)in->pdata)->first_clust);
  uint16_t         clust = ((struct fat_inode_info *)in->pdata)->first_clust;
  // printkf("entaddr: %x\n", dr);

  int i;
  for (i = 0; i < DT_MAXDIR; i++, dr++) {
    if (!dr->fname[0])
      break;
    if ((uint8_t)dr->fname[0] == 0xe5)
      continue;

    d[i].size = dr->size;
    // snprintkf(d[i].data, DIRENT_MAXSIZ, "% 8.8s % 3.3s", dr->fname, dr->ext);
    d[i].type &= S_IFMT;
    d[i].type |= dr->fatt & FAT_SUBDIR ? S_IFDIR : S_IFREG;

    char buf[20];
    fat2normal(dr->fname, dr->ext, buf);
    strncpy(d[i].data, buf, sizeof buf);

    d[i].in = iget(&g_fat_sb, clust << 16 | i * sizeof(d[0]));

    if (!d[i].in) {
      d[i].in = NULL;
      printkf("unable to find %s\n", buf);
      continue;
    }
  }
  d[0].count = i;
  return d;
}

int closedir_fat(struct inode *dir, DIR *d) {
  (void)dir;

  for (int i = 0; i < d->count; i++) {
    d[i].in ? iput(d[i].in) : (void)0;
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

int unlink_fat(struct inode *in, const char *name) {
  fsino_t i = fat_lookup_from(name, in);
  if (i < 0)
    return -ENOENT;
  struct inode    *k = iget(in->sb, i);
  struct direntry *d = get_dirent(&((struct fat_inode_info *)k->pdata)->loc);
  d->fname[0]        = 0xe5;
  return 0;
}

int read_fat(struct file *f, void *buf, size_t siz) {
  (void)f;
  (void)buf;
  (void)siz;

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
  in->ops  = &fat_iops;
  in->fops = &fat_fops;
}

fsino_t fat_lookup_from(const char *restrict path, const struct inode *restrict from) {
  char *pth = strdup(path);

  char *sv;
  char *tok = strtok_r(pth, "/", &sv);

  struct direntry *dr;
  int              entries = 32;
  int              found   = 0;
  int              cluster = 0;

  fsino_t ret = -1;

  cluster = ((struct fat_inode_info *)from->pdata)->first_clust;

  if (!from) {
    cluster = 0x7fffff00;
    dr      = fsinfo.root_addr;
  }

  while (tok) {
    found = 0;

    dr = get_addr(cluster);
    /*
    if (cluster == 0) {
      cluster = ((struct fat_inode_info *)from->pdata)->first_clust;
      dr      = get_addr(cluster);
      entries = 32;
    } else if (cluster == 0x7fffff00) {
      entries = 32;
    } else {
      dr      = get_addr(cluster);
      entries = 1024 / sizeof(struct direntry);
    }
    */

    // printkf("dir loc: %x\n", (char *)dr - 0x7c00);
    for (int i = 0; i < entries; i++) {
      // printkf("%s vs %s\n", dr[i].fname, tok);
      if (dr[i].fname[0] == 0x00)
        break;
      if ((uint8_t)dr[i].fname[0] == 0xE5)
        continue;
      if (fat_name_match(&dr[i], tok)) {
        found++;

        // printkf("RET clust_par: %d off %d\n", cluster, i * sizeof(dr[0]));
        ret = (cluster << 16) | ((uintptr_t)&dr[i] - (uintptr_t)dr);

        if (dr[i].fatt & FAT_SUBDIR) {
          cluster = dr[i].low_cluster;
        }

        break;
      }
    }

    if (!found) {
      free(pth);
      return -1;
    }

    tok = strtok_r(NULL, "/", &sv);
  }

  free(pth);
  if (found)
    return ret;

  return -1;
}

mode_t get_mode(uint8_t fatt) {
  mode_t m = 0766;
  if (fatt & FAT_SUBDIR) {
    m |= S_IFDIR;
    return m;
  }
  m |= S_IFREG;
  return m;
}

void fat_inoder(struct inode *r) {
  fsino_t           f   = r->ino;
  struct fat_dirloc loc = {.dir_clust = f >> 16, .offset = f & ((1 << 16) - 1)};
  // printkf("clust: %d off: %d\n", loc.dir_clust, loc.offset);
  if (loc.dir_clust == 0 && loc.offset == 0) {
    // printkf("root detected\n");
    struct fat_inode_info *inf = malloc(sizeof(struct fat_inode_info));
    memcpy(inf, &rootinfo, sizeof(*inf));
    memcpy(r, &rootnode, sizeof(*r));
    r->pdata = inf;
    r->fops  = &fat_fops;
    r->ops   = &fat_iops;
    return;
  }
  struct direntry *d = get_dirent(&loc);

  // printkf("d: %p\n", (char *)d - 0x7c00);

  struct fat_inode_info *inf = malloc(sizeof(struct fat_inode_info));
  inf->attr                  = d->fatt;
  inf->first_clust           = d->low_cluster;
  memcpy(&inf->loc, &loc, sizeof(loc));
  inf->siz = d->size;

  r->fops  = &fat_fops;
  r->ops   = &fat_iops;
  r->mode  = get_mode(d->fatt);
  r->pdata = inf;
  r->size = d->size;
}

void fat_put_inode(struct inode *in) {
  free(((struct fat_inode_info *)in->pdata));
}

struct inode_ops fat_iops = {
    .opendir  = opendir_fat,
    .closedir = closedir_fat,
    .lookup   = lookup_fat,
    .unlink   = unlink_fat};

struct file_ops fat_fops = {
    .open  = 0,
    .close = 0,
    .read  = read_fat,
    .write = write_fat};

struct super_ops fat_sops = {
    .read_inode = fat_inoder,
    .put_inode  = fat_put_inode};

void set_sspecial() {
  init_devs();

  fsino_t dev_fs = fat_lookup_from("dev", &rootnode);
  // printkf("devfs ino: %d\n", dev_fs);
  struct inode *in = iget(&g_fat_sb, dev_fs);
  imount(in, &devmnt);
}

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

  //if(off + count > in->size) return 0;
  size_t siz = in->size;
  uint16_t cluster = ((struct fat_inode_info *)in->pdata)->first_clust;
  size_t   read    = 0;

  cluster = off > 1024 ? fattab_read(cluster) : cluster;
  off     = off % 1024;

  while (cluster < 0xff8) {
    void *addr = get_addr(cluster);

    size_t dcount = MIN(siz, 1024) - off;
    //dcount        = dcount > count ? count : dcount;
    dcount = MIN(dcount, count);
    //dcount = MIN(dcount, siz);
    // printkf("dcount: %d\n", dcount);
    //printkf("%c\n", *(char*)((char *)addr + off));
    memcpy(buf, addr + off, dcount);

    if (dcount > count) {
      printkf("fat read: overflow!\n");
      break;
    }

    count -= dcount;
    siz -= dcount;
    read += dcount;

    off     = 0;
    cluster = fattab_read(cluster);
  }

  return read;
}