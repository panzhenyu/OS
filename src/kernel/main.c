#include "print.h"
#include "init.h"

int main()
{
	put_str("I am kernel\n");
	init_all();
	asm volatile("sti");		// 临时开中断
	while(1);
	return 0;
}
