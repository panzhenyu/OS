#include "print.h"
#include "init.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"
#include "sync.h"
#include "console.h"
#include "ioqueue.h"

void thread_1(void* g)
{
	while(1)
		console_put_char(getchar());
}
int main()
{
	init_all();
	intr_enable();
	thread_start("thread-1", 1, thread_1, (void*)0);
	while(1);
	return 0;
}
