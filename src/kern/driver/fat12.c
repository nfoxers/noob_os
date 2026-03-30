#include "driver/fat12.h"
#include "driver/time.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "video/printf.h"
#include "video/video.h"
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

struct file root_fd[NOFILE] = {0};

extern struct inode root_dir;
extern struct proc *volatile p_curproc;

void init_fs() {
  print_init("fat", "initializing filesystem driver...", 0);
  
  if (BS->bootsig != 0xaa55) {
    printk("mismatching boot signatures, either corrupt memory or idk\n");
    return;
  }

  fsinfo.fat_sectors = BS->fats * BS->spf;
  fsinfo.fat_addr    = fsinfo.fat_sectors * 512 + 0x7c00;
  fsinfo.fat_start   = BS->sec_reserved;

  fsinfo.root_start   = fsinfo.fat_sectors + fsinfo.fat_start;
  fsinfo.root_addr    = (struct direntry *)(fsinfo.root_start * 512 + 0x7c00);
  fsinfo.root_sectors = (32 * BS->roots + BS->bps - 1) / BS->bps;

  fsinfo.data_start   = fsinfo.root_start + fsinfo.root_sectors;
  fsinfo.data_addr    = fsinfo.data_start * 512 + 0x7c00;
  fsinfo.data_sectors = BS->total_sec - fsinfo.data_start;

  root_dir.type = INODE_DIR;
  root_dir.cluster0 = 0;
  root_dir.entaddr = fsinfo.root_addr;
  root_dir.size = 0;
  root_dir.permission = 0766;

  kmemcpy(&p_curproc->p_user->u_cdir, &root_dir, sizeof(struct inode));
}

uint16_t fat_value(uint16_t cluster) {
  uint8_t *fat        = (uint8_t *)(fsinfo.fat_start * 512 + 0x7c00);
  uint16_t fat_offset = cluster + (cluster / 2);
  uint16_t ent_offset = fat_offset % 1024;

  uint16_t val = *(uint16_t *)&fat[ent_offset];

  val = (cluster & 1) ? val >> 4 : val & 0xfff;

  return val;
}

void write_fat(uint16_t cluster, uint16_t next) {
  uint8_t *fat        = (uint8_t *)(fsinfo.fat_start * 512 + 0x7c00);
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

  return !kstrcmp(fatname, name);
}

void set_perms(struct inode *in, uint8_t fatt) {
  in->permission = 0766; // x only for the user
  if (fatt & FAT_RDONLY)
    in->permission &= ~(PRM_W * (PRM_USR | PRM_GRP | PRM_OTH));
}

int fat_lookup(const char *restrict path, struct inode *restrict inode) {
  // todo: long filanames
  char pathbuf[32];
  kstrncpy(pathbuf, path, sizeof(pathbuf));
  uint8_t root = 0;

  if(pathbuf[0] == '/' && pathbuf[1] == 0) {
    inode->cluster0   = 0;
    inode->entaddr    = root_dir.entaddr;
    inode->size       = 0;
    inode->type       = INODE_DIR;
    inode->permission = root_dir.permission;
    return 0;
  }

  if(pathbuf[0] == '/') {
    root = 1;
  }

  char *component = kstrtok(pathbuf, "/");

  struct direntry *dir;

  uint16_t cluster = 0; // root
  int      entries = BS->roots;
  int      found   = 0;

  if(!root) {
    cluster = p_curproc->p_user->u_cdir.cluster0;
    if(!component) {
      inode->cluster0   = p_curproc->p_user->u_cdir.cluster0;
      inode->entaddr    = p_curproc->p_user->u_cdir.entaddr;
      inode->size       = 0;
      inode->type       = INODE_DIR;
      inode->permission = p_curproc->p_user->u_cdir.permission;
      return 0;
    }
  }

  while (component) {
    found = 0;
    if (cluster == 0) {
      dir     = (struct direntry *)fsinfo.root_addr;
      entries = BS->roots;
    } else {
      dir     = (struct direntry *)((cluster - 2) * 1024 + fsinfo.data_addr);
      entries = 1024 / sizeof(struct direntry);
    }

    for (int i = 0; i < entries; i++) {
      if (dir[i].fname[0] == 0x00)
        break;
      if ((uint8_t)dir[i].fname[0] == 0xE5)
        continue;
      if (fat_name_match(&dir[i], component)) {
        found = 1;
        if (dir[i].fatt & FAT_SUBDIR) {
          cluster     = dir[i].low_cluster;
          inode->type = INODE_DIR;
        } else {
          inode->type = INODE_FILE;
        }

        inode->cluster0 = dir[i].low_cluster;
        inode->entaddr  = &dir[i];
        inode->size     = dir[i].size;

        set_perms(inode, dir[i].fatt);

        break;
      }
    }

    if (!found)
      return 1;

    component = kstrtok(NULL, "/");
  }

  if (!found)
    return 1;

  return 0;
}

