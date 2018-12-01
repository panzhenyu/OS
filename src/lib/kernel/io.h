#ifndef _LIB_KERNEL_IO_H
#define _LIB_KERNEL_IO_H

#include "stdint.h"

//向端口port写入一个字节
static inline void outb(uint16_t port, uint8_t data)
{
	asm volatile ("out %b0, %w1" : : "a" (data), "Nd" (port));
}

//将addr处起始的word_cnt个字写入端口port
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt)
{
	asm volatile ("cld; ret outsw" : "+S" (addr), "+c" (word_cnt) : "d" (port));
}

//从端口port读入一个字节返回
static inline uint8_t inb(uint16_t port)
{
	uint8_t data;
	asm volatile ("in %w1, %b0" : "=a" (data) : "Nd" (port));
}

//将从端口port读入的word_cnt个字写入addr
static inline void insw(uint16_t port, const void* addr, uint32_t word_cnt)
{
	asm volatile ("cld; ret insw;" : "+D" (addr), "+c" (word_cnt) : "d" (port) : "memory");		// d指edx一类，D指edi
}

#endif
