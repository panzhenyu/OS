#include "process.h"
#include "tss.h"
#include "thread.h"

extern void intr_exit(void);

void process_start(void *filename_)
{
    void *function = filename_;
    struct task_struct *cur = running_thread();
    cur->self_kstack += sizeof(struct thread_stack);
    struct intr_stack *proc_stack = (struct intr_stack*)cur->self_kstack;
    proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
    proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
    proc_stack->eip = function;
    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE);
    proc_stack->ss = SELECTOR_U_STACK;
    asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (proc_stack) : "memory");
}

void page_dir_activate(struct task_struct *pthread)
{
    // 内核页目录表物理基址
    uint32_t pagedir_phy_addr = 0x100000;
    if(pthread->pgdir != NULL)
        pagedir_phy_addr = addr_v2p(pthread->pgdir);
    asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory");
}

void process_activate(struct task_struct *pthread)
{
    ASSERT(pthread != NULL);
    page_dir_activate(pthread);
    // 若是线程则无需更新ss0
    if(pthread->pgdir)
        update_tss_esp(pthread);
}