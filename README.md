# noob-os
### do not try at home
this is undoubtedly the most useless bootable software in the world, anyways just run `make` (make sure `nasm` is installed on your system) and bin/os.img should pop up. in addition, `make run` should run the executable (using qemu-system-i386)

currently the kernel is about 12KB in size (wow), has printf and part-working "shell" (unfun commands). nothing interesting, though  

*i must stress that this hobby project does not have and will never have any production usability*

it has too a fat12 driver (partial usability), and cooperative multitasking

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

and this might be the current memory map:
```
+--------------+
|              |
| other data   |
+- - - - - - - + ????
|              |
|              |
|              | 
| kernel       | 
+--------------+ 0x9400 12.2 KiB
|              |
| root dirs    | 
+--------------+ 0x8600 3584 B
|              |
| FAT12 table  | 
+--------------+ 0x7e00 2048 B
| bootstrap    | 
+--------------+ 0x7c00 512 B
| kernel stack | 
+- - - - - - - +
|              |
| proc stack   |
+- - - - - - - + ????
| kernel heap  | 
+--------------+ __bss_end__
|              |
| BSS segment  | 
+--------------+ 0x0500 30,4 KB
| BIOS stuff   | 
+--------------+ 0x0000 1280 B
```

nota bene: every process other than the root proc gets 512B of stack space

fun fact: i specifically used `clang` and the llvm toolchain instead of a gcc cross compiler so that i can compile the project on my mobile phone (via termux), where there cross compilers are the hardest and most painful to install!

### legal notice
this is still a hobby OS project, it doesn't store, process, or use any kind of user data. it is also void of and does not provide accounts, services, or content.

so things like age verification / platform regulation (see California AB-1043 & Brazil Law No. 15.211/2025) doesn't really apply "in my opinion". therefore, you are still allowed to locally distribute this software from within these region, but be aware that any unforeseeable legal consequences... can *probably* befall you.
