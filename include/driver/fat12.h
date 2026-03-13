#ifndef FAT12_H
#define FAT12_H

#include "stdint.h"
#include "mem/mem.h"

struct bootsect {
  uint8_t jmp[3];
  char oem[8];
  uint16_t bps;
  uint8_t spc;
  uint16_t sec_reserved;
  uint8_t fats;
  uint16_t roots;
  uint16_t total_sec;
  uint8_t mdt;
  uint16_t spf;
  uint16_t spt;
  uint16_t heads;
  uint32_t hidden;
  uint32_t lsc;

  uint8_t drive_num;
  uint8_t res;
  uint8_t sig;
  uint32_t volid;
  char vollab[11];
  char sysid[8];
  uint8_t boot[448];
  uint16_t bootsig;
} __attribute__((packed));

#define FAT_RDONLY 0x01
#define FAT_HIDDEN 0x02
#define FAT_SYSTEM 0x04
#define FAT_VOLLAB 0x08
#define FAT_SUBDIR 0x10
#define FAT_ARCHIV 0x20
#define FAT_DEVICE 0x40
#define FAT_RESERV 0x80

struct direntry {
  char fname[8];
  char ext[3];
  uint8_t fatt;
  uint8_t reserved;
  uint8_t create_cs;
  uint16_t create_time;
  uint16_t create_date;
  uint16_t la_date;
  uint16_t high_cluster;
  uint16_t lm_time;
  uint16_t lm_date;
  uint16_t low_cluster;
  uint32_t size;
} __attribute__((packed));

#define INODE_FILE 0x0001
#define INODE_DIR  0x0002
#define INODE_STDSTREAM 0x0003

#define F_RDONLY 0x0001
#define F_WRONLY 0x0002 // i'll just implement them later man
#define F_USED 0x0004

struct inode {
  uint16_t size;
  uint16_t cluster0;
  struct direntry *entaddr;
  uint16_t type;
};

struct file {
  struct inode *inode;
  uint16_t position;
  uint16_t flags;
};

void init_fs();

void list_dir();
int sys_unlink(const char *path);

#define O_CREAT 0x0001

int sys_open(const char *fname, uint16_t flags);
size_t sys_read(int fd, uint8_t *buf, size_t n);
size_t sys_write(int fd, const uint8_t *buf, size_t n);

#endif