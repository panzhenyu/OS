# compile

gcc -c src/main.c -o main.o
ld -e main -Ttext 0xc0001500 main.o -o bin/kernel.bin
rm main.o

# dd

dd if=bin/kernel.bin of=~/Desktop/bochs/disk/hd60M.img bs=512 count=200 seek=9 conv=notrunc
