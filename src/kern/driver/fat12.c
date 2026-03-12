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

  uint16_t tmp = *(uint16_t *)&fat[ent_offset];

  if (cluster & 1) {
    tmp |= next << 4;
  } else {
    tmp |= next;
  }

  *(uint16_t *)&fat[ent_offset] = tmp;
  *(uint16_t *)&fat[ent_offset + 1024] = tmp;
}

size_t trim_len(const char *p, size_t n) {
  while((n > 0) && (p[n-1] == ' ')) n--;
  return n;
}

int fatcmp(const char *field, size_t siz, const char *str) {
  size_t len = trim_len(field, siz);
  size_t slen = kstrlen(str);

  if(len != slen) return len - slen;
  return kmemcmp(field, str, len);
}

int fat_lookup(const char *path, struct inode *inode) {
  // todo: proper directory recursion thing AND long filenames
  struct direntry *diraddr = (struct direntry*)fsinfo.root_addr;

  char buffer[32];
  kstrncpy(path, buffer, 32);
    
  char *fname;
  char *ext;

  fname = kstrtok(buffer, ".");
  ext = kstrtok(NULL, ".");
  uint8_t found = 0;

  for(int i = 0; i < BS->roots; i++, diraddr++) {
    if(!fatcmp(diraddr->fname, 8, fname)) {
      if(fatcmp(diraddr->ext, 3, ext)) {
        continue;
      };
      inode->cluster0 = diraddr->low_cluster;
      inode->size = diraddr->size;
      inode->type = INODE_FILE;
      found++;
    }
  }

  return !found; // not found = 1 (err), otherwise found = 0 (ok [thumbs up emoji here])
}

void write_file(const char *fname, uint8_t *data, uint16_t siz) {
  struct direntry *diraddr =
      (struct direntry *)(fsinfo.root_start * 512 + 0x7c00);

  for (; diraddr->fname[0]; diraddr++); // let's find the next available direntry
  
  char buffer[32]; // kstrtok demands mutable char *
  kstrncpy(fname, buffer, 32);
    
  char fname_pad[8];
  char ext[3];
  kmemset(fname_pad, ' ', 8);
  kmemset(ext, ' ', 3);

  char *tmp = kstrtok(buffer, ".");
  kstrncpy(tmp, fname_pad, kstrlen(tmp));
  tmp = kstrtok(NULL, ".");
  kstrncpy(tmp, ext, kstrlen(tmp));

  kstrncpy(fname_pad, diraddr->fname, 8);
  kstrncpy(ext, diraddr->ext, 3);

  diraddr->fatt = 0x00;
  diraddr->high_cluster = 0x00;
  diraddr->size = siz;

  uint16_t cur_clust =3; // we skip 0 and 1 because its somehow reserved, also, =3
  for (; fat_value(cur_clust) != 0; cur_clust++)
    ;
  diraddr->low_cluster = cur_clust;

  uint16_t cluster_stop = ((siz) / 512) + cur_clust;
  for (; cur_clust < cluster_stop; cur_clust++) {
    write_fat(cur_clust, cur_clust + 1); // GOODBYE, FRAGMENTATION!
  }
  write_fat(cur_clust, 0xfff);

  uint8_t *waddr = (uint8_t *)((diraddr->low_cluster - 2) * BS->spc * 512 +
                               (fsinfo.data_start * 512 + 0x7c00));
  kmemcpy(data, waddr, siz);
}

uint16_t read_file(struct inode *inode, uint8_t *data, uint16_t req_siz) {
  uint16_t min_size = req_siz < inode->size ? req_siz : inode->size;

  uint16_t cluster = inode->cluster0;
  uint16_t read = 0;

  while((cluster != 0xfff) && (read < min_size)) {
    uint16_t next = fat_value(cluster);
    
    uint16_t readmax = ((min_size - read) > 1024) ? 1024 : (min_size - read);

    size_t addr = (cluster-2)*1024 + fsinfo.data_addr;
    kmemcpy((uint8_t*)addr, data+read, readmax);
    read += readmax;

    cluster = next;
  }

  return min_size;
}

uint16_t write_file2(struct inode *inode, const uint8_t *data, uint16_t req_siz) {

  uint16_t cluster = inode->cluster0;
  uint16_t clusters = inode->size / 1024;
  uint16_t req_clusters = req_siz / 1024;

  int16_t diff = req_clusters - clusters;
  if(diff <= 0) {
    uint16_t write = 0;
    while((cluster != 0xfff) && (write < req_siz)) {
      uint16_t next = fat_value(cluster);
      
      uint16_t writemax = ((req_siz - write) > 1024) ? 1024 : (req_siz - write);

      size_t addr = (cluster-2)*1024 + fsinfo.data_addr;
      kmemcpy(data+write, (uint8_t*)addr, writemax);
      write += writemax;

      cluster = next;
    }
    return write;
  }
  // here we need to expand tha fat entry, i'll just do it later its midnight here!
  printkf("req clusters: %d\n", req_clusters);

  return 0;
}

int sys_open(const char *fname, uint16_t flags) {
  int fd = 1;
  for(; global_fd[fd].flags & F_USED ;fd++); // find available fd, 0 is reserved for errors

  struct inode *in = kmalloc(sizeof(struct inode));
  if(fat_lookup(fname, in)) {
    printkf("err: '%s': no such file\n", fname);
    stupidfree(sizeof(struct inode));
    return 0;
  }

  // going here means the file exists

  global_fd[fd].inode = in;
  global_fd[fd].flags = F_USED;

  return fd;
} 

size_t sys_read(int fd, uint8_t *buf, size_t n) {
  struct file f = global_fd[fd];
  if(!(f.flags & F_USED)) {
    printkf("invalid fd %d :(\n", fd);
    return 0;
  }

  struct inode *in = f.inode;
  uint16_t read = 0;
  if(in->type == INODE_FILE) {
    read = read_file(in, buf, n);
    f.position += read;
  }
  return read;
}

size_t sys_write(int fd, const uint8_t *buf, size_t n) {
  struct file f = global_fd[fd];
  if(!(f.flags & F_USED)) {
    printkf("invalid fd %d :(\n", fd);
    return 0;
  }

  struct inode *in = f.inode;
  uint16_t write = 0;
  if(in->type == INODE_FILE) {
    write = write_file2(in, buf, n);
  }
  return write;
}