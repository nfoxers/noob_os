#!/bin/bash

if [ "$(id -u)" -ne 0 ]; then
    echo "try using sudo, ok?"
    exit 1
fi

dd if=/dev/zero of=./grub/initdisk.img bs=1M count=16
export LOOPDEV=$(sudo losetup --show -f ./grub/initdisk.img)
mkfs.ext2 -v $LOOPDEV
sudo mount $LOOPDEV /mnt/os
sudo mkdir /mnt/os/boot
sudo grub-install --target=i386-pc --boot-directory=/mnt/os/boot --no-floppy --force $LOOPDEV
sudo umount /mnt/os
sudo losetup -d $LOOPDEV
sudo chmod a+rwx ./grub/initdisk.img