#include "memory.h"
#include "print.h"

#define PG_SIZE 4096

/*内核主进程栈顶为0xc009f000
 *将来主进程PCB占用一个页，其基地址为0xc009e000
 *位图基址从0xc009a000开始，一直到0xc009e000，共四个位图可用*/
#define MEM_BITMAP_BASE 0xc009a000

/*0xc0000000是内核区起始虚拟地址，0xc0100000表示跨过1MB*/
#define K_HEAP_START 0xc0100000

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
	uint16_t all_free_pages = free_mem / PG_SIZE;

	uint16_t kernel_free_pages = all_free_pages / 2;
	uint16_t user_free_pages = all_free_pages - kernel_free_pages;
	
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
}
