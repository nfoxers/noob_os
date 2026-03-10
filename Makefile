AS = nasm
CC = clang
LD = ld.lld

CFLAGS = -target i386-elf -ffreestanding -m32 -Iinclude -Oz
LDFLAGS = -m elf_i386 -T src/link.ld

SRC = $(shell find ./src/kern -name *.c -o -name *.asm)
OBJ = $(patsubst ./src/kern/%.c,./build/kern/%.o, $(SRC))
OBJ := $(patsubst ./src/kern/%.asm,./build/kern/%.o, $(OBJ))

.PHONY: clean run

all: bin/os.img

bin/os.img: build/boot.bin build/kern.bin
	@mkdir -p bin
	cp $< $@
	truncate $@ -s 320K
	mcopy -i $@ build/kern.bin ::KERNEL.BIN

build/boot.bin: src/boot/boot.asm
	@mkdir -p build
	$(AS) -fbin $< -o $@

build/kern.bin: build/kern.elf
	llvm-objcopy -O binary $< $@

build/kern.elf: $(OBJ)
	$(LD) $(LDFLAGS) $^ -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.asm
	@mkdir -p $(dir $@)
	$(AS) -felf $< -o $@

clean:
	rm -rf build bin

run: bin/os.img
	qemu-system-i386 -drive format=raw,file=$<
	
