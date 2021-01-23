#ifndef _LIB_STRING_H
#define _LIB_STRING_H

#include "stdint.h"

//初始化dst起始处连续的size个大小的内存
void memset(void* dst, uint8_t value, uint32_t size);

//将src起始处连续的size个大小的内存复制给dst
void memcpy(void* dst, const void* src, uint32_t size);

//比较m1与m2起始处连续的size个内存的大小，m1 > m2时返回1，相等时返回0，否则返回-1
int memcmp(const void* m1, const void* m2, uint32_t size);

//将字符串从src复制到dst
char* strcpy(char* dst, const char* src);

//比较两个字符串，s1中的字符大于s2中的字符时返回1，相等时返回0，否则返回-1
int strcmp(const char* s1, const char* s2);

//将src拼接到dst后
char* strcat(char* dst, const char* src);

//从左到右查找str中首次出现ch的地址
char* strchr(const char* str, uint8_t ch);

//从右向左查找str中首次出现ch的地址
char* strrchr(const char* str, uint8_t ch);

//返回字符串中ch出现的次数
uint32_t strchrs(const char* str, uint8_t ch);

//返回字符串长度
uint32_t strlen(const char* str);

#endif
