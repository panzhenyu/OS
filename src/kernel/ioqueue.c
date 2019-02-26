#include "ioqueue.h"
#include "console.h"
#include "thread.h"

struct ioqueue sys_ioqueue;
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
    ioq_init(&sys_ioqueue);
    iq = &sys_ioqueue;
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
    uint32_t old_len = 0;
    iq = piq;
    while(1)
    {
        if(old_len >= piq->length)
            continue;
        console_put_char(piq->ascii[old_len]);
        if(piq->ascii[old_len++] == '\r')
            break;
    }
    iq = old_ioq;
    piq->length = old_len;
}

uint8_t getchar()
{
    struct ioqueue* cur = running_thread()->input_buff;
    if(cur->length == 0)
        io_getchar(cur);
    lock_acquire(&cur->iolock);
    uint8_t ans = cur->ascii[0]; int i;
    for(i=1;i<cur->length;i++)
        cur->ascii[i-1] = cur->ascii[i];
    cur->length--;
    lock_release(&cur->iolock);
    return ans;
}

// char* io_getline()
// {
    
// }