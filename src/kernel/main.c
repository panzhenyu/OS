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

int user_a_pid = 0, user_b_pid = 0;

void k_thread_a(void *arg)
{
	char *para = arg;
	console_put_str(" thread_a_pid:0x");
	console_put_uint32t(sys_getpid());
	console_put_char('\n');
	console_put_str(" user_a_pid:0x");
	console_put_uint32t(user_a_pid);
	console_put_char('\n');
	while(1);
}

void k_thread_b(void *arg)
{
	char *para = arg;
	console_put_str(" thread_b_pid:0x");
	console_put_uint32t(sys_getpid());
	console_put_char('\n');
	console_put_str(" user_b_pid:0x");
	console_put_uint32t(user_b_pid);
	console_put_char('\n');
	while(1);
}

void user_a()
{
	user_a_pid = getpid();
	while(1);
}

void user_b()
{
	user_b_pid = getpid();
	while(1);
}

int main()
{
	init_all();
	process_execute(user_a, "a", 10);
	process_execute(user_b, "b", 10);
	intr_enable();
	console_put_str(" main_pid:0x");
	console_put_uint32t(sys_getpid());
	console_put_char('\n');
	thread_start("k_thread_a", 31, k_thread_a, "argA ");
	thread_start("k_thread_b", 31, k_thread_b, "argB ");
	while(1);
	return 0;
}