uint16_t read_file(struct inode *restrict inode, uint8_t *restrict data, uint16_t req_siz, size_t offset) {
  if (offset >= inode->size)
    return 0;

  uint16_t to_read = MIN(req_siz, inode->size - offset);

  uint16_t cluster = inode->cluster0;

  for (size_t i = 0; i < offset / 1024; i++)
    cluster = fat_value(cluster);

  size_t   cluster_off = offset % 1024;
  uint16_t read        = 0;

  while ((cluster < 0xFF8) && (read < to_read)) {

    size_t addr = (cluster - 2) * 1024 + fsinfo.data_addr;

    uint16_t chunk = MIN(1024 - cluster_off, to_read - read);

    kmemcpy(data + read, (uint8_t *)(addr + cluster_off), chunk);

    read += chunk;
    cluster_off = 0;
    cluster     = fat_value(cluster);
  }

  return read;
}

uint16_t last_cluster(uint16_t cur) {
  for (; fat_value(cur) < 0xff8; cur = fat_value(cur))
    ;
  return cur;
}

uint16_t find_free_cluster() {
  uint16_t i = 2;
  while (fat_value(i++) != 0)
    ;
  return i - 1;
}

uint16_t write_file(struct inode *restrict inode, const uint8_t *restrict data, size_t req_siz, size_t offset) {

  uint16_t cluster = inode->cluster0;
  for (size_t i = 0; i < offset / 1024; i++)
    cluster = fat_value(cluster);

  uint16_t cluster_end = last_cluster(cluster);
  size_t   cluster_off = offset % 1024;

  uint16_t clusters     = inode->size / 1024;
  uint16_t req_clusters = req_siz / 1024;

  int16_t diff = req_clusters - clusters;
  if (diff <= 0) {
    uint16_t write = 0;
    while ((cluster < 0xff8) && (write < req_siz)) {
      uint16_t next = fat_value(cluster);

      uint16_t writemax = MIN(1024 - cluster_off, req_siz - write);

      size_t addr = (cluster - 2) * 1024 + fsinfo.data_addr + cluster_off;
      kmemcpy((uint8_t *)addr, data + write, writemax);
      write += writemax;

      cluster     = next;
      cluster_off = 0;
    }
    return write;
  }

  uint16_t prev = cluster_end;

  while (diff--) {
    uint16_t new = find_free_cluster();

    write_fat(prev, new);
    write_fat(new, 0xfff);

    prev = new;
  }

  uint16_t write = 0;
  while ((cluster < 0xff8) && (write < req_siz)) {
    uint16_t next = fat_value(cluster);

    uint16_t writemax = MIN(1024 - cluster_off, req_siz - write);

    size_t addr = (cluster - 2) * 1024 + fsinfo.data_addr + cluster_off;
    kmemcpy((uint8_t *)addr, data + write, writemax);
    write += writemax;

    cluster     = next;
    cluster_off = 0;
  }

  struct direntry *d = inode->entaddr;

  d->size = req_siz + offset;

  return write;
}

void split_path(const char *restrict path, char *restrict pr, char *restrict n) {
  char pathbuf[32];
  kstrncpy(pathbuf, path, sizeof(pathbuf));

  char *last = kstrrchr(pathbuf, '/');

  char *parent;
  char *name;

  if (last) {
    *last  = 0;
    parent = pathbuf;
    name   = last + 1;
  } else {
    parent = "";
    name   = pathbuf;
  }

  kstrcpy(pr, parent);
  kstrcpy(n, name);
}

void fat_format_name(const char *restrict name, char out[restrict 11]) {
  char fname[9] = {0};
  char ext[4]   = {0};

  char tmp[16];
  kstrncpy(tmp, name, sizeof(tmp));

  char *f = kstrtok(tmp, ".");
  char *e = kstrtok(NULL, ".");

  if (f)
    snprintkf(fname, sizeof(fname), "%-8s", f);
  if (e)
    snprintkf(ext, sizeof(ext), "%-3s", e);

  kmemcpy(out, fname, 8);
  kmemcpy(out + 8, ext, 3);
}

