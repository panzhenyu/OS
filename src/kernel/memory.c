#include "print.h"
#include "stdint.h"
#include "debug.h"
#include "string.h"
#include "global.h"
#include "thread.h"
#include "interrupt.h"
#include "sync.h"

/* 内核主进程栈顶为0xc009f000
 * 内核主进程PCB占用一个页，其基地址为0xc009e000
 * 位图基址从0xc009a000开始，一直到0xc009e000，共四个位图可用 */
#define MEM_BITMAP_BASE 0xc009a000

/* 0xc0000000是内核区起始虚拟地址，0xc0100000表示跨过1MB */
#define K_HEAP_START 0xc0100000

#define PDE_IDX(vaddr) ((vaddr & 0xffc00000) >> 22)	// 返回虚拟地址高十位
#define PTE_IDX(vaddr) ((vaddr & 0x003ff000) >> 12)	// 返回虚拟地址中间十位

/* 内存池 */
struct pool
{
	struct lock lock;
	struct bitmap pool_bitmap;		// 内存池位图结构
	uint32_t phy_addr_start;		// 内存池所管理的物理内存起始地址
	uint32_t pool_size;				// 内存池字节容量
};

/* 内存块 */
struct mem_block
{
	struct list_elem free_elem;
};

struct arena
{
	struct mem_block_desc *desc;	// arena绑定的内存块描述符
	uint32_t cnt;					// large为1时cnt代表页框数，为0时cnt代表空闲的内存块数
	bool large;
};

struct mem_block_desc k_block_descs[BLOCK_DESC_CNT];	// 内核内存块描述符组
struct pool kernel_pool, user_pool;						// 内核内存池与用户内存池
struct virtual_addr kernel_vaddr;						// 内核虚拟地址池

/* 初始化内核内存池、用户内存池 */
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
	
	uint32_t kp_start = used_mem;								// 内核内存池起始地址
	uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;	// 用户内存池起始地址
	
	kernel_pool.phy_addr_start = kp_start;
	user_pool.phy_addr_start = up_start;

	kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
	user_pool.pool_size = user_free_pages * PG_SIZE;

	kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
	user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

	kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;					// 暂定为0xc009a000
	user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);		// 紧跟内核位图

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

	lock_init(&kernel_pool.lock);
	lock_init(&user_pool.lock);

	kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
	kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);
	// 内核虚拟地址池的基地址必须要用户进程内核区之中，否则在用户进程调度至另一用户进程时将无法将pcb的pgdir转换成对应的物理地址
	kernel_vaddr.vaddr_start = K_HEAP_START;
	bitmap_init(&kernel_vaddr.vaddr_bitmap);

	put_str("   mem_pool_init done\n");
}

/* 初始化内存块描述符组 */
void block_desc_init(struct mem_block_desc* desc_array)
{
	if(desc_array == NULL)
		return;
	int i, block_size = 16;
	for(i = 0 ; i < BLOCK_DESC_CNT ; i++)
	{
		desc_array[i].block_size = block_size;
		desc_array[i].blocks_per_arena = (PG_SIZE - sizeof(struct arena)) / block_size;
		list_init(&desc_array[i].free_list);
		block_size *= 2;
	}
}

/* 初始化内存管理系统 */
void mem_init()
{
	put_str("mem_init start\n");
	uint32_t mem_bytes_total = (*(uint32_t*)(0xb03));
	mem_pool_init(mem_bytes_total);
	block_desc_init(k_block_descs);
	put_str("mem_init done\n");
}

/* 获取虚拟地址vaddr的页目录项指针 */
uint32_t* pde_ptr(uint32_t vaddr)
{
	return (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);
}

