#include "print.h"
#include "init.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"
#include "sync.h"
#include "console.h"
#include "ioqueue.h"

void thread_1(void* var)
{
	while(1)
	{
		int x = io_getchar();
		while(x == -1)
			x = io_getchar();
		console_put_char((uint8_t)x);
	}
}

int main()
{
	init_all();
	intr_enable();
	thread_start("thread", 1, thread_1, (void*)0);
	while(1);
	return 0;
}
