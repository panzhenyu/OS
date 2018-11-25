# make objects
nasm 	-I boot/include/\
	-o ../bin/boot/mbr.bin\
	boot/mbr.s
nasm 	-I boot/include/\
	-o ../bin/boot/loader.bin\
	boot/loader.s
nasm 	-f elf32\
	-o ../bin/lib/kernel/print.o\
	lib/kernel/print.s
gcc 	-m32 -c\
	-I lib/ -I lib/kernel/\
	-o ../bin/kernel/main.o\
	kernel/main.c