/* 获取虚拟地址vaddr的页表项指针 */
uint32_t* pte_ptr(uint32_t vaddr)
{
	return (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
}

/* 根据当前页表将虚拟地址vaddr转换为对应的物理地址 */
uint32_t addr_v2p(uint32_t vaddr)
{
	uint32_t *pte = pte_ptr(vaddr);
	return ((*pte & 0xfffff000) | (vaddr & 0x00000fff));
}

/* 一次申请pg_cnt个虚拟页，申请失败返回NULL，申请成功返回起始虚拟地址 */
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
		struct task_struct *cur = running_thread();
		bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);
		if(bit_idx_start == -1)
			return NULL;
		while(cnt < pg_cnt)
			bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
		vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
	}
	return (void*)vaddr_start;
}

/* 释放虚拟地址池中以vaddr为起始地址的连续pg_cnt个页 */
static void vaddr_remove(enum pool_flags pf, uint32_t vaddr, uint32_t pg_cnt)
{
	struct task_struct *cur = running_thread();
	struct virtual_addr *vmem_pool = pf == PF_KERNEL ? &kernel_vaddr : &cur->userprog_vaddr;
	uint32_t bit_idx = (vaddr - vmem_pool->vaddr_start) / PG_SIZE;
	while(pg_cnt--)
		bitmap_set(&vmem_pool->vaddr_bitmap, bit_idx++, 0);
}

/* 一次申请一个物理页，申请失败返回NULL，申请成功返回页的起始物理地址 */
static void* palloc(struct pool* m_pool)
{
	lock_acquire(&m_pool->lock);
	int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
	if(bit_idx == -1)
		return NULL;
	bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
	uint32_t page_phyaddr = m_pool->phy_addr_start + bit_idx * PG_SIZE;
	lock_release(&m_pool->lock);
	return (void*)page_phyaddr;
}

/* 释放以paddr为起始地址的物理内存页 */
static void pfree(struct pool *m_pool, uint32_t paddr)
{
	uint32_t bit_idx = (paddr - m_pool->phy_addr_start) / PG_SIZE;
	bitmap_set(&m_pool->pool_bitmap, bit_idx, 0);
}

/* 用于做虚拟页到物理页的映射 */
static void page_table_add(void* _vaddr, void* _page_phyaddr)
{
	uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
	uint32_t* pde = pde_ptr(vaddr);
	uint32_t* pte = pte_ptr(vaddr);
	// 判别pde是否存在，第0位为存在位，若不存在，则在内核空间分配一页当做页表
	if(*pde & 0x00000001)
	{
		ASSERT(!(*pte & 0x00000001));
		if(*pte & 0x00000001)
			PANIC("pte repeat");
		*pte = page_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
	}
	else
	{
		uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
		if(pde_phyaddr == NULL)
			return;
		*pde = pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
		memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
		ASSERT(!(*pte & 0x00000001));
		*pte = page_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
	}
}

/* 删除虚拟地址vaddr在页表中的映射关系 */
void page_table_pte_remove(uint32_t vaddr)
{
	uint32_t *pte = pte_ptr(vaddr);
	*pte &= ~PG_P_1;
	uint32_t pagedir_phy_addr = 0x100000;
	struct task_struct *pthread = running_thread();
    if(pthread->pgdir != NULL)
        pagedir_phy_addr = addr_v2p((uint32_t)pthread->pgdir);
    asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory");	// 更新tlb
	// asm volatile ("invlpg %0" :: "m" (vaddr) : "memory");
}

/* 申请cnt个连续的物理内存页，返回虚拟首地址 */
static void* malloc_page(enum pool_flags pf, uint32_t cnt)
{
	struct pool* mem_pool = pf == PF_KERNEL ? &kernel_pool : &user_pool;
	void* vaddr_start = vaddr_get(pf, cnt);
	if(vaddr_start == NULL)
		return NULL;
	uint32_t vaddr = (uint32_t)vaddr_start;
	void* page_phyaddr;
	while(cnt-- > 0)
	{
		page_phyaddr = palloc(mem_pool);
		if(page_phyaddr == NULL)
		{
			// 回滚所有已分配的页
			return NULL;
		}
		page_table_add((void*)vaddr, page_phyaddr);
		vaddr += PG_SIZE;
	}
	return vaddr_start;
}

