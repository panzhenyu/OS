#include "print.h"
#include "init.h"
#include "memory.h"

int main()
{
	put_str("I am kernel\n");
	init_all();
	void* addr = get_kernel_pages(3);
	put_str("\n get_kernel_page start vaddr is ");
	put_uint32_hex((uint32_t)addr);
	put_str("\n");
	uint32_t* num = 0x00000000;
	put_uint32_hex(*num);
	while(1);
	return 0;
}
