#include "process.h"
#include "tss.h"
#include "memory.h"
#include "console.h"
#include "string.h"
#include "interrupt.h"
#include "debug.h"

extern void intr_exit(void);

/* 用户进程入口函数 */
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
    // 调度之后发生的申请，此时页表已改变
    ASSERT(get_a_page(PF_USER, USER_STACK3_VADDR));
    proc_stack->esp = (void*)USER_STACK3_VADDR + PG_SIZE;
    proc_stack->ss = SELECTOR_U_STACK;
    asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (proc_stack) : "memory");
}

/* 切换进程页表 */
void page_dir_activate(const struct task_struct *pthread)
{
    // 内核页目录表物理基址
    uint32_t pagedir_phy_addr = 0x100000;
    if(pthread->pgdir != NULL)
        pagedir_phy_addr = addr_v2p((uint32_t)pthread->pgdir);
    asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory");
}

/* 切换进程页表并更新ss0 */
void process_activate(const struct task_struct *pthread)
{
    ASSERT(pthread != NULL);
    page_dir_activate(pthread);
    // 若是线程则无需更新ss0
    if(pthread->pgdir)
        update_tss_esp(pthread);
}

/* 创建用户进程页表并返回虚拟地址，以便修改页表项 */
uint32_t* create_page_dir(void)
{
    uint32_t *base = (uint32_t*)PAGE_DIR_KERNEL_VADDR;
    uint32_t *page_dir_vaddr = (uint32_t*)get_kernel_pages(1);
    if(page_dir_vaddr == NULL)
    {
        console_put_str("create_page_dir: get_kernel_pages failed!\n");
        return NULL;
    }
    uint32_t size = (PDE_MAXLEN - PDE_KERNEL_BASEITEM) * PDE_ITEMLEN;
    memcpy(page_dir_vaddr + PDE_KERNEL_BASEITEM, base + PDE_KERNEL_BASEITEM, size);     // 复制768-1022项，共255项
    uint32_t phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
    page_dir_vaddr[PDE_MAXLEN] = phy_addr | PG_US_U | PG_RW_W | PG_P_1;                 // 1023项指向页目录表
    return page_dir_vaddr;
}

void create_user_vaddr_bitmap(struct task_struct *user_prog)
{
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
    uint32_t bitmap_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = bitmap_bytes_len;
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages((bitmap_bytes_len + PG_SIZE - 1) / PG_SIZE);
    bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}

void process_execute(void *filename, const char *name, int prio)
{
    struct task_struct *user_prog = get_kernel_pages(1);
    init_thread(user_prog, name, prio);
    create_user_vaddr_bitmap(user_prog);
    thread_create(user_prog, process_start, filename);
    user_prog->pgdir = create_page_dir();
    block_desc_init(user_prog->u_block_descs);

    enum intr_status old_status = intr_disable();
    ASSERT(!have_elem(&thread_ready_list, &user_prog->general_tag));
    list_append(&thread_ready_list, &user_prog->general_tag);
    ASSERT(!have_elem(&thread_all_list, &user_prog->all_list_tag));
    list_append(&thread_all_list, &user_prog->all_list_tag);
    intr_set_status(old_status);
}