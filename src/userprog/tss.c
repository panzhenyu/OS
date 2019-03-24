#include "global.h"
#include "print.h"
#include "string.h"
#include "tss.h"

/* tss结构体 */
struct tss
{
    uint32_t backlink;
    uint32_t *esp0;
    uint32_t ss0;
    uint32_t *esp1;
    uint32_t ss1;
    uint32_t *esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t (*eip) (void);
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trace;
    uint32_t iobase;
};

static struct tss tss;

/* 更新tss中的esp0为pthread的0级栈 */
void update_tss_esp(struct task_struct* pthread)
{
    tss.esp0 = (uint32_t*)((uint32_t)pthread + PG_SIZE);    // 将PCB尾部当做0级栈顶
}

/* 创建GDT描述符 */
static struct gdt_desc make_gdt_desc(uint32_t *desc_addr, uint32_t limit, uint8_t attr_low, uint8_t attr_high)
{
    uint32_t desc_base = (uint32_t)desc_addr;
    struct gdt_desc desc;   // 局部变量可能产生意外
    desc.limit_low_word = limit & 0x0000ffff;
    desc.base_low_word = desc_base & 0x0000ffff;
    desc.base_mid_bype = ((desc_base & 0x00ff0000) >> 16);
    desc.attr_low_byte = attr_low;
    desc.limit_high_attr_high = (((limit & 0x000f0000) >> 16) + (uint8_t)attr_high);
    desc.base_high_byte = desc_base >> 24;
    return desc;
}

void tss_init()
{
    put_str("tss_init start\n");
    uint32_t tss_size = sizeof(tss);
    memset(&tss, 0, tss_size);
    tss.ss0 = SELECTOR_K_STACK;
    tss.iobase = tss_size;      // 表示无io位图
    // 0x903为gdt基址，由于已经开启了分页模式，因此采用虚拟地址访存
    *((struct gdt_desc*)0xc0000923) = make_gdt_desc((uint32_t*)&tss, tss_size - 1, TSS_ATTR_LOW, TSS_ATTR_HIGH);
    *((struct gdt_desc*)0xc000092b) = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_CODE_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
    *((struct gdt_desc*)0xc0000933) = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_DATA_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
    uint64_t gdt_opegrand = ((8 * 7 - 1) | ((uint64_t)(uint32_t)0xc0000903 << 16));
    asm volatile ("lgdt %0" : : "m" (gdt_opegrand));    // 修正gdt
    asm volatile ("ltr %w0" : : "r" (SELECTOR_TSS));    // 加载tss选择子至tr寄存器
    put_str("tss_init and ltr done\n");
}