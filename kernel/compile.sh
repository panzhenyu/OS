gcc -m32 -c -o kernel.o src/main.c
ld -m elf_i386 -Ttext 0xc0001500 -e main -o bin/kernel.bin kernel.o
rm kernel.o
