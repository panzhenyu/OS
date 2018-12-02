%.o:%.c
	gcc -c -o $@ $^
obj = t1.o t2.o
test.bin:$(obj)
	gcc -o $@ $^
all:test.bin
	@echo "compile done"
