#ifndef _KERNEL_IOQUEUE_H
#define _KERNEL_IOQUEUE_H

#include "stdint.h"
#include "list.h"
#include "sync.h"

#define IOQUEUE_INIT_SIZE 1024

struct ioqueue
{
    uint8_t ascii[IOQUEUE_INIT_SIZE];
    uint32_t size;
    uint32_t length;
    struct lock iolock;
};

void ioq_init(struct ioqueue* piq);
void ioqueue_init();
void io_input(uint8_t ascii);
void io_getchar();
uint8_t getchar();
char* io_getline();

#endif