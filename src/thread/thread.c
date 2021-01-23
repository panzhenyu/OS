#include "thread.h"
#include "stdint.h"
#include "global.h"
#include "memory.h"
#include "string.h"
#include "debug.h"
#include "print.h"
#include "interrupt.h"
#include "list.h"
#include "process.h"
#include "sync.h"

#define PG_SIZE 4096

struct task_struct *main_thread;    // 主线程
struct task_struct *idle_thread;    // idel线程
struct list thread_ready_list;      // 所有线程的队列
struct list thread_all_list;        // 就绪线程队列
static struct list_elem* thread_tag;

/* 线程切换函数 */
extern void switch_to(struct task_struct* cur, struct task_struct* next);

/* 获取运行线程PCB指针 */
struct task_struct* running_thread()
{
    uint32_t esp;
    asm volatile ("movl %%esp, %0" : "=g"(esp));
    return (struct task_struct*)(esp & 0xfffff000);
}

/* 获取进程pid*/
struct lock pid_lock;           // pid锁
static pid_t allocate_pid()
{
    static pid_t next_pid = 0;
    lock_acquire(&pid_lock);
    next_pid++;
    lock_release(&pid_lock);
    return next_pid;
}

/* 线程入口函数 */
static void kernel_thread(thread_func* function, void* func_arg)
{
    // 这里打开中断很关键，内核线程第一次被调度时，并不是处于退出中断的状态，不会通过退出中断的一系列pop操作修正中断标志位
    intr_enable();
    function(func_arg);
}

// static void thread_destroy()
// {
//     while(1);
// }

/* 创建线程上下文环境 */
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
    pthread->self_kstack -= sizeof(struct intr_stack);          // 预留中断使用栈的空间，供用户进程使用
    pthread->self_kstack -= sizeof(struct thread_stack);        // 预留线程栈的空间，供调度使用
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    // kthread_stack->unused_retaddr = thread_destroy;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = \
    kthread_stack->esi = kthread_stack->edi = 0;
}

/* 初始化线程基本信息 */
void init_thread(struct task_struct* pthread, const char* name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->pid = allocate_pid();
    if(pthread == main_thread)
        pthread->status = TASK_RUNNING;
    else
        pthread->status = TASK_READY;
    pthread->ticks = prio;
    pthread->priority = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
    pthread->self_kstack = (uint8_t*)pthread + PG_SIZE;        // 线程自己在内核态下使用的栈顶地址
    pthread->stack_magic = 0x19870916;
}

/* 创建一个线程，并将其加入就绪队列 */
struct task_struct* thread_start
(
    const char* name,
    int prio,
    thread_func function,
    void* func_arg
)
{
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    enum intr_status old_status = intr_disable();
    ASSERT(!have_elem(&thread_ready_list, &thread->general_tag));           // 链表更新已为原子操作，此处不必担心调度影响任务队列更新
    list_append(&thread_ready_list, &thread->general_tag);
    ASSERT(!have_elem(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    intr_set_status(old_status);
    return thread;
}

/* 设置当前线程为主线程 */
static void make_main_thread()
{
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);
    enum intr_status old_status = intr_disable();
    ASSERT(!have_elem(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
    intr_set_status(old_status);
}

/* 线程、进程调度函数，若不是因时间片用完而调度，需要在阻塞函数中给出线程状态 */
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
        // 任务需等待某事件发生后才能继续上cpu执行
        ASSERT(!have_elem(&thread_ready_list, &cur->general_tag));
        cur->ticks = cur->priority;
    }
    if(list_isEmpty(&thread_ready_list))
        thread_unblock(idle_thread);
    thread_tag = NULL;
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
    process_activate(next);     // 切换进程页表、ss0
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

/* 主动让出cpu资源 */
void thread_yield()
{
    enum intr_status old_status = intr_disable();
    struct task_struct *cur = running_thread();
    cur->status = TASK_READY;
    schedule();
    intr_set_status(old_status);
}

static void idle(void *arg)
{
    while(1)
    {
        thread_block(TASK_BLOCKED);
        // 执行hlt之前用sti开中断，确保可被外部中断唤醒
        asm volatile ("sti; hlt" ::: "memory");
    }
}

void thread_init()
{
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    lock_init(&pid_lock);
    make_main_thread();
    idle_thread = thread_start("idle", 10, idle, NULL);
    put_str("thread_init done\n");
}