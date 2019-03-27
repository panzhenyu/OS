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

int test_var_a = 0;

void k_thread_a(void* args)
{
	while(1)
	{
		// console_put_uint32t(test_var_a);
		// console_put_char('\n');
	}
}

void user_a()
{
	while(1)
	{
		test_var_a++;
	}
}

void user_b()
{
	while(1)
		test_var_a++;
}
int main()
{
	init_all();
	thread_start("consumer_a", 31, k_thread_a, " A_");
	process_execute(user_a, "a", 10);
	process_execute(user_b, "b", 10);
	intr_enable();
	while(1);
	return 0;
}
