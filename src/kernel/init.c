#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "memory.h"
#include "timer.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"
#include "ide.h"
#include "syscall-init.h"
#include "fs.h"

void init_all()
{
	put_str("init_all\n");
	idt_init();
	timer_init();
	mem_init();
	thread_init();
	console_init();
	keyboard_init();
	tss_init();
	syscall_init();
	ide_init();
	fs_init();
}
