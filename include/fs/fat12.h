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

#define FAT_RDONLY 0x01
#define FAT_HIDDEN 0x02
#define FAT_SYSTEM 0x04
#define FAT_VOLLAB 0x08
#define FAT_SUBDIR 0x10
#define FAT_ARCHIV 0x20
#define FAT_DEVICE 0x40
#define FAT_RESERV 0x80

struct direntry {
  char     fname[8];
  char     ext[3];
  uint8_t  fatt;
  uint8_t  reserved;
  uint8_t  create_cs;
  uint16_t create_time;
  uint16_t create_date;
  uint16_t la_date;
  uint16_t high_cluster;
  uint16_t lm_time;
  uint16_t lm_date;
  uint16_t low_cluster;
  uint32_t size;
} __attribute__((packed));

void init_fs();

#define O_CREAT                0x0001
#define O_JUSTGIVEMETHEADDRESS 0x0002

#define ROOT_ADDR (struct direntry*)0x8600

#endif