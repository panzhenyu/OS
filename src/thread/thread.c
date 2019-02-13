#include "thread.h"
#include "stdint.h"
#include "global.h"
#include "memory.h"
#include "string.h"
#include "debug.h"
#include "print.h"
#include "interrupt.h"
#include "list.h"

#define PG_SIZE 4096

struct task_struct* main_thread;
struct list thread_ready_list;
struct list thread_all_list;
static struct list_elem* thread_tag;

extern void switch_to(struct task_struct* cur, struct task_struct* next);

struct task_struct* running_thread()
{
    uint32_t esp;
    asm volatile ("movl %%esp, %0" : "=g"(esp));
    return (struct task_struct*)(esp & 0xfffff000);
}

static void kernel_thread(thread_func* function, void* func_arg)
{
    intr_enable();
    function(func_arg);
}

// static void thread_destroy()
// {
//     while(1);
// }

void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
    pthread->self_kstack -= sizeof(struct intr_stack);          // 预留中断使用栈的空间，作用？
    pthread->self_kstack -= sizeof(struct thread_stack);        // 预留线程栈的空间
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    // kthread_stack->unused_retaddr = thread_destroy;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = \
    kthread_stack->esi = kthread_stack->edi = 0;
}

/* 初始化线程基本信息 */
void init_thread(struct task_struct* pthread, char* name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    if(pthread == main_thread)
        pthread->status = TASK_RUNNING;
    else
        pthread->status = TASK_READY;
    pthread->ticks = prio;
    pthread->priority = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);        // 线程自己在内核态下使用的栈顶地址
    pthread->stack_magic = 0x19870916;
}

struct task_struct* thread_start
(
    char* name,
    int prio,
    thread_func function,
    void* func_arg
)
{
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    ASSERT(!have_elem(&thread_ready_list, &thread->general_tag));           // 链表更新已为原子操作，此处不必担心调度影响任务队列更新
    list_append(&thread_ready_list, &thread->general_tag);
    ASSERT(!have_elem(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    return thread;
}

static void make_main_thread()
{
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);
    ASSERT(!have_elem(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

void schedule()
{
    ASSERT(intr_get_status() == INTR_OFF);
    struct task_struct* cur = running_thread();
    if(cur->status == TASK_RUNNING)
    {
        // 运行中的任务因用完时间片需要调度
        ASSERT(!have_elem(&thread_ready_list, &cur->general_tag));
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
        list_append(&thread_ready_list, &cur->general_tag);
    }
    else
    {
        // 任务需等待某时间发生后才能继续上cpu执行
        ASSERT(!have_elem(&thread_ready_list, &cur->general_tag));
        cur->ticks = cur->priority;
    }
    ASSERT(!list_isEmpty(&thread_ready_list));
    thread_tag = NULL;
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    switch_to(cur, next);
}

/* 阻塞当前线程 */
void thread_block(enum task_status stat)
{
    // 三种阻塞态
    ASSERT(stat == TASK_BLOCKED || stat == TASK_WAITING || stat == TASK_HANGING);
    enum intr_status old_status = intr_disable();
    struct task_struct* cur = running_thread();
    cur->status = stat;
    schedule();
    intr_set_status(old_status);
}

/* 唤醒一个线程 */
void thread_unblock(struct task_struct* pthread)
{
    enum intr_status old_status = intr_disable();
    enum task_status stat = pthread->status;
    ASSERT(stat == TASK_BLOCKED || stat == TASK_WAITING || stat == TASK_HANGING);
    if(pthread->status != TASK_READY)
    {
        ASSERT(!have_elem(&thread_ready_list, &pthread->general_tag));
        if(pthread->status == TASK_RUNNING)
            PANIC("thread_unblock: blocked thread is running\n");
        if(have_elem(&thread_ready_list, &pthread->general_tag))
            PANIC("thread_unblock: blocked thread in ready list\n");
        pthread->status = TASK_READY;
        list_push(&thread_ready_list, &pthread->general_tag);
    }
    intr_set_status(old_status);
}

void thread_init()
{
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    make_main_thread();
    put_str("thread_init done\n");
}