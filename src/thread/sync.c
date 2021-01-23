#include "interrupt.h"
#include "debug.h"
#include "thread.h"
#include "sync.h"

void sema_init(struct semaphore* sema, uint8_t value)
{
    sema->value = value;
    list_init(&sema->waiters);
}

void lock_init(struct lock* lock)
{
    lock->holder = NULL;
    sema_init(&lock->semaphore, 1);
    lock->holder_repeat_nr = 0;
}

void sema_down(struct semaphore* psema)
{
    enum intr_status old_status = intr_disable();
    // 注意信号量的value为无符号八位整数，因此不可能小于零，应先阻塞再减一
    while(psema->value == 0)
    {
        struct task_struct* cur = running_thread();
        ASSERT(!have_elem(&psema->waiters, &cur->general_tag));
        if(have_elem(&psema->waiters, &cur->general_tag))
            PANIC("sema_down: thread blocked has been in waiters list\n");
        list_append(&psema->waiters, &cur->general_tag);
        thread_block(TASK_BLOCKED);
    }
    --psema->value;
    intr_set_status(old_status);
}

void sema_up(struct semaphore* psema)
{
    enum intr_status old_status = intr_disable();
    if(!list_isEmpty(&psema->waiters))
    {
        struct task_struct* awake_task = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(awake_task);
    }
    ++psema->value;
    intr_set_status(old_status);
}

void lock_acquire(struct lock* plock)
{
    struct task_struct* cur = running_thread();
    if(plock->holder != cur)
    {
        sema_down(&plock->semaphore);
        plock->holder = cur;
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    }
    else
    {
        ++plock->holder_repeat_nr;
    }
}

void lock_release(struct lock* plock)
{
    if(plock->holder_repeat_nr > 1)
    {
        --plock->holder_repeat_nr;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore);
}