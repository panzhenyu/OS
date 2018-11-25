ld -m elf_i386 -Ttext 0xc0001500 -e main -o ../bin/kernel/kernel.bin kernel/main.o lib/kernel/print.o
