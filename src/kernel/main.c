#include "print.h"
#include "init.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"
#include "sync.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "process.h"
#include "syscall.h"
#include "syscall-init.h"
#include "stdio.h"

void k_thread_a(void *arg)
{
	void *addr[8];
	console_put_str(" thread_a start\n");
	int max = 1000;
	while(max--)
	{
		int size = 128;
		addr[1] = sys_malloc(size);
		size *= 2;
		addr[2] = sys_malloc(size);
		size *= 2;
		addr[3] = sys_malloc(size);
		sys_free(addr[1]);
		addr[4] = sys_malloc(size);
		size *= 2; size *= 2; size *= 2; size *= 2;
		size *= 2; size *= 2; size *= 2;
		addr[5] = sys_malloc(size);
		addr[6] = sys_malloc(size);
		sys_free(addr[5]);
		size *= 2;
		addr[7] = sys_malloc(size);
		sys_free(addr[6]);
		sys_free(addr[7]);
		sys_free(addr[2]);
		sys_free(addr[3]);
		sys_free(addr[4]);
	}
	console_put_str(" thread_a end\n");
	while(1);
}

void k_thread_b(void *arg)
{
	void *addr[10];
	int max = 1000;
	console_put_str(" thread_b start\n");
	while(max--)
	{
		int size = 9;
		addr[1] = sys_malloc(size);
		size *= 2;
		addr[2] = sys_malloc(size);
		size *= 2;
		sys_free(addr[2]);
		addr[3] = sys_malloc(size);
		sys_free(addr[1]);
		addr[4] = sys_malloc(size);
		addr[5] = sys_malloc(size);
		addr[6] = sys_malloc(size);
		sys_free(addr[5]);
		size *= 2;
		addr[7] = sys_malloc(size);
		sys_free(addr[6]);
		sys_free(addr[7]);
		sys_free(addr[3]);
		sys_free(addr[4]);
		size *= 2; size *= 2; size *= 2;
		addr[1] = sys_malloc(size);
		addr[2] = sys_malloc(size);
		addr[3] = sys_malloc(size);
		addr[4] = sys_malloc(size);
		addr[5] = sys_malloc(size);
		addr[6] = sys_malloc(size);
		addr[7] = sys_malloc(size);
		addr[8] = sys_malloc(size);
		addr[9] = sys_malloc(size);
		sys_free(addr[1]);
		sys_free(addr[2]);
		sys_free(addr[3]);
		sys_free(addr[4]);
		sys_free(addr[5]);
		sys_free(addr[6]);
		sys_free(addr[7]);
		sys_free(addr[8]);
		sys_free(addr[9]);
	}
	console_put_str(" thread_b end\n");
	while(1);
}

int main()
{
	init_all();
	intr_enable();
	thread_start("k_thread_a", 31, k_thread_a, "I am thread_a");
	thread_start("k_thread_b", 31, k_thread_b, "I am thread_b");
	while(1);
	return 0;
}
