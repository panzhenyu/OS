#ifndef _SYSCALL_INIT_H
#define _SYSCALL_INIT_H

enum SYSCALL_NR
{
    SYS_GETPID
};

uint32_t sys_getpid();
void syscall_init();

#endif