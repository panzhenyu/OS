#ifndef _DEVICE_IOQUEUE_H
#define _DEVICE_IOQUEUE_H

#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define BUFSIZE 64

struct ioqueue
{
    struct lock lock;
    struct task_struct* producer;       // 记录生产者
    struct task_struct* consumer;       // 记录消费者
    char buf[BUFSIZE];
    uint32_t head;
    uint32_t tail;
};

void ioqueue_init(struct ioqueue* ioq);
int ioq_full(struct ioqueue* ioq);
int ioq_empty(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);
#endif