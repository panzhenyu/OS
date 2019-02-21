#include "print.h"
#include "init.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"
#include "sync.h"
#include "console.h"


void thread1(void* arg)
{
	while(1)
	{
		console_put_str("thread_1 ");
	}
}

void thread2(void* arg)
{
	while(1)
	{
		console_put_str("thread_2 ");
	}
}

int main()
{
	init_all();
	intr_enable();
	thread_start("thread-1", 1, thread1, (void*)0);
	thread_start("thread-2", 1, thread2, (void*)0);
	while(1);
	return 0;
}
