#include "console.h"
#include "sync.h"

static struct lock console_lock;

void console_init()
{
    lock_init(&console_lock);
}

void console_acquire()
{
    lock_acquire(&console_lock);
}

void console_release()
{
    lock_release(&console_lock);
}

void console_put_char(uint8_t char_ascii)
{
    console_acquire();
    put_char(char_ascii);
    console_release();
}

void console_put_str(const char* str)
{
    console_acquire();
    put_str(str);
    console_release();
}

void console_put_uint32t(uint32_t number_hex)
{
    console_acquire();
    put_uint32_hex(number_hex);
    console_release();
}