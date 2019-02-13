#include "print.h"
#include "init.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"
#include "sync.h"

struct lock* iolock;

void thread1(void* arg)
{
	while(1)
	{
		lock_acquire(iolock);
		put_str("thread-1\n");
		lock_release(iolock);
	}
}

void thread2(void* arg)
{
	while(1)
	{
		lock_acquire(iolock);
		put_str("thread-2\n");
		lock_release(iolock);
	}
}

int main()
{
	put_str("I am kernel\n");
	init_all();
	lock_init(iolock);
	intr_enable();
	thread_start("thread-1", 1, thread1, (void*)0);
	thread_start("thread-2", 1, thread2, (void*)0);
	while(1);
	return 0;
}
