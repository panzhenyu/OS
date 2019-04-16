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
	console_put_uint32t((uint32_t)sys_malloc(33));
	console_put_uint32t((uint32_t)sys_malloc(63));
	while(1);
}

void user_a()
{
	write("I am user_a\n");
	while(1);
}

int main()
{
	init_all();
	thread_start("k_thread_a", 31, k_thread_a, "argA ");
	// process_execute(user_a, "a", 10);
	intr_enable();
	while(1);
	return 0;
}
