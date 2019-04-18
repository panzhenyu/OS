#include "thread.h"
#include "syscall-init.h"
#include "print.h"
#include "console.h"
#include "string.h"

#define syscall_nr 32
typedef void* syscall;
syscall syscall_table[syscall_nr];

uint32_t sys_getpid()
{
    return running_thread()->pid;
}

uint32_t sys_write(char *str)
{
    console_put_str(str);
    return strlen(str);
}

extern void* sys_malloc(uint32_t size);
extern void sys_free(void* vaddr);

void syscall_init()
{
    put_str("syscall_init start\n");
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_MALLOC] = sys_malloc;
    syscall_table[SYS_FREE] = sys_free;
    put_str("syscall_init done\n");
}
