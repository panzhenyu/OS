# make objects
nasm 	-f bin\
	-I boot/include/\
	-o ../bin/boot/mbr.bin\
	boot/mbr.s

nasm 	-f bin\
	-I boot/include/\
	-o ../bin/boot/loader.bin\
	boot/loader.s

nasm 	-f elf32\
	-o ../bin/lib/kernel/print.o\
	lib/kernel/print.s

nasm	-f elf32\
	-o ../bin/kernel/kernel.o\
	kernel/kernel.s

gcc	-I lib/ -I lib/kernel/\
	-o ../bin/kernel/main.o\
	-m32 -c\
	kernel/main.c
