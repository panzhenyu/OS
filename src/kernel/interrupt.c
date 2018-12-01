#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"

#define PIC_M_CTRL 0x20				// 主片的控制端口
#define PIC_M_DATA 0x21				// 主片的数据端口
#define PIC_S_CTRL 0xa0				// 从片的控制端口
#define PIC_S_DATA 0xa1				// 从片的数据端口
#define IDT_DESC_CNT 0x21			// 目前总共支持的中断数

// 中断门结构体
struct gate_desc
{
	uint16_t func_offset_low_word;		// 中断处理程序入口地址低16位
	uint16_t selector;			// 中断处理程序段选择子
	uint8_t dcount;				// 门描述符中第四字节
	uint8_t attribute;			// 中断门属性
	uint16_t func_offset_high_word;		// 中断处理程序入口地址高16位
}

static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];		// 中断描述符表
extern intr_handler intr_entry_table[IDT_DESC_CNT];	// 中断程序入口地址数组

static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function)
{
	p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
	p_gdesc->selector = SELECTOR_K_CODE;
	p_gdesc->dcount = 0;
	p_gdesc->attribute = attr;
	p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

//初始化idt表
static void idt_desc_init()
{
	int i;
	for(i=0;i<IDT_DESC_CNT;i++)
		make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
}

//初始化8259A
static void pic_init()
{
	//初始化主片
	outb(PIC_M_CTRL, 0x11);			// ICW1: 边沿触发，级联8259，需要ICW4
	outb(PIC_M_DATA, 0x20);			// ICW2: 起始中断向量号为0x20

	outb(PIC_M_DATA, 0x04);			// ICW3: IR2接从片
	outb(PIC_M_DATA, 0x01);			// ICW4: 8086模式，正常EOI

	//初始化从片
	outb(PIC_S_CTRL, 0x11);			// ICW1: 边沿触发，级联8259，需要ICW4
	outb(PIC_S_DATA, 0x28);			// ICW2: 起始中断向量号为0x28

	outb(PIC_S_DATA, 0x02);			// ICW3: 设置从片连接到主片的IR2引脚
	outb(PIC_S_DATA, 0x01);			// ICW4: 8086模式，正常EOI

	//打开主片上的IR0，也就是目前只接受时钟产生的中断
	outb(PIC_M_DATA, 0xfe);
	outb(PIC_S_DATA, 0xff);

	put_str("   pic_init done\n");
}

void idt_init()
{
	put_str("idt_init start\n");
	idt_desc_init();			// 初始化中断描述符表
	pic_init();				// 初始化8295A
	uint64_t idt_operand = ((sizeof(idt)-1) | ((uint64_t)((uint32_t)idt << 16)));		// 低16位为表界限，高32位为表基址
	asm volatile("lidt %0" : : "m" (idt_operand));
	put_str("idt_init done\n");
}

