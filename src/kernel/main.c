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
	while (1) {
		void *addr1 = sys_malloc(256);
		void *addr2 = sys_malloc(255);
		void *addr3 = sys_malloc(254);
		console_put_str(" thread_a malloc addr:0x");
		console_put_uint32t((int)addr1);
		console_put_char(',');
		console_put_uint32t((int)addr2);
		console_put_char(',');
		console_put_uint32t((int)addr3);
		console_put_char('\n');
		int cpu_delay = 10000000;
		while(cpu_delay--);
		sys_free(addr1);
		sys_free(addr2);
		sys_free(addr3);
	}
}

void k_thread_b(void *arg)
{
	while (1) {
		void *addr1 = sys_malloc(256);
		void *addr2 = sys_malloc(255);
		void *addr3 = sys_malloc(254);
		console_put_str(" thread_b malloc addr:0x");
		console_put_uint32t((int)addr1);
		console_put_char(',');
		console_put_uint32t((int)addr2);
		console_put_char(',');
		console_put_uint32t((int)addr3);
		console_put_char('\n');
		int cpu_delay = 10000000;
		while(cpu_delay--);
		sys_free(addr1);
		sys_free(addr2);
		sys_free(addr3);
	}
}

void u_prog_a()
{
	void *addr1 = malloc(256);
	void *addr2 = malloc(255);
	void *addr3 = malloc(254);
	printf(" prog_a malloc addr:0x%x, 0x%x, 0x%x\n", (int)addr1, (int)addr2, (int)addr3);
	int cpu_delay = 10000000;
	while(cpu_delay--);
	free(addr1);
	free(addr2);
	free(addr3);
	while(1);
}

void u_prog_b()
{
	void *addr1 = malloc(256);
	void *addr2 = malloc(255);
	void *addr3 = malloc(254);
	printf(" prog_a malloc addr:0x%x, 0x%x, 0x%x\n", (int)addr1, (int)addr2, (int)addr3);
	int cpu_delay = 10000000;
	while(cpu_delay--);
	free(addr1);
	free(addr2);
	free(addr3);
	while(1);
}

int main()
{
	init_all();
	intr_enable();
	console_put_str("begin\n");
	thread_start("k_thread_a", 31, k_thread_a, "I am thread_a");
	thread_start("k_thread_b", 31, k_thread_b, "I am thread_b");
	process_execute(u_prog_a, "u_prog_a", 31);
	process_execute(u_prog_b, "u_prog_b", 31);
	while(1);
	return 0;
}
