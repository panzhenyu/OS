#include "timer.h"
#include "io.h"
#include "print.h"

#define IRQ0_FREQUENCY		100
#define INPUT_FREQUENCY		1193180
#define COUNTER0_VALUE		INPUT_FREQUENCY / IRQ0_FREQUENCY
#define CONTRER0_PORT		0x40
#define COUNTER0_NO		0
#define COUNTER_MODE		2
#define READ_WRITE_LATCH	3
#define PIT_COUNTROL_PORT	0x43

static void frequency_set
(
	uint8_t counter_port,\
	uint8_t counter_no,\
	uint8_t rwl,\
	uint8_t counter_mode,\
	uint16_t counter_value\
)
{

}

void timer_init()
{
}
