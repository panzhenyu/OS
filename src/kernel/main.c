#include "print.h"
#include "init.h"
#include "debug.h"

int main()
{
	put_str("I am kernel\n");
	init_all();
	asm volatile("sti");		// 临时开中断
	ASSERT(1==2);
	while(1);
	return 0;
}
