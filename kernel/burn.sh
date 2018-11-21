# compile

gcc -m32 -c -o kernel.o src/main.c
ld -m elf_i386 -Ttext 0xc0001500 -e main -o bin/kernel.bin kernel.o
rm kernel.o

# dd

dd if=bin/kernel.bin of=~/Desktop/bochs/disk/hd60M.img bs=512 count=200 seek=9 conv=notrunc
