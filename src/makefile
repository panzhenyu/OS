BUILD_DIR = ./build
HD = /home/panda/Desktop/bochs/disk/hd60M.img
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB =	-I include/lib/ -I include/lib/kernel/ -I include/lib/user/ -I include/kernel/		\
		-I include/device/ -I include/thread -I include/userprog -I include/user			\
		-I include/fs/
INCLUDES =	include/lib* include/lib/kernel/* include/lib/user/* include/kernel/*			\
			include/device/* include/thread/* include/userprog/* include/user/*				\
			include/fs/*
ASFLAGS = -f elf32
CFLAGS = -Wall $(LIB) -c -m32 -fno-builtin
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS =	$(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/console.o $(BUILD_DIR)/keyboard.o			\
		$(BUILD_DIR)/timer.o $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/process.o $(BUILD_DIR)/interrupt.o		\
		$(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/switch.o			\
		$(BUILD_DIR)/tss.o $(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/bitmap.o				\
		$(BUILD_DIR)/string.o $(BUILD_DIR)/list.o $(BUILD_DIR)/sync.o $(BUILD_DIR)/syscall.o			\
		$(BUILD_DIR)/syscall-init.o $(BUILD_DIR)/stdio.o $(BUILD_DIR)/ide.o $(BUILD_DIR)/fs.o			\

## build c code
$(BUILD_DIR)/main.o: kernel/main.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: kernel/memory.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: device/timer.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string.o: lib/string.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: kernel/debug.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/thread.o: thread/thread.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/list.o: lib/kernel/list.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/sync.o: thread/sync.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/console.o: device/console.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o: device/keyboard.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ioqueue.o: device/ioqueue.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/tss.o: userprog/tss.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/process.o: userprog/process.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall.o: user/syscall.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall-init.o: userprog/syscall-init.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/stdio.o: lib/user/stdio.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ide.o: device/ide.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/fs.o: fs/fs.c $(INCLUDES)
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