/* 获取连续的pg_cnt个内核物理页，返回虚拟首地址 */
void* get_kernel_pages(uint32_t pg_cnt)
{
	lock_acquire(&kernel_pool.lock);
	void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if(vaddr != NULL)
		memset(vaddr, 0, PG_SIZE * pg_cnt);
	lock_release(&kernel_pool.lock);
	return vaddr;
}

/* 获取连续的pg_cnt个用户物理页，返回虚拟首地址 */
void* get_user_pages(uint32_t pg_cnt)
{
	lock_acquire(&user_pool.lock);
	void* vaddr = malloc_page(PF_USER, pg_cnt);
	if(vaddr != NULL)
		memset(vaddr, 0, PG_SIZE * pg_cnt);
	lock_release(&user_pool.lock);
	return vaddr;
}

/* 获取以虚拟地址vaddr为起始地址的内存页 */
bool get_a_page(enum pool_flags pf, uint32_t vaddr)
{
	struct pool *mem_pool = pf == PF_KERNEL ? &kernel_pool : &user_pool;
	lock_acquire(&mem_pool->lock);
	struct task_struct* cur = running_thread();
	int32_t bit_idx = -1;
	if(cur->pgdir == NULL && pf == PF_KERNEL)
		bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
	else if(cur->pgdir != NULL && pf == PF_USER)
		bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
	else
		PANIC("get_a_page: not allow kernel alloc userspace or user alloc kernelspace by get_a_page\n");
	// 判断该位是否合法或是否已被使用
	if(bit_idx < 0 || bitmap_get(&cur->userprog_vaddr.vaddr_bitmap, bit_idx))
		bit_idx = -1;
	else
	{
		bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx, 1);
		void *page_phyaddr = palloc(mem_pool);
		page_table_add((void*)vaddr, page_phyaddr);
	}
	lock_release(&mem_pool->lock);
	return bit_idx < 0 ? FALSE : TRUE;
}

/* 释放以虚拟地址vaddr为起始地址的连续的pg_cnt个内存页 */
void free_pages(enum pool_flags pf, uint32_t _vaddr, uint32_t _pg_cnt)
{
	// 页基址低12位为0
	ASSERT((_vaddr % PG_SIZE) == 0);
	uint32_t vaddr = _vaddr, pg_cnt = _pg_cnt;
	struct pool *mem_pool = pf == PF_KERNEL ? &kernel_pool : &user_pool;
	while(pg_cnt-- > 0)
	{
		pfree(mem_pool, addr_v2p(vaddr));
		page_table_pte_remove(vaddr);
		vaddr += PG_SIZE;
	}
	vaddr_remove(pf, _vaddr, _pg_cnt);
}

/* 返回arena中第idx个内存块的指针 */
static struct mem_block* arena2block(struct arena *a, uint32_t idx)
{
	return (struct mem_block*)((uint32_t)a + sizeof(struct arena) + a->desc->block_size * idx);
}

/* 返回内存块b所在的arena地址 */
static struct arena* block2arena(struct mem_block *b)
{
	return (struct arena*)((uint32_t)b & 0xFFFFF000);
}

