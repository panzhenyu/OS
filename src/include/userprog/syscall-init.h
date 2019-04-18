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
void syscall_init();

#endif