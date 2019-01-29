#include "print.h"
#include "init.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"

int main()
{
	put_str("I am kernel\n");
	init_all();
	intr_enable();
	while(1);
	return 0;
}
