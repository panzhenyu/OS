# assemble
nasm -I ./include/ ./src/mbr.s -o ./bin/mbr.bin
nasm -I ./include/ ./src/loader.s -o ./bin/loader.bin

# dd
dd if=./bin/mbr.bin of=~/Desktop/bochs/disk/hd60M.img bs=512 count=1 conv=notrunc
dd if=./bin/loader.bin of=~/Desktop/bochs/disk/hd60M.img seek=2 bs=512 count=4 conv=notrunc
