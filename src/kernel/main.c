#include "print.h"
#include "init.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"
#include "sync.h"
#include "console.h"

int main()
{
	init_all();
	intr_enable();
	while(1);
	return 0;
}
