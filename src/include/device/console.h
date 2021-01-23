#ifndef _DEVICE_CONSOLE_H
#define _DEVICE_CONSOLE_H

#include "print.h"
#include "stdint.h"

void console_init();
void console_acquire();
void console_release();
void console_put_char(uint8_t char_ascii);
void console_put_str(const char* str);
void console_put_uint32t(uint32_t number_hex);

#endif