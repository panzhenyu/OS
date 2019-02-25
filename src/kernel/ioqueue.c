#include "ioqueue.h"
#include "print.h"
#include "console.h"

static struct ioqueue sysiq;
static struct ioqueue* iq;

void ioq_init(struct ioqueue* piq)
{
    piq->length = 0;
    piq->size = IOQUEUE_INIT_SIZE;
    lock_init(&piq->iolock);
}

void ioqueue_init()
{
    put_str("ioqueue_init start\n");
    ioq_init(&sysiq);
    iq = &sysiq;
    running_thread()->input_buff = &sysiq;              // 设置main线程的input_buff为系统io队列
    put_str("ioqueue_init done\n");
}

void io_input(uint8_t ascii)
{
    lock_acquire(&iq->iolock);
    if(iq->length == iq->size)                          // 需改进
        iq->length = 0;
    iq->ascii[iq->length++] = ascii;
    lock_release(&iq->iolock);
}

void io_getchar(struct ioqueue* piq)
{
    struct ioqueue* old_ioq = iq;
    uint32_t old_len = piq->length;
    iq = piq;
    while(old_len < piq->length && piq->ascii[++old_len] != '\n');
    iq = &sysiq;
    piq->length = old_len;
}

// char* io_getline()
// {
    
// }