/* 为进程或线程申请size字节空间，申请成功返回非零首地址，申请失败返回NULL */
void* sys_malloc(uint32_t size)
{
	enum pool_flags pf;
	struct pool *mem_pool;
	struct mem_block_desc *descs;
	struct task_struct *cur = running_thread();
	if(cur->pgdir == NULL)
	{
		pf = PF_KERNEL;
		mem_pool = &kernel_pool;
		descs = k_block_descs;
	}
	else
	{
		pf = PF_USER;
		mem_pool = &user_pool;
		descs = cur->u_block_descs;
	}
	if(size <= 0 || size > mem_pool->pool_size)
		return NULL;

	struct arena *a;
	struct mem_block *b;
	lock_acquire(&mem_pool->lock);
	if(size > 1024)
	{
		uint32_t pg_cnt = (size + sizeof(struct arena) - 1) / PG_SIZE + 1;
		switch(pf)
		{
			case PF_KERNEL:
				a = get_kernel_pages(pg_cnt);
				break;
			case PF_USER:
				a = get_user_pages(pg_cnt);
				break;
		}
		if(!a)
		{
			lock_release(&mem_pool->lock);
			return NULL;
		}
		a->desc = NULL;
		a->cnt = pg_cnt;
		a->large = 1;
		lock_release(&mem_pool->lock);
		return (void*)(a + 1);
	}
	else
	{
		uint8_t desc_idx;
		for(desc_idx = 0 ; desc_idx < BLOCK_DESC_CNT ; desc_idx++)
			if(descs[desc_idx].block_size >= size)
				break;
		if(list_isEmpty(&descs[desc_idx].free_list))
		{
			switch(pf)
			{
				case PF_KERNEL:
					a = get_kernel_pages(1);
					break;
				case PF_USER:
					a = get_user_pages(1);
					break;
			}
			if(!a)
			{
				lock_release(&mem_pool->lock);
				return NULL;
			}
			a->desc = &descs[desc_idx];
			a->cnt = descs[desc_idx].blocks_per_arena;
			a->large = 0;
			uint32_t block_idx;
			for(block_idx = 0 ; block_idx < descs[desc_idx].blocks_per_arena ; block_idx++)
			{
				b = arena2block(a, block_idx);
				// b块空闲时，前8字节用于存放链表信息
				ASSERT(!have_elem(&descs[desc_idx].free_list, &b->free_elem));
				list_append(&descs[desc_idx].free_list, &b->free_elem);
			}
		}
		b = elem2entry(struct mem_block, free_elem, list_pop(&descs[desc_idx].free_list));
		a = block2arena(b);
		memset(b, 0, descs[desc_idx].block_size);
		--a->cnt;
		lock_release(&mem_pool->lock);
		return (void*)b;
	}
}

static void free_arena(enum pool_flags pf, struct arena *a)
{
	ASSERT(a->cnt == a->desc->blocks_per_arena);
	struct mem_block *b;
	struct mem_block_desc *desc = a->desc;
	uint32_t block_idx;
	for(block_idx = 0 ; block_idx < desc->blocks_per_arena ; block_idx++)
	{
		b = arena2block(a, block_idx);
		ASSERT(have_elem(&desc->free_list, &b->free_elem));
		list_remove(&b->free_elem);
	}
	free_pages(pf, (uint32_t)a, 1);
}

void sys_free(void *vaddr)
{
	ASSERT(vaddr != NULL);
	if(vaddr == NULL)
		return;
	enum pool_flags pf;
	struct arena *a;
	struct mem_block *b;
	struct pool *mem_pool;
	struct mem_block_desc *desc;
	struct task_struct *cur = running_thread();
	if(cur->pgdir == NULL)
	{
		pf = PF_KERNEL;
		mem_pool = &kernel_pool;
	}
	else
	{
		pf = PF_USER;
		mem_pool = &user_pool;
	}
	lock_acquire(&mem_pool->lock);
	b = (struct mem_block*)vaddr;
	a = block2arena(b);
	desc = a->desc;
	if(a->large == TRUE && a->desc == NULL)
		free_pages(pf, (uint32_t)vaddr - sizeof(struct arena), a->cnt);		// 当初返回的时候跳过了arena
	else
	{
		list_append(&desc->free_list, &b->free_elem);
		++a->cnt;
		if(a->cnt == desc->blocks_per_arena)
			free_arena(pf, a);
	}
	lock_release(&mem_pool->lock);
}