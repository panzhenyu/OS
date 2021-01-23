#ifndef _SYSCALL_INIT_H
#define _SYSCALL_INIT_H

enum SYSCALL_NR
{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE
};

uint32_t sys_getpid();
uint32_t sys_write(char *str);
extern void* sys_malloc(uint32_t size);
extern void sys_free(void* vaddr);
void syscall_init();

#endif