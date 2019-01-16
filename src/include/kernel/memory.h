#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"

#define PG_P_1		1			// 存在位
#define PG_P_0		0			// 存在位
#define PG_RW_R		0			// R/W属性位，读/执行
#define PG_RW_W		2			// R/W属性位，读/写/执行
#define PG_US_S		0			// U/S属性位，系统级
#define PG_US_U		4			// U/S属性位，用户级

//虚拟地址池
struct virtual_addr
{
	struct bitmap vaddr_bitmap;		// 位图结构管理
	uint32_t vaddr_start;			// 虚拟地址起始地址
};

enum pool_flags
{
	PF_KERNEL = 1,				// 内核内存池
	PF_USER = 2				// 用户内存池
};

extern struct pool kernel_pool, user_pool;

void mem_init();
uint32_t* pde_ptr(uint32_t vaddr);
uint32_t* pte_ptr(uint32_t vaddr);
void* malloc_page(enum pool_flags pf, uint32_t cnt);
void* get_kernel_pages(uint32_t pg_cnt);

#endif
