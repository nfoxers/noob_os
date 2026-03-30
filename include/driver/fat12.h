#ifndef FAT12_H
#define FAT12_H

#include "mem/mem.h"
#include "stdint.h"

struct bootsect {
  uint8_t  jmp[3];
  char     oem[8];
  uint16_t bps;
  uint8_t  spc;
  uint16_t sec_reserved;
  uint8_t  fats;
  uint16_t roots;
  uint16_t total_sec;
  uint8_t  mdt;
  uint16_t spf;
  uint16_t spt;
  uint16_t heads;
  uint32_t hidden;
  uint32_t lsc;

  uint8_t  drive_num;
  uint8_t  res;
  uint8_t  sig;
  uint32_t volid;
  char     vollab[11];
  char     sysid[8];
  uint8_t  boot[448];
  uint16_t bootsig;
} __attribute__((packed));

void init_fs();

void list_dir(const char *path);

#define O_CREAT                0x0001
#define O_JUSTGIVEMETHEADDRESS 0x0002

int    fsys_open(const char *fname, uint16_t flags);
size_t fsys_read(int fd, uint8_t *buf, size_t n);
size_t fsys_write(int fd, const uint8_t *buf, size_t n);
int    fsys_unlink(const char *path);

int fsys_mkdir(const char *path);
int fsys_close(int fd);
int fsys_cd(const char *path);

#endif