#ifndef _LIB_KERNEL_PRINT_H
#define _LIB_KERNEL_PRINT_H

#include "stdint.h"

void put_char(uint8_t char_asci);
void put_str(char* str_asci);
void put_int32(int32_t num);
void put_uint32_hex(uint32_t unum);

#endif
