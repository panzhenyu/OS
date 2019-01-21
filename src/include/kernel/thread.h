#ifndef _KERNEL_THREAD_H
#define _KERNEL_THREAD_H

#include "stdint.h"

typedef void thread_func(void*);            // 函数类型（并非函数指针类型，使用函数指针需采用thread_func*声明）

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
    uint32_t* self_kstack;          // 各内核线程的内核栈
    enum task_status status;        // 线程状态
    uint8_t priority;               // 线程优先级
    char name[16];                  // 线程名
    uint32_t stack_magic;           // 栈边界标记，用于检测栈溢出
};


#endif