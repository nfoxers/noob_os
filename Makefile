AS = nasm
CC = clang
LD = ld.lld

CFLAGS = -target i386-elf -g -Wall -Wextra -ffreestanding -fno-pic -m32 -Iinclude -Oz -MMD -MP \
-march=i686 -foptimize-sibling-calls -fomit-frame-pointer -fno-stack-protector -fno-builtin \
-fno-ident -fno-asynchronous-unwind-tables -fno-unwind-tables \
-fno-pie -falign-functions=1 -falign-loops=1 -fno-plt -nostdlib

LDFLAGS = -m elf_i386 -T src/link.ld
LDFLAGSu = -m elf_i386 -T user/link.ld

QMFLAGS = -netdev user,id=net0 -device rtl8139,netdev=net0

SRC = $(shell find ./src/kern -name '*.c' -o -name '*.asm')
OBJ = $(patsubst ./src/kern/%.c,./build/kern/%.o, $(SRC))
OBJ := $(patsubst ./src/kern/%.asm,./build/kern/%.o, $(OBJ))

SRCu = $(shell find ./user -name '*.c')
OBJu = $(patsubst ./user/%.c,./build/user/%.o, $(SRCu))

DEP := $(OBJ:.o=.d) $(OBJu:.o=.d)

.PHONY: clean run debug size size2

all: bin/os.img size

bin/os.img: build/boot.bin build/kern.bin build/user.bin
	@mkdir -p bin
	cp $< $@
	truncate $@ -s 320K
	mcopy -i $@ build/kern.bin ::kernel.bin
	mmd -i $@ ::home
	mcopy -i $@ data/data.txt ::home/data.txt
	mmd -i $@ ::home/tdir
	mcopy -i $@ data/data.txt ::home/tdir/data.txt
	mcopy -i $@ build/user.bin ::home/user.bin

build/boot.bin: src/boot/boot.asm
	@mkdir -p build
	$(AS) -fbin $< -o $@

build/kern.bin: build/kern.elf
	llvm-objcopy -O binary $< $@

build/user.bin: build/user.elf
	llvm-objcopy -O binary $< $@

build/user.elf: $(OBJu)
	$(LD) $(LDFLAGS) $^ -o $@

build/kern.elf: $(OBJ)
	$(LD) $(LDFLAGS) $^ -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/user/%.o: user/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.asm
	@mkdir -p $(dir $@)
	$(AS) -felf $< -o $@

-include $(DEP)

clean:
	rm -rf build bin

run: bin/os.img
	qemu-system-i386 $(QMFLAGS) -drive format=raw,file=$<
	
debug: bin/os.img
	qemu-system-i386 -s -S $(QMFLAGS) -drive format=raw,file=$<

size: tools/chksiz.py
	@echo --------------
	@python3 $<
	@llvm-size build/kern.elf

size2: bin/os.img
	@llvm-objdump -t build/kern.elf | awk '{ dec = strtonum("0x"$$5); print dec, $$0 }' | sort -n