#ifndef _THREAD_THREAD_H
#define _THREAD_THREAD_H

#include "list.h"
#include "memory.h"

typedef void thread_func(void*);            // 函数类型（并非函数指针类型，使用函数指针需采用thread_func*声明）
typedef int16_t pid_t;

enum task_status
{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

struct intr_stack
{
    uint32_t vec_no;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;         // pushad会将esp压入，但是esp是不断变化的，因此会被popad忽略
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t err_code;
    void (*eip)();
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;
};

struct thread_stack
{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;
    void (*eip) (thread_func* func, void* func_arg);
    void* unused_retaddr;
    thread_func* function;
    void* func_arg;
};

/* 进程或线程的pcb */
struct task_struct
{
    uint8_t* self_kstack;                                       // 各内核线程的内核栈，位置不可变
    pid_t pid;                                                  // 进程pid
    enum task_status status;                                    // 线程状态
    char name[16];                                              // 线程名
    uint8_t priority;                                           // 线程优先级
    uint8_t ticks;                                              // 每次在处理器上执行的时间嘀嗒数
    uint32_t elapsed_ticks;                                     // 任务执行的总时间数
    struct list_elem general_tag;                               // 线程在一般队列中的结点
    struct list_elem all_list_tag;                              // 线程在线程队列thread_all_list中的结点
    struct virtual_addr userprog_vaddr;                         // 用户进程虚拟地址池
    struct mem_block_desc u_block_descs[BLOCK_DESC_CNT];        // 内存块描述符组
    uint32_t* pgdir;                                            // 进程页表的虚拟地址
    uint32_t stack_magic;                                       // 栈边界标记，用于检测栈溢出，位置不可变
};

extern struct list thread_ready_list;
extern struct list thread_all_list;

void thread_init();
void init_thread(struct task_struct* pthread, const char* name, int prio);
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg);
void schedule();
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct* pthread);
struct task_struct* running_thread();
struct task_struct* thread_start(const char* name, int prio, thread_func function, void* func_arg);

#endif