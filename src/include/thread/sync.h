#ifndef _THREAD_SYNC_H
#define _THREAD_SYNC_H

#include "stdint.h"
#include "list.h"
#include "thread.h"

struct semaphore
{
    uint8_t value;
    struct list waiters;
};

struct lock
{
    struct task_struct* holder;     // 持有者
    struct semaphore semaphore;     // 信号量
    uint32_t holder_repeat_nr;      // 锁的持有者重复申请锁的次数
};

void sema_init(struct semaphore* sema, uint8_t value);
void lock_init(struct lock* lock);
void sema_down(struct semaphore* psema);
void sema_up(struct semaphore* psema);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);

#endif