import os

# an utility solely to monitor how the kernel grow (excluding bss segment)

siz = os.path.getsize("build/kern.bin")

boot_offset = 0x7c00
fat_offset = 0x1800

read_sectors = 64

span = boot_offset + fat_offset + siz

max = read_sectors * 512 - fat_offset

print(f"kernel size: {siz} B {siz/1000:.3} KB ({siz/1024:.3} KiB)")
print(f"memory span: 0x9400 -- 0x{span:X}")
print(f"maximum kernel size: {max} B {max/1000:.3} KB, {(siz/max * 100):.3}%")