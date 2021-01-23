#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "stdint.h"

uint32_t getpid();
uint32_t write(char *str);
void* malloc(uint32_t size);
void free(void* vaddr);

#endif