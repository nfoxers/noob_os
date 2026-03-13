#include "driver/fat12.h"
#include "mem/mem.h"
#include "video/printf.h"
#include "video/video.h"
#include <stdint.h>

#define BS ((struct bootsect *)0x7c00)
#define MAX_FD 16

struct fat12info {
  uint16_t fat_start;
  uint16_t fat_addr;
  uint16_t fat_sectors;

  uint16_t root_start;
  struct direntry *root_addr;
  uint16_t root_sectors;

  uint16_t data_start;
  uint16_t data_addr;
  uint16_t data_sectors;
};

struct fat12info fsinfo = {0};

struct file global_fd[MAX_FD] = {0};

void init_fs() {
  if (BS->bootsig != 0xaa55) {
    printk("mismatching boot signatures, either corrupt memory or idk\n");
    return;
  }

  fsinfo.fat_sectors = BS->fats * BS->spf;
  fsinfo.fat_addr = fsinfo.fat_sectors * 512 + 0x7c00;
  fsinfo.fat_start = BS->sec_reserved;

  fsinfo.root_start = fsinfo.fat_sectors + fsinfo.fat_start;
  fsinfo.root_addr = (struct direntry *)(fsinfo.root_start * 512 + 0x7c00);
  fsinfo.root_sectors = (32 * BS->roots + BS->bps - 1) / BS->bps;

  fsinfo.data_start = fsinfo.root_start + fsinfo.root_sectors;
  fsinfo.data_addr = fsinfo.data_start * 512 + 0x7c00;
  fsinfo.data_sectors = BS->total_sec - fsinfo.data_start;

  printk("fs init ok\n");
}

uint16_t fat_value(uint16_t cluster) {
  uint8_t *fat = (uint8_t *)(fsinfo.fat_start * 512 + 0x7c00);
  uint16_t fat_offset = cluster + (cluster / 2);
  uint16_t ent_offset = fat_offset % 1024;

  uint16_t val = *(uint16_t *)&fat[ent_offset];

  val = (cluster & 1) ? val >> 4 : val & 0xfff;

  return val;
}

void write_fat(uint16_t cluster, uint16_t next) {
  uint8_t *fat = (uint8_t *)(fsinfo.fat_start * 512 + 0x7c00);
  uint16_t fat_offset = cluster + (cluster / 2);
  uint16_t ent_offset = fat_offset % 1024;

  next &= 0x0FFF;

  uint16_t tmp = *(uint16_t *)&fat[ent_offset];

  if (cluster & 1) {
    tmp = (tmp & 0x000F) | (next << 4);
    } else {
    tmp = (tmp & 0xF000) | next;
  }

  *(uint16_t *)&fat[ent_offset] = tmp;
  *(uint16_t *)&fat[ent_offset + 1024] = tmp;
}

int fat_name_match(struct direntry *e, const char *name) {
  char fatname[13];
  char fname[9];
  char ext[4];

  snprintkf(fname, sizeof(fname), "%.8s", e->fname);
  snprintkf(ext, sizeof(ext), "%.3s", e->ext);

  for(int i = 7; i >= 0 && fname[i] == ' '; i--)
    fname[i] = 0;

  for(int i = 2; i >= 0 && ext[i] == ' '; i--)
    ext[i] = 0;

  if(ext[0])
    snprintkf(fatname, sizeof(fatname), "%s.%s", fname, ext);
  else
    snprintkf(fatname, sizeof(fatname), "%s", fname);

  return !kstrcmp(fatname, name);
}

int fat_lookup(const char *path, struct inode *inode) {
  // todo: long filanames
  char pathbuf[32];
  kstrncpy(pathbuf, path, sizeof(pathbuf));

  char *component = kstrtok(pathbuf, "/");

  struct direntry *dir;
  uint16_t cluster = 0; // root
  int entries = BS->roots;

  while(component) {
    int found = 0;
    if(cluster == 0) {
      dir = (struct direntry*)fsinfo.root_addr;
      entries = BS->roots;
    } else {
      dir = (struct direntry*)((cluster-2)*1024 + fsinfo.data_addr);
      entries = 1024 / sizeof(struct direntry);
    }

    for(int i = 0; i < entries; i++) {
      if(dir[i].fname[0] == 0x00)
        break;
      if((uint8_t)dir[i].fname[0] == 0xE5)
        continue;
      if(fat_name_match(&dir[i], component)) {
        found = 1;
        if(dir[i].fatt & FAT_SUBDIR) {
          cluster = dir[i].low_cluster;
          inode->type = INODE_DIR;
        } else {
          inode->type = INODE_FILE;
        }

        inode->cluster0 = dir[i].low_cluster;
        inode->entaddr = &dir[i];
        inode->size = dir[i].size;

        break;
      }
    }

    if(!found)
      return 1;

    component = kstrtok(NULL, "/");
  }

  return 0;
}

