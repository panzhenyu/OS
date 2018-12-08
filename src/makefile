BUILD_DIR = ./build
HD_DIR = /home/panda/Desktop/bochs/disk/hd60M.img
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I include/lib/ -I include/lib/kernel/ -I include/lib/user/ -I include/kernel/ -I include/device/
GLOBAL_HEADER = include/lib/stdint.h include/kernel/global.h
ASFLAGS = -f elf32 
CFLAGS = -Wall $(LIB) -c -m32 -fno-builtin
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o\
       $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/timer.o\
       $(BUILD_DIR)/string.o\
       $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/bitmap.o

## build c code
$(BUILD_DIR)/main.o: kernel/main.c $(GLOBAL_HEADER)\
	include/lib/kernel/print.h\
	include/kernel/init.h include/kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c $(GLOBAL_HEADER)\
	include/lib/kernel/print.h\
	include/kernel/init.h include/kernel/interrupt.h\
	include/device/timer.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c $(GLOBAL_HEADER)\
	include/lib/kernel/print.h include/lib/kernel/io.h\
	include/kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: kernel/memory.c $(GLOBAL_HEADER)\
	include/lib/kernel/print.h\
	include/kernel/memory.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: device/timer.c $(GLOBAL_HEADER)\
	include/lib/kernel/print.h include/lib/kernel/io.h\
	include/device/timer.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string.o: lib/string.c $(GLOBAL_HEADER)\
	include/lib/string.h\
	include/kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: kernel/debug.c $(GLOBAL_HEADER)\
	include/lib/kernel/print.h\
	include/kernel/interrupt.h include/kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c $(GLOBAL_HEADER)\
	include/lib/string.h\
	include/lib/kernel/bitmap.h\
	include/kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

## build asm code
$(BUILD_DIR)/kernel.o: kernel/kernel.s
	$(AS) $(ASFLAGS) $< -o $@ 

$(BUILD_DIR)/print.o: lib/kernel/print.s
	$(AS) $(ASFLAGS) $< -o $@

## link objects
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY : mk_dir hd clean all

mk_dir:
	if [ ! -d $(BUILD_DIR) ]; then\
		mkdir $(BUILD_DIR);\
	fi

hd:
	dd	if=$(BUILD_DIR)/kernel.bin\
		of=$(HD_DIR)\
		bs=512 seek=9 count=200 conv=notrunc

clean:
	cd $(BUILD_DIR) && rm -f ./*

build: $(BUILD_DIR)/kernel.bin

all: mk_dir build hd
