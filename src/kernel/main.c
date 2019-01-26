#include "print.h"
#include "init.h"
#include "memory.h"
#include "thread.h"

void f()
{
	put_str("hhhl");
	while(1);
}

int main()
{
	put_str("I am kernel\n");
	init_all();
	thread_start("hh", 1, f, (void*)0);
	while(1);
	return 0;
}
