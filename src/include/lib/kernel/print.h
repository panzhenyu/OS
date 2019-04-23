#ifndef _LIB_KERNEL_PRINT_H
#define _LIB_KERNEL_PRINT_H

#include "stdint.h"
#include "stdio.h"

void set_cursor(uint16_t);
void put_char(uint8_t char_ascii);
void put_str(const char* str);
void put_int32(int32_t num);
void put_uint32_hex(uint32_t unum);
int printk(const char *format, ...);

#endif