uint16_t read_file(struct inode *inode, uint8_t *data, uint16_t req_siz, size_t offset) {
  if (offset >= inode->size)
    return 0;

  uint16_t to_read = MIN(req_siz, inode->size - offset);

  uint16_t cluster = inode->cluster0;

  for (size_t i = 0; i < offset / 1024; i++)
    cluster = fat_value(cluster);

  size_t cluster_off = offset % 1024;
  uint16_t read = 0;

  while ((cluster < 0xFF8) && (read < to_read)) {

    size_t addr = (cluster - 2) * 1024 + fsinfo.data_addr;

    uint16_t chunk = MIN(1024 - cluster_off, to_read - read);

    kmemcpy(data + read, (uint8_t*)(addr + cluster_off), chunk);

    read += chunk;
    cluster_off = 0;
    cluster = fat_value(cluster);
  }

  return read;
}

uint16_t last_cluster(uint16_t cur) {
  for(; fat_value(cur) < 0xff8; cur = fat_value(cur)) ;
  return cur;
}

uint16_t find_free_cluster() {
  uint16_t i = 2;
  while(fat_value(i++) != 0);
  return i-1;
}

uint16_t write_file(struct inode *inode, const uint8_t *data, size_t req_siz, size_t offset) {

  uint16_t cluster = inode->cluster0;
  for (size_t i = 0; i < offset / 1024; i++)
    cluster = fat_value(cluster);
  
  uint16_t cluster_end = last_cluster(cluster);
  size_t cluster_off = offset % 1024;

  uint16_t clusters = inode->size / 1024;
  uint16_t req_clusters = req_siz / 1024;

  int16_t diff = req_clusters - clusters;
  if(diff <= 0) {
    uint16_t write = 0;
    while((cluster < 0xff8) && (write < req_siz)) {
      uint16_t next = fat_value(cluster);
      
      uint16_t writemax = MIN(1024 - cluster_off, req_siz - write);

      size_t addr = (cluster-2)*1024 + fsinfo.data_addr + cluster_off;
      kmemcpy((uint8_t*)addr, data+write, writemax); 
      write += writemax;

      cluster = next;
      cluster_off = 0;
    }
    return write;
  }

  uint16_t prev = cluster_end;

  while(diff--) {
    uint16_t new = find_free_cluster();

    write_fat(prev, new);
    write_fat(new, 0xfff);
    
    prev = new;
  }

  uint16_t write = 0;
  while((cluster < 0xff8) && (write < req_siz)) {
    uint16_t next = fat_value(cluster);
      
    uint16_t writemax = MIN(1024 - cluster_off, req_siz - write);

    size_t addr = (cluster-2)*1024 + fsinfo.data_addr + cluster_off;
    kmemcpy((uint8_t*)addr, data+write, writemax);
    write += writemax;

    cluster = next;
    cluster_off = 0;
  }

  struct direntry *d = inode->entaddr;

  d->size = req_siz + offset;

  return write;
}

void split_path(const char *path, char *pr, char *n) {
  char pathbuf[32];
  kstrncpy(pathbuf, path, sizeof(pathbuf));

  char *last = kstrrchr(pathbuf, '/');

  char *parent;
  char *name;

  if(last) {
      *last = 0;
      parent = pathbuf;
      name = last + 1;
  } else {
      parent = "";
      name = pathbuf;
  }

  kstrcpy(pr, parent);
  kstrcpy(n, name);
}

