#include "interrupt.h"
#include "debug.h"
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
    while(psema->value <= 0)
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