void *get_addr(uint16_t lc) {
  return (void *)(fsinfo.data_addr + (lc - 2) * 1024);
}

void list_dir(const char *path) {
  struct inode in = {0};
  if (fat_lookup(path, &in)) {
    printkf("%s doesnt exist\n", path);
    return;
  } else if (in.type == INODE_FILE) {
    printkf("%s is not a dir\n", path);
  }

  struct direntry *de;
  struct direntry *start;

  if (in.cluster0 == 0)
    de = fsinfo.root_addr;
  else {
    de = (struct direntry *)get_addr(in.entaddr->low_cluster);
  }

  start = de;
  for (; (de - start) < 32; de++) {
    if (de->fname[0] == '\0')
      break;
    if (de->fname[0] == 0x0f)
      continue;
    if ((uint8_t)de->fname[0] == 0xe5)
      continue;
    printkf("% 8.8s % 3.3s", de->fname, de->ext);
    if (de->fatt & FAT_SUBDIR)
      printk(" <DIR> ");
    else
      printk("       ");
    printkf("%d\n", de->size);
  }
}

void fat_find_free(struct inode *restrict in, struct direntry **restrict d) {
  if (in->type != INODE_DIR)
    return;

  struct direntry *a;
  if (!(in->cluster0))
    a = fsinfo.root_addr;
  else
    a = (struct direntry *)((in->cluster0 - 2) * 1024 + fsinfo.data_addr);
  while (a->fname[0])
    a++;
  *d = a;
}

uint16_t get_tim_packed(uint16_t *date) {
  struct time_s time;
  read_time(&time);

  uint16_t tim = 0;

  tim |= time.sec / 2 & 0b11111;
  tim |= (time.min & 0b111111) << 5;
  tim |= (time.hour & 0b11111) << 11;

  if (date) {
    *date = 0;
    *date |= time.day & 0b11111;
    *date |= (time.mon & 0b1111) << 5;
    *date |= ((time.year + 20) & 0b1111111) << 9;
  }

  return tim;
}

int fat_create(const char *restrict path, struct inode *restrict inode) {
  char parent[128];
  char name[32];

  split_path(path, parent, name);

  struct inode dir = {0};

  if (parent[0]) {
    if (fat_lookup(parent, &dir))
      return 0;

    if (dir.type != INODE_DIR)
      return 0;
  } else {
    dir.cluster0 = p_curproc->p_user->u_cdir.cluster0; // cur_dir
    dir.type     = INODE_DIR;
  }

  struct direntry *slot = NULL;
  fat_find_free(&dir, &slot);
  if (!slot) {
    printk("fail\n");
    return 0;
  }

  char fatname[11];
  fat_format_name(name, fatname);

  kmemcpy(slot->fname, fatname, 8);
  kmemcpy(slot->ext, fatname + 8, 3);

  slot->fatt        = 0;
  slot->size        = 0;
  slot->low_cluster = find_free_cluster();

  uint16_t date     = 0;
  slot->create_time = get_tim_packed(&date);
  slot->lm_time     = slot->create_time;
  slot->create_date = date;
  slot->lm_date     = date;
  slot->la_date     = date;

  inode->cluster0 = slot->low_cluster;
  inode->size     = 0;
  inode->type     = INODE_FILE;
  inode->entaddr  = slot;

  write_fat(slot->low_cluster, 0xfff);

  return 0;
}

void create_dots(struct direntry *dir, uint16_t lcp) { // dir = direntry in parent
  struct direntry *d = (struct direntry *)get_addr(dir->low_cluster);

  snprintkf(d->fname, 13, "%-13c", '.');
  d->low_cluster = dir->low_cluster;
  d->fatt        = FAT_SUBDIR;
  d->create_time = d->lm_time = dir->lm_time;
  d->create_date = d->lm_date = d->la_date = dir->la_date;

  d++;

  snprintkf(d->fname, 13, "%-13s", "..");
  d->low_cluster = lcp;
  d->fatt        = FAT_SUBDIR;
  d->create_time = d->lm_time = dir->lm_time;
  d->create_date = d->lm_date = d->la_date = dir->la_date;
}

