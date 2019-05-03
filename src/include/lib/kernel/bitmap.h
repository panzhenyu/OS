#ifndef _LIB_KERNEL_BITMAP_H
#define _LIB_KERNEL_BITMAP_H

#include "global.h"

#define BITMAP_MASK 1
#define DIV_ROUND_UP(a, b) ((a) % (b) == 0 ? (a) / (b) : (a) / (b) + 1)

struct bitmap
{
	uint32_t btmp_bytes_len;	// 位图字节数
	uint8_t* bits;				// 位图
};

void bitmap_init(struct bitmap* btmp);									// 初始化位图，0表未占用
int bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx);			// 返回bit_idx处的值
int bitmap_scan(struct bitmap* btmp, uint32_t cnt);						// 申请连续的cnt个位，成功返回起始下标，失败返回-1
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value);	// 设置bit_idx位为value
int8_t bitmap_get(struct bitmap* btmp, uint32_t bit_idx);				// 获取bit_idx位

#endif