void fat_format_name(const char *name, char out[11]) {
    char fname[9] = {0};
    char ext[4] = {0};

    char tmp[16];
    kstrncpy(tmp, name, sizeof(tmp));

    char *f = kstrtok(tmp, ".");
    char *e = kstrtok(NULL, ".");

    if(f) snprintkf(fname, sizeof(fname), "%-8s", f);
    if(e) snprintkf(ext, sizeof(ext), "%-3s", e);

    kmemcpy(out, fname, 8);
    kmemcpy(out + 8, ext, 3);
}

void list_dir() {
  struct direntry *de = fsinfo.root_addr;
  for(; de - fsinfo.root_addr < BS->roots*32; de++) {
    if(de->fname[0] == '\0') break;
    if(de->fname[0] == 0x0f) continue;
    if((uint8_t)de->fname[0] == 0xe5) continue;
    printkf("% 8.8s % 3.3s", de->fname, de->ext);
    if(de->fatt & FAT_SUBDIR) printk(" <DIR> ");
    else printk("       ");
    printkf("%d\n", de->size);
  }
}

void fat_find_free(struct inode *in, struct direntry **d) {
  if(in->type != INODE_DIR) return;
  
  struct direntry *a;
  if(!(in->cluster0)) a = fsinfo.root_addr;
  else a = (struct direntry *)((in->cluster0-2) * 1024 + fsinfo.data_addr);
  while(a->fname[0]) a++;
  *d = a;
}

int fat_create(const char *path, struct inode *inode) {

  char parent[128];
  char name[32];

  split_path(path, parent, name);

  struct inode dir = {0};

  if(parent[0]) {
    if(fat_lookup(parent, &dir))
      return 0;

    if(dir.type != INODE_DIR)
      return 0;
  } else {
      dir.cluster0 = 0; // root
      dir.type = INODE_DIR;
  }

  struct direntry *slot = NULL;
  fat_find_free(&dir, &slot);
  if(!slot) {
    printk("fail\n");return 0;
  }

  char fatname[11];
  fat_format_name(name, fatname);

  kmemcpy(slot->fname, fatname, 8);
  kmemcpy(slot->ext, fatname + 8, 3);

  slot->fatt = 0;
  slot->size = 0;
  slot->low_cluster = find_free_cluster();

  inode->cluster0 = slot->low_cluster;
  inode->size = 0;
  inode->type = INODE_FILE;
  inode->entaddr = slot;

  write_fat(slot->low_cluster, 0xfff);

  return 0;
}

int sys_unlink(const char *path) {
  struct inode in = {0};
  if(fat_lookup(path, &in)) {
    printkf("unlink: file '%s' does not exist\n", path);
    return 1;
  }

  in.entaddr->fname[0] = 0xe5;

  return 0;
}

int sys_open(const char *fname, uint16_t flags) {
  int fd = 1;
  for(; global_fd[fd].flags & F_USED ;fd++); // find available fd, 0 is reserved for errors

  struct inode *in = kmalloc(sizeof(struct inode));
  if(fat_lookup(fname, in) && !(flags & O_CREAT)) {
    printkf("err: '%s': no such file\n", fname);
    stupidfree(sizeof(struct inode));
    return 0;
  } else if(flags & O_CREAT) {
    fat_create(fname, in);
  }

  // going here means the file exists

  global_fd[fd].inode = in;
  global_fd[fd].flags = F_USED;

  return fd;
} 

size_t sys_read(int fd, uint8_t *buf, size_t n) {
  struct file *f = &global_fd[fd];
  if(!(f->flags & F_USED)) {
    printkf("invalid fd %d :(\n", fd);
    return 0;
  }

  struct inode *in = f->inode;
  uint16_t read = 0;
  if(in->type == INODE_FILE) {
    read = read_file(in, buf, n, f->position);
    f->position += read;
  }
  return read;
}

size_t sys_write(int fd, const uint8_t *buf, size_t n) {
  struct file *f = &global_fd[fd];
  if(!(f->flags & F_USED)) {
    printkf("invalid fd %d :(\n", fd);
    return 0;
  }

  struct inode *in = f->inode;
  uint16_t write = 0;
  if(in->type == INODE_FILE) {
    printkf("fp: %d\n", f->position);
    write = write_file(in, buf, n, f->position);
    printkf("w: %d\n", write);
    f->position += write;
  }
  return write;
}