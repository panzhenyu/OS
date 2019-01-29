#include "timer.h"
#include "io.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"
#include "thread.h"

#define IRQ0_FREQUENCY		100
#define INPUT_FREQUENCY		1193180
#define COUNTER0_VALUE		INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT		0x40
#define COUNTER0_NO		0
#define COUNTER_MODE		2			// 比特发生器工作模式
#define READ_WRITE_LATCH	3			// 先读写低字节，后读写高字节
#define PIT_COUNTROL_PORT	0x43

uint32_t ticks = 0;						// 用于记录内核自中断开启以来总共的嘀嗒数

static void frequency_set
(
	uint8_t counter_port,
	uint8_t counter_no,
	uint8_t rwl,
	uint8_t counter_mode,
	uint16_t counter_value
)
{
	uint8_t ctrl_w = (counter_no << 6) | (rwl << 4) | (counter_mode << 1);
	outb(PIT_COUNTROL_PORT, ctrl_w);			// 写入控制字
	outb(counter_port, counter_value);			// 先写入低字节
	outb(counter_port, counter_value >> 8);		// 后写入高字节
}

static void intr_timer_handler()
{
	struct task_struct* cur_thread = running_thread();
	ASSERT(cur_thread->stack_magic == 0x19870916);				// 检测栈溢出
	++cur_thread->elapsed_ticks;
	++ticks;
	if(cur_thread->ticks == 0)
		schedule();
	else
		--cur_thread->ticks;
}

void timer_init()
{
	put_str("timer_init start\n");
	frequency_set(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
	register_handler(0x20, intr_timer_handler);
	put_str("timer_init done\n");
}
