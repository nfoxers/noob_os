# noob-os
### do not try at home
this is undoubtedly the most useless bootable software in the world.

aanyways just run `make` and bin/os.img should pop up. in addition, `make run` should run the executable (using qemu-system-i386).

currently the kernel is about 35KB in size, has vfs and part-working kernel mode "shell" (unfun commands). nothing interesting, though.

*i must stress that this hobby project does not have and will never have any production usability*

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
low memory
+==============+ 0x000FFFFF
|              |
| other data   |
+- - - - - - - + ????
|              |
|              |
|              | 
| kernel       | 
+--------------+ 0x9400 26.3 KiB
|              |
| root dirs    | 
+--------------+ 0x8600 3584 B
|              |
| FAT12 table  | 
+--------------+ 0x7e00 2048 B
| bootstrap    | 
+--------------+ 0x7c00 512 B
| kernel stack | 
+- - - - - - - + ????
|              |
| proc stack   |
+--------------+ __bss_end__
|              |
| BSS segment  | 
+--------------+ 0x0500 30,4 KB
| BIOS stuff   | 
+==============+ 0x0000 1280 B
```
```
high memory
+==============+ ????
|              |
| stuff?       |
+- - - - - - - + 0x00200000
|              |
|              |
|              |
| kernel heap  |
+==============+ 0x00100000 1 MiB

```


fun fact: i specifically used `clang` and the llvm toolchain instead of a gcc cross compiler so that i can compile the project on my mobile phone (via termux), where there cross compilers are the hardest and most painful to install!

## screenshot
### 04/12 (acdf7f)

![boot image thing](./screenshot/0412.png)

![file operations thing](./screenshot/0412_fops.png)

### 04/19 (0c9a8cb)

![login screen :3](./screenshot/0419.png)

![idk](./screenshot/0419_ok.png)

*these text might make you assume the kernel's advanced, but in reality it never was THAT complex*

## legal notice
this is still a hobby OS project, it doesn't store, process, or use any kind of user data. it is also void of and does not provide accounts, services, or content.

so things like age verification / platform regulation (see California AB-1043 & Brazil Law No. 15.211/2025) doesn't really apply "in my opinion". therefore, you are still allowed to locally distribute this software from within these region, but be aware that any unforeseeable legal consequences... can *probably* befall you.
