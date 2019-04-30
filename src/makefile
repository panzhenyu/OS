BUILD_DIR = ./build
HD = /home/panda/Desktop/bochs/disk/hd60M.img
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB =	-I include/lib/ -I include/lib/kernel/ -I include/lib/user/ -I include/kernel/\
		-I include/device/ -I include/thread -I include/userprog -I include/user
INCLUDES =	include/lib* include/lib/kernel/* include/lib/user/* include/kernel/*\
			include/device/* include/thread/* include/userprog/* include/user/*
GLOBAL_HEADER = include/lib/stdint.h include/kernel/global.h
ASFLAGS = -f elf32 
CFLAGS = -Wall $(LIB) -c -m32 -fno-builtin
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS =	$(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/console.o $(BUILD_DIR)/keyboard.o\
		$(BUILD_DIR)/timer.o $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/process.o $(BUILD_DIR)/interrupt.o\
		$(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/switch.o\
		$(BUILD_DIR)/tss.o $(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/bitmap.o\
		$(BUILD_DIR)/string.o $(BUILD_DIR)/list.o $(BUILD_DIR)/sync.o $(BUILD_DIR)/syscall.o\
		$(BUILD_DIR)/syscall-init.o $(BUILD_DIR)/stdio.o $(BUILD_DIR)/ide.o

## build c code
$(BUILD_DIR)/main.o: kernel/main.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c $(GLOBAL_HEADER)\
	include/lib/kernel/print.h include/lib/kernel/io.h\
	include/kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: kernel/memory.c $(GLOBAL_HEADER)\
	include/lib/string.h\
	include/lib/kernel/print.h include/lib/kernel/bitmap.h include/lib/kernel/list.h\
	include/kernel/memory.h include/kernel/debug.h include/kernel/interrupt.h\
	include/thread/sync.h include/thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: device/timer.c $(GLOBAL_HEADER)\
	include/lib/kernel/print.h include/lib/kernel/io.h\
	include/kernel/interrupt.h include/thread/thread.h include/kernel/debug.h\
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

$(BUILD_DIR)/thread.o: thread/thread.c $(GLOBAL_HEADER)\
	include/lib/string.h\
	include/lib/kernel/list.h include/lib/kernel/print.h\
	include/kernel/interrupt.h include/kernel/debug.h include/kernel/memory.h\
	include/thread/thread.h include/thread/sync.h\
	include/userprog/process.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/list.o: lib/kernel/list.c $(GLOBAL_HEADER)\
	include/lib/kernel/list.h\
	include/kernel/interrupt.h include/kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/sync.o: thread/sync.c $(GLOBAL_HEADER)\
	include/lib/kernel/list.h\
	include/kernel/interrupt.h include/kernel/debug.h\
	include/thread/sync.h include/thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/console.o: device/console.c $(GLOBAL_HEADER)\
	include/lib/kernel/print.h\
	include/device/console.h\
	include/thread/sync.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o: device/keyboard.c $(GLOBAL_HEADER)\
	include/lib/kernel/io.h\
	include/kernel/interrupt.h\
	include/device/keyboard.h include/device/console.h include/device/ioqueue.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ioqueue.o: device/ioqueue.c $(GLOBAL_HEADER)\
	include/kernel/debug.h include/kernel/interrupt.h\
	include/thread/sync.h include/thread/thread.h\
	include/device/ioqueue.h include/device/keyboard.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/tss.o: userprog/tss.c $(GLOBAL_HEADER)\
	include/lib/string.h\
	include/lib/kernel/print.h\
	include/userprog/tss.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/process.o: userprog/process.c $(GLOBAL_HEADER)\
	include/lib/string.h\
	include/device/console.h\
	include/kernel/memory.h include/kernel/interrupt.h include/kernel/debug.h\
	include/thread/thread.h\
	include/userprog/process.h include/userprog/tss.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall.o: user/syscall.c $(GLOBAL_HEADER)\
	include/lib/kernel/print.h\
	include/user/syscall.h\
	include/userprog/syscall-init.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall-init.o: userprog/syscall-init.c $(GLOBAL_HEADER)\
	include/lib/string.h\
	include/thread/thread.h\
	include/userprog/syscall-init.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/stdio.o: lib/user/stdio.c $(GLOBAL_HEADER)\
	include/lib/string.h\
	include/lib/user/stdio.h\
	include/user/syscall.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ide.o: device/ide.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

## build asm code
$(BUILD_DIR)/mbr.bin: boot/mbr.s
	$(AS) -f bin -I include/boot/ $< -o $@

$(BUILD_DIR)/loader.bin: boot/loader.s
	$(AS) -f bin -I include/boot/ $< -o $@

$(BUILD_DIR)/kernel.o: kernel/kernel.s
	$(AS) $(ASFLAGS) $< -o $@ 

$(BUILD_DIR)/print.o: lib/kernel/print.s
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/switch.o: thread/switch.s
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
	dd	if=$(BUILD_DIR)/mbr.bin\
		of=$(HD)\
		bs=512 count=1 conv=notrunc

	dd	if=$(BUILD_DIR)/loader.bin\
		of=$(HD)\
		bs=512 seek=2 count=4 conv=notrunc

	dd	if=$(BUILD_DIR)/kernel.bin\
		of=$(HD)\
		bs=512 seek=9 count=200 conv=notrunc

clean:
	cd $(BUILD_DIR) && rm -f ./*

build: $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/mbr.bin $(BUILD_DIR)/loader.bin

all: mk_dir build hd
