AS = nasm
CC = clang
LD = ld.lld

CFLAGS = -target i386-elf -g -Wall -Wextra -ffreestanding -fno-pic -m32 -Iinclude -Oz -MMD -MP -mno-sse -msoft-float -march=i686
LDFLAGS = -m elf_i386 -T src/link.ld

SRC = $(shell find ./src/kern -name '*.c' -o -name '*.asm')
OBJ = $(patsubst ./src/kern/%.c,./build/kern/%.o, $(SRC))
OBJ := $(patsubst ./src/kern/%.asm,./build/kern/%.o, $(OBJ))

DEP := $(OBJ:.o=.d)


.PHONY: clean run debug size

all: bin/os.img size

bin/os.img: build/boot.bin build/kern.bin
	@mkdir -p bin
	cp $< $@
	truncate $@ -s 320K
	mcopy -i $@ build/kern.bin ::kernel.bin
	mcopy -i $@ data/data.txt ::data.txt
	mmd -i $@ ::tdir
	mcopy -i $@ data/data.txt ::tdir/data.txt

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

-include $(DEP)

clean:
	rm -rf build bin

run: bin/os.img
	qemu-system-i386 -drive format=raw,file=$<
	
debug: bin/os.img
	qemu-system-i386 -s -S -drive format=raw,file=$<

size: tools/chksiz.py
	@echo --------------
	@python3 $<
	@llvm-size build/kern.elf