IMG=${1:-bin/disk.img}

truncate -s 10M "$IMG"

sgdisk --zap-all "$IMG"
sgdisk --new=1:2048:+3M --typecode=1:8300 "$IMG"
sgdisk --new=2:0:+2M --typecode=2:8300 "$IMG"

S1=$(sgdisk -i 1 "$IMG" | awk '/First sector/ {print $3}')
S2=$(sgdisk -i 2 "$IMG" | awk '/First sector/ {print $3}')

O1=$((S1 * 512))
O2=$((S2 * 512))

touch build/part1.img
touch build/part2.img

truncate -s 3M build/part1.img
truncate -s 2M build/part2.img

mkfs.ext2 -b 1024 build/part1.img
mkfs.ext2 -b 2048 build/part2.img

dd if=build/part1.img of="$IMG" conv=notrunc seek=$O1
dd if=build/part2.img of="$IMG" conv=notrunc seek=$O2

rm build/part1.img
rm build/part2.img