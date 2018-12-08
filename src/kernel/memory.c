#include "memory.h"
#include "print.h"
#include "stdint.h"
#include "bitmap.h"
#include "debug.h"
#include "string.h"
#include "global.h"

#define PG_SIZE 4096

/*内核主进程栈顶为0xc009f000
 *将来主进程PCB占用一个页，其基地址为0xc009e000
 *位图基址从0xc009a000开始，一直到0xc009e000，共四个位图可用*/
#define MEM_BITMAP_BASE 0xc009a000

/*0xc0000000是内核区起始虚拟地址，0xc0100000表示跨过1MB*/
#define K_HEAP_START 0xc0100000

#define PDE_IDX(vaddr) ((vaddr & 0xffc00000) >> 22)	// 返回虚拟地址高十位
#define PTE_IDX(vaddr) ((vaddr & 0x003ff000) >> 12)	// 返回虚拟地址中间十位
struct pool
{
	struct bitmap pool_bitmap;		// 内存池位图结构
	uint32_t phy_addr_start;		// 内存池所管理的物理内存起始地址
	uint32_t pool_size;			// 内存池字节容量
};

struct pool kernel_pool, user_pool;		// 生成内核内存池与用户内存池
struct virtual_addr kernel_vaddr;		// 给内核分配虚拟地址

static void mem_pool_init(uint32_t all_mem)
{
	put_str("   mem_pool_init start\n");
	uint32_t page_table_size = PG_SIZE * 256;
	uint32_t used_mem = page_table_size + (1 << 20);
	uint32_t free_mem = all_mem - used_mem;
	uint32_t all_free_pages = free_mem / PG_SIZE;
	uint32_t kernel_free_pages = all_free_pages / 2;
	uint32_t user_free_pages = all_free_pages - kernel_free_pages;
	
	uint32_t kbm_length = kernel_free_pages / 8;
	uint32_t ubm_length = user_free_pages / 8;
	
	uint32_t kp_start = used_mem;		// 内核内存池起始地址
	uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;	// 用户内存池起始地址
	
	kernel_pool.phy_addr_start = kp_start;
	user_pool.phy_addr_start = up_start;

	kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
	user_pool.pool_size = user_free_pages * PG_SIZE;

	kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
	user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

	kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;			// 暂定为0xc009a000
	user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);	// 紧跟内核位图

	put_str("      kernel_pool_bitmap_start:");
	put_uint32_hex((uint32_t)kernel_pool.pool_bitmap.bits);
	put_str("      kernel_pool_phy_addr_start:");
	put_uint32_hex(kernel_pool.phy_addr_start);
	put_char('\n');
	put_str("      user_pool_bitmap_start:");
	put_uint32_hex((uint32_t)user_pool.pool_bitmap.bits);
	put_str("      user_pool_phy_addr_start:");
	put_uint32_hex(user_pool.phy_addr_start);
	put_char('\n');

	bitmap_init(&kernel_pool.pool_bitmap);
	bitmap_init(&user_pool.pool_bitmap);

	kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
	kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);
	kernel_vaddr.vaddr_start = K_HEAP_START;
	bitmap_init(&kernel_vaddr.vaddr_bitmap);
	put_str("   mem_pool_init done\n");
}

void mem_init()
{
	put_str("mem_init start\n");
	uint32_t mem_bytes_total = (*(uint32_t*)(0xb03));
	mem_pool_init(mem_bytes_total);
	put_str("mem_init done\n");
}

uint32_t* pde_ptr(uint32_t vaddr)
{
	return (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);
}

uint32_t* pte_ptr(uint32_t vaddr)
{
	return (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
}

/*一次申请pg_cnt个虚拟页，申请失败返回NULL，申请成功返回起始虚拟地址*/
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
	int vaddr_start = 0, bit_idx_start = -1;
	uint32_t cnt = 0;
	if(pf == PF_KERNEL)
	{
		bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
		if(bit_idx_start == -1)
			return NULL;
		while(cnt < pg_cnt)
			bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
		vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
	}
	else
	{
	}
	return (void*)vaddr_start;
}

/*一次申请一个物理页，申请失败返回NULL，申请成功返回页的起始物理地址*/
static void* paddr_get(struct pool* m_pool)
{
	int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
	if(bit_idx == -1)
		return NULL;
	bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
	uint32_t page_phyaddr = m_pool->phy_addr_start + bit_idx * PG_SIZE;
	return (void*)page_phyaddr;
}
