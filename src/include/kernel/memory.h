#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H

#include "bitmap.h"

#define PG_P_1		1			// 存在位
#define PG_P_0		0			// 存在位
#define PG_RW_R		0			// R/W属性位，读/执行
#define PG_RW_W		2			// R/W属性位，读/写/执行
#define PG_US_S		0			// U/S属性位，系统级
#define PG_US_U		4			// U/S属性位，用户级
#define PAGE_DIR_KERNEL_VADDR 0xFFFFF000
#define PDE_KERNEL_BASEITEM 768
#define PDE_USER_BASEITEM 0
#define PDE_MAXLEN 1023
#define PDE_ITEMLEN 4
#define PTE_MAXLEN 1023
#define PTE_ITEMLEN 4

/* 虚拟地址池 */
struct virtual_addr
{
	struct bitmap vaddr_bitmap;		// 位图结构管理
	uint32_t vaddr_start;			// 虚拟地址起始地址
};

enum pool_flags
{
	PF_KERNEL = 1,				// 内核内存池
	PF_USER = 2					// 用户内存池
};

extern struct pool kernel_pool, user_pool;

void mem_init();
uint32_t* pde_ptr(uint32_t vaddr);
uint32_t* pte_ptr(uint32_t vaddr);
void* get_kernel_pages(uint32_t pg_cnt);
void* get_user_pages(uint32_t pg_cnt);
bool get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);

#endif