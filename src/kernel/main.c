#include "print.h"
#include "init.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"

void thread1(void* arg)
{
	while(1)
		put_str("thread-1 ");
}

void thread2(void* arg)
{
	while(1)
		put_str("thread-2 ");
}

int main()
{
	put_str("I am kernel\n");
	init_all();
	thread_start("thread-1", 31, thread1, (void*)0);
	thread_start("thread-1", 8, thread2, (void*)0);
	intr_enable();
	while(1);
	return 0;
}
