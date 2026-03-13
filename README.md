# noob-os
### do not try at home
this is undoubtedly the most useless bootable software in the world, anyways just run `make` (make sure `nasm` is installed on your system) and bin/os.img should pop up. in addition, `make run` should run the executable (using qemu-system-i386)

currently the kernel is about 10KB in size (wow), has printf and part-working "shell" (unfun commands). nothing interesting, though

it has too a fat12 driver in some way, it is the MOST COMPLEX part of the entire project!

prerequisites you *must* have before compiling:
```
clang
nasm
lld
llvm
mtools
qemu-system-i386
python3 (optional)
```
while others, `find`, `truncate`, et cetera are assumed to already be on your system  

and yeah, this might be the current memory map:
```
+--------------+
|              |
| other data   |
+- - - - - - - + ????
|              |
|              |
|              | 
| kernel       | 
+--------------+ 0x9400 9.7 KB
|              |
| root dirs    | 
+--------------+ 0x8600 3584 B
|              |
| FAT12 table  | 
+--------------+ 0x7e00 2048 B
| bootstrap    | 
+--------------+ 0x7c00 512 B
|              |
| kernel stack | 
+- - - - - - - + ????
| kernel heap  | 
+--------------+ __bss_end__ (see link.ld)
|              |
| BSS segment  | 
+--------------+ 0x0500 30,4 KB
| BIOS stuff   | 
+--------------+ 0x0000 1280 B
```

fun fact: i specifically used `clang` and the llvm toolchain instead of a gcc cross compiler so that i can compile the project on my mobile phone (via termux), where there cross compilers are the hardest and most painful to install!