int fat_mkdir(const char *restrict path, struct inode *restrict inode) {
  char parent[128];
  char name[32];

  split_path(path, parent, name);

  struct inode dir = {0};

  if (parent[0]) {
    if (fat_lookup(parent, &dir))
      return 0;

    if (dir.type != INODE_DIR)
      return 0;
  } else {
    dir.cluster0 = p_curproc->p_user->u_cdir.cluster0; // cur_dir
    dir.type     = INODE_DIR;
  }

  struct direntry *slot = NULL;
  fat_find_free(&dir, &slot);
  if (!slot) {
    printk("fail\n");
    return 0;
  }

  snprintkf(slot->fname, 13, "%-13.13s", name);

  slot->fatt        = FAT_SUBDIR;
  slot->size        = 0;
  slot->low_cluster = find_free_cluster();

  uint16_t date     = 0;
  slot->create_time = get_tim_packed(&date);
  slot->lm_time     = slot->create_time;
  slot->create_date = date;
  slot->lm_date     = date;
  slot->la_date     = date;
  create_dots(slot, dir.cluster0);

  inode->cluster0 = slot->low_cluster;
  inode->size     = 0;
  inode->type     = INODE_DIR;
  inode->entaddr  = slot;

  write_fat(slot->low_cluster, 0xfff);

  return 0;
}

/* system call abstraction */

int fsys_unlink(const char *path) {
  struct inode in = {0};
  if (fat_lookup(path, &in)) {
    printkf("unlink: file '%s' does not exist\n", path);
    return 1;
  }

  in.entaddr->fname[0] = 0xe5;

  return 0;
}

int fsys_mkdir(const char *path) {
  int fd = 1;
  for (; root_fd[fd].flags & F_USED; fd++)
    ;

  struct inode *in = kmalloc(sizeof(struct inode));
  if (!fat_lookup(path, in)) {
    printkf("err: %s already exists\n", path);
    kfree(in);
    return 0;
  }

  fat_mkdir(path, in);

  root_fd[fd].inode = in;
  root_fd[fd].flags = F_USED;
  root_fd[fd].position = 0;

  return fd;
}

int fsys_open(const char *fname, uint16_t flags) {
  int fd = 1;
  for (; root_fd[fd].flags & F_USED; fd++)
    ; // find available fd, 0 is reserved for errors

  struct inode *in = kmalloc(sizeof(struct inode));
  if (fat_lookup(fname, in) && !(flags & O_CREAT)) {
    printkf("err: '%s': no such file\n", fname);
    kfree(in);
    return 0;
  } else if (flags & O_CREAT) {
    fat_create(fname, in);
  }

  // going here means the file exists

  root_fd[fd].inode = in;
  root_fd[fd].flags = F_USED;
  root_fd[fd].position = 0;

  if(flags & O_JUSTGIVEMETHEADDRESS) {
    root_fd[fd].flags |= F_ADDR;
  }

  return fd;
}

size_t fsys_read(int fd, uint8_t *buf, size_t n) {
  struct file *f = &root_fd[fd];
  if (!(f->flags & F_USED)) {
    printkf("invalid fd %d :(\n", fd);
    return 0;
  }

  struct inode *in   = f->inode;
  uint16_t      read = 0;
  if (in->type == INODE_FILE && !(f->flags & F_ADDR)) {
    read = read_file(in, buf, n, f->position);
    f->position += read;
  } else if(in->type == INODE_FILE && f->flags & F_ADDR) {
    kmemcpy(buf + f->position, (uint8_t*)get_addr(in->cluster0) + f->position, n);
    f->position += n;
    read = n;
  }
  return read;
}

size_t fsys_write(int fd, const uint8_t *buf, size_t n) {
  struct file *f = &root_fd[fd];
  if (!(f->flags & F_USED)) {
    printkf("invalid fd %d :(\n", fd);
    return 0;
  }

  struct inode *in    = f->inode;
  uint16_t      write = 0;
  if (in->type == INODE_FILE) {
    printkf("fp: %d\n", f->position);
    write = write_file(in, buf, n, f->position);
    printkf("w: %d\n", write);
    f->position += write;
  }
  return write;
}

int fsys_close(int fd) {
  if (!(root_fd[fd].flags & F_USED)) {
    return 1;
  }

  kfree(root_fd[fd].inode);
  root_fd[fd].flags &= ~F_USED;

  return 0;
}

int fsys_cd(const char *path) {
  struct inode in = {0};
  if(fat_lookup(path, &in)) {
    printkf("%s: doesnt exit\n", path);
    return 0;
  } else if(in.type == INODE_FILE) {
    printkf("not a dir\n");
    return 0;
  }

  kmemcpy(&p_curproc->p_user->u_cdir, &in, sizeof(struct inode));
  return 0;
}