#ifndef _SYSCALL_INIT_H
#define _SYSCALL_INIT_H

enum SYSCALL_NR
{
    SYS_GETPID,
    SYS_WRITE
};

uint32_t sys_getpid();
uint32_t sys_write(char *str);
void syscall_init();

#endif