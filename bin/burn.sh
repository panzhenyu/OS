dd if=boot/mbr.bin of=~/Desktop/bochs/disk/hd60M.img bs=512 count=1 conv=notrunc
dd if=boot/loader.bin of=~/Desktop/bochs/disk/hd60M.img seek=2 bs=512 count=4 conv=notrunc
dd if=kernel/kernel.bin of=~/Desktop/bochs/disk/hd60M.img bs=512 count=200 seek=9 conv=notrunc
