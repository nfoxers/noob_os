#ifndef VIRTIO_H
#define VIRTIO_H

#include "stdint.h"
#include <driver/pci.h>

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

/* pci details*/

#define VIO_PCI_CAP_COMMON_CFG 1
#define VIO_PCI_CAP_NOTIFY_CFG 2
#define VIO_PCI_CAP_ISR_CFG 3
#define VIO_PCI_CAP_DEVICE_CFG 4
#define VIO_PCI_CAP_PCI_CFG 5
#define VIO_PCI_CAP_SHM_CFG 8
#define VIO_PCI_CAP_VENDOR_CFG 9

struct vio_pci_cap {
  uint8_t cap_vndr;
  uint8_t cap_next;
  uint8_t cap_len;
  uint8_t cfg_type;
  uint8_t bar;
  uint8_t id;
  uint8_t pad[2];
  uint32_t off;
  uint32_t len;
};

/* mmio details */

typedef const uint64_t ro64_t;
typedef uint64_t rw64_t;
typedef const uint32_t ro32_t;
typedef uint32_t rw32_t;
typedef const uint16_t ro16_t;
typedef uint16_t rw16_t;
typedef const uint8_t ro8_t;
typedef uint8_t rw8_t;

struct vio_pci_common_cfg {
  rw32_t device_feature_select;
  ro32_t device_feature;
  rw32_t driver_feature_select;
  rw32_t driver_feature;

  rw16_t config_msix_vector;
  ro16_t num_queues;
  rw8_t device_status;
  ro8_t config_generation;

  rw16_t queue_select;
  rw16_t queue_size;
  rw16_t queue_msix_vector;
  rw16_t queue_enable;
  ro16_t queue_notify_off;
  rw64_t queue_desc;
  rw64_t queue_driver;
  rw64_t queue_device;
  ro16_t queue_notif_config_data;
  rw16_t queue_reset;

  ro16_t admin_queue_index;
  ro16_t admin_queue_num;
} __attribute__((packed));

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

/* functions */

void init_virtio_blk(struct pci_hdr *hdr);

#endif