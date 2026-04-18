#ifndef VIRTIO_H
#define VIRTIO_H

#include "stdint.h"

#define VIO_REG_DEVFEAT 0x00
#define VIO_REG_GESFEAT 0x04
#define VIO_REG_QUEADDR 0x08
#define VIO_REG_QUESIZE 0x0c
#define VIO_REG_QUESLCT 0x0e
#define VIO_REG_QUENOTF 0x10
#define VIO_REG_DEVSTAT 0x12
#define VIO_REG_ISRSTAT 0x13

// network virtio
#define VIO_REG_MAC  0x14
#define VIO_REG_STAT 0x1a

// block virtio
#define VIO_REG_SECTS     0x14
#define VIO_REG_MAXSIZ    0x1c
#define VIO_REG_MAXCOUNT  0x20
#define VIO_REG_CYLCOUNT  0x24
#define VIO_REG_HEADCOUNT 0x26
#define VIO_REG_SECTCOUNT 0x27
#define VIO_REG_BLOCKLEN  0x28

// bits for DEVFEAT
#define DFT_DEVACK  0x01
#define DFT_DRVLOAD 0x02
#define DFT_DRVRDY  0x04
#define DFT_DEVERR  0x40
#define DFT_DRVFAIL 0x80

struct viobuf {
  uint64_t addr;
  uint32_t len;
  uint16_t flg;
  uint16_t next;
} __attribute__((packed));

struct vionetpack {
  uint8_t flg;
  uint8_t seg_off;
  uint16_t headlen;
  uint16_t seglen;
  uint16_t chk_start;
  uint16_t chk_off;
  uint16_t bufcont;
} __attribute__((packed));

struct vioblkreq {
  uint32_t type;
  uint32_t res;
  uint64_t sect;
  uint8_t data[];
  // uint8_t status
} __attribute__((packed));

#endif