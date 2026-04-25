AS = nasm
AR = llvm-ar
CC = clang
LD = ld.lld
STRIP = llvm-strip
GZ = gzip
XZ = xz

CFLAGS = -target i386-elf -g -Wall -Wextra -ffreestanding -fno-pic -m32 -Iinclude -Oz -MMD -MP \
-march=i686 -foptimize-sibling-calls -fno-stack-protector -fno-builtin \
-fno-ident -fno-asynchronous-unwind-tables -fno-unwind-tables \
-fno-pie -falign-functions=1 -falign-loops=1 -fno-plt -nostdlib

CFLAGSu = $(CFLAGS) -nostartfiles -nodefaultlibs

LDFLAGS = -m elf_i386 -T src/link.ld
LDFLAGSu = -m elf_i386 -T user/link.ld

QMFLAGS = 

SRC = $(shell find ./src/kern -name '*.c' -o -name '*.asm')
OBJ = $(patsubst ./src/kern/%.c,./build/kern/%.o, $(SRC))
OBJ := $(patsubst ./src/kern/%.asm,./build/kern/%.o, $(OBJ))

SRCu = $(shell find ./user -name '*.c')
OBJu = $(patsubst ./user/%.c,./build/user/%.o, $(SRCu))

SRClc = $(shell find ./libc -name '*.c')
OBJlc = $(patsubst ./libc/%.c,./build/libc/%.o, $(SRClc)) ./build/libc/crt.o

DATA := $(shell find ./data)

DEP := $(OBJ:.o=.d) $(OBJu:.o=.d)
DEP_EXTRA := Makefile grub/grub.cfg $(DATA)


.PHONY: clean run debug size size2

all: size bin/os.img 

bin/os.img: build/kern.elf build/user.elf $(DEP_EXTRA)
	@mkdir -p bin

	$(STRIP) --strip-all $< -o build/kernel
#	rm build/kernel.gz
	$(XZ) -9f build/kernel

	cp ./grub/initdisk.img $@
	e2cp ./grub/grub.cfg $@:/boot/grub
	e2cp build/kernel.xz $@:/boot

	e2cp data/etc/* $@:/etc

	e2mkdir $@:/dev
	e2mkdir -P 666 $@:/root
	e2mkdir $@:/home/user

build/user.elf: $(OBJu) build/libc.a
	$(LD) $(LDFLAGSu) $^ -o $@

build/kern.elf: $(OBJ)
	$(LD) $(LDFLAGS) $^ -o $@

build/libc.a: $(OBJlc)
	rm -f $@
	$(AR) rcs $@ $^



build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.asm
	@mkdir -p $(dir $@)
	$(AS) -felf $< -o $@

build/user/%.o: user/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/libc/%.o: libc/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/libc/%.o: libc/%.asm
	@mkdir -p $(dir $@)
	$(AS) -felf $< -o $@

-include $(DEP)

clean:
	rm -rf build bin

run: bin/os.img
	qemu-system-i386 $(QMFLAGS) -drive format=raw,file=$<,if=ide
	
debug: bin/os.img
	qemu-system-i386 -s -S $(QMFLAGS) -drive format=raw,file=$<,if=ide

size: build/kern.elf
	@echo ---------------------------------
	@llvm-size $<

size2: bin/os.img
	@llvm-objdump -t build/kern.elf | awk '{ dec = strtonum("0x"$$5); print dec, $$0 }' | sort -n
