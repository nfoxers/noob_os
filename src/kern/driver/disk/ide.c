#include "dev/block_dev.h"
#include "fs/devfs.h"
#include "video/printf.h"
#include <driver/disk/ide.h>
#include <io.h>
#include <mem/mem.h>
#include <stddef.h>
#include <stdint.h>

// todo: actual ide/ahci support

struct gendisk   atamaster;
struct block_dev ata_bdev;
struct bd_ops    ata_ops;
struct req_queue ata_queue;
struct dpart_tbl ata_parts;
struct hd_struct ata_part;

static void ata_delay(void) {
  inb(ATA_PRIMARY_CTRL);
  inb(ATA_PRIMARY_CTRL);
  inb(ATA_PRIMARY_CTRL);
  inb(ATA_PRIMARY_CTRL);
}

static int ata_poll_bsy(void) {
  ata_delay();
  uint32_t timeout = 100000;
  while (timeout--) {
    uint8_t status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
    if (status & ATA_SR_ERR)
      return -1;
    if (!(status & ATA_SR_BSY))
      return 0;
  }
  return -1;
}

static int ata_poll_drq(void) {
  uint32_t timeout = 100000;
  while (timeout--) {
    uint8_t status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
    if (status & ATA_SR_ERR)
      return -1;
    if (status & ATA_SR_DRQ)
      return 0;
  }
  return -1;
}

static int ata_check_drive(int drive) {
  uint8_t select = drive ? ATA_SELECT_SLAVE : ATA_SELECT_MASTER;

  // Select the drive
  outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, select);
  ata_delay();

  outb(ATA_PRIMARY_BASE + ATA_REG_SECCOUNT, 0);
  outb(ATA_PRIMARY_BASE + ATA_REG_LBA_LO, 0);
  outb(ATA_PRIMARY_BASE + ATA_REG_LBA_MID, 0);
  outb(ATA_PRIMARY_BASE + ATA_REG_LBA_HI, 0);

  outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
  ata_delay();

  uint8_t status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
  if (status == 0)
    return 0;

  if (ata_poll_bsy() != 0)
    return 0;

  uint8_t mid = inb(ATA_PRIMARY_BASE + ATA_REG_LBA_MID);
  uint8_t hi  = inb(ATA_PRIMARY_BASE + ATA_REG_LBA_HI);
  if (mid != 0 || hi != 0)
    return 0;

  if (ata_poll_drq() != 0)
    return 0;

  for (int i = 0; i < 256; i++) {
    inw(ATA_PRIMARY_BASE + ATA_REG_DATA);
  }

  return 1;
}

ssize_t dsk_read(struct block_dev *bd, sector_t sect, size_t len, void *buf) {
  if(bd && bd->bd_disk && bd->bd_disk->bops && bd->bd_disk->bops->read) {
    return bd->bd_disk->bops->read(sect, len, buf);
  }
  return -1;
}

struct device ata_dev;

ssize_t ata_devread(struct device *d, void *buf, size_t count) {
  struct block_dev *bd = d->pdata;

  size_t   off  = d->off;
  sector_t sect = bd->bd_start + off / 512;
  off %= 512;
  size_t   i;
  uint8_t *buff = malloc(512);

  for (i = 0; i < count / 512; i++) {
    dsk_read(bd, sect, 1, buff);
    memcpy(buf + i*512, buff + off, 512 - off);
    off = 0;
  }

  dsk_read(bd, sect + i, 1, buff);
  memcpy(buf + i*512, buff + off, count % 512);

  d->off += count;
  free(buff);
  return count;
}

int ata_init(void) {
  int r = ata_check_drive(ATA_DRIVE_MASTER);
  if (!r) {
    printkf("no drives found");
  }

  ata_ops.read  = ata_read_sectors;
  ata_ops.write = ata_write_sectors;

  ata_part.bd         = NULL;
  ata_part.nr_sect    = 12345;
  ata_part.start_sect = 0;
  ata_parts.part[0]   = &ata_part;

  ata_queue.head = NULL;

  atamaster.maj      = 2;
  atamaster.minor    = 0;
  atamaster.bops     = &ata_ops;
  atamaster.part_tbl = &ata_parts;
  atamaster.queue    = &ata_queue;

  ata_bdev.bd_queue = &ata_queue;
  ata_bdev.bd_dev   = 2 << 16;
  ata_bdev.bd_disk  = &atamaster;
  ata_bdev.bd_start = 0;
  ata_bdev.parent   = NULL;

  ata_dev.name     = "sda";
  ata_dev.pdata    = &ata_bdev;
  ata_dev.ops.read = ata_devread;

  creat_devfs("sda", &ata_dev, 2, 0);

  return 1;
}

int ata_read_sectors(uint32_t lba, size_t sec_count, uint8_t buf[static 512]) {
  if (ata_poll_bsy() != 0)
    return -1;

  uint8_t lba_select = (ATA_LBA_MASTER) | ((lba >> 24) & 0x0F);

  outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, lba_select);
  ata_delay();
  outb(ATA_PRIMARY_BASE + ATA_REG_SECCOUNT, sec_count);
  outb(ATA_PRIMARY_BASE + ATA_REG_LBA_LO, (uint8_t)(lba));
  outb(ATA_PRIMARY_BASE + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
  outb(ATA_PRIMARY_BASE + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));
  outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

  uint16_t *p = (uint16_t *)buf;
  for (size_t s = 0; s < sec_count; s++) {
    if (ata_poll_bsy() != 0)
      return -1;
    if (ata_poll_drq() != 0)
      return -1;
    for (int i = 0; i < 256; i++) {
      p[s * 256 + i] = inw(ATA_PRIMARY_BASE + ATA_REG_DATA);
    }
  }
  return sec_count;
}

int ata_write_sectors(uint32_t lba, size_t count, const uint8_t buf[static 512]) {
  if (ata_poll_bsy() != 0)
    return -1;

  uint8_t lba_select = (ATA_LBA_MASTER) | ((lba >> 24) & 0x0F);

  outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, lba_select);
  ata_delay();
  outb(ATA_PRIMARY_BASE + ATA_REG_SECCOUNT, count);
  outb(ATA_PRIMARY_BASE + ATA_REG_LBA_LO, (uint8_t)(lba));
  outb(ATA_PRIMARY_BASE + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
  outb(ATA_PRIMARY_BASE + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));
  outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

  const uint16_t *p = (const uint16_t *)buf;
  for (size_t s = 0; s < count; s++) {
    if (ata_poll_bsy() != 0)
      return -1;
    if (ata_poll_drq() != 0)
      return -1;
    for (int i = 0; i < 256; i++) {
      outw(ATA_PRIMARY_BASE + ATA_REG_DATA, p[s * 256 + i]);
    }
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_FLUSH);
    if (ata_poll_bsy() != 0)
      return -1;
  }
  return 0;
}