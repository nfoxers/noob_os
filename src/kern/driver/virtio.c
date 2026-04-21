#include "driver/virtio.h"
#include "driver/pci.h"
#include "video/printf.h"

void init_virtio_blk(struct pci_hdr *hdr) {
  if(hdr->common.vendid != 0x1af4 || hdr->common.devid != 0x1001) {
    printkf("err: invalid virtio-blk device\n");
    return;
  }

  
}