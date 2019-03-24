#include "bitmap.h"
#include "string.h"
#include "debug.h"

void bitmap_init(struct bitmap* btmp)
{
	memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

int bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx)
{
	uint32_t byte_idx = bit_idx / 8;
	uint32_t bit_odd = bit_idx % 8;
	return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}

int bitmap_scan(struct bitmap* btmp, uint32_t cnt)
{
	uint32_t idx_byte = 0, idx_bit = 0;
	while((idx_byte < btmp->btmp_bytes_len) && (btmp->bits[idx_byte] == 0xff))	// 加速查询
		idx_byte++;
	ASSERT(idx_byte <= btmp->btmp_bytes_len);
	if(idx_byte == btmp->btmp_bytes_len)
		return -1;
	while((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte])
		idx_bit++;
	int bit_idx_start = idx_byte * 8 + idx_bit;
	if(cnt == 1)
		return bit_idx_start;
	uint32_t bit_left = btmp->btmp_bytes_len * 8 - bit_idx_start - 1;		// 原书无-1，考虑bit_idx_start = 0, btmp_bytes_len = 1的情况，此时0号已被确认占用，则只剩下7个待查位
	uint32_t count = 1, next_bit = bit_idx_start + 1;
	bit_idx_start = -1;
	while(bit_left-- > 0)
	{
		if(!bitmap_scan_test(btmp, next_bit))
			count++;
		else
			count = 0;
		if(count == cnt)
		{
			bit_idx_start = next_bit - cnt + 1;
			break;
		}
		++next_bit;
	}
	return bit_idx_start;
}

/* 设置位图bit_idx位的值 */
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value)
{
	ASSERT(value == 0 || value == 1);
	uint32_t byte_idx = bit_idx / 8;
	uint32_t bit_odd = bit_idx % 8;
	btmp->bits[byte_idx] = value ? (btmp->bits[byte_idx] | (BITMAP_MASK << bit_odd)) : (btmp->bits[byte_idx] & ~(BITMAP_MASK << bit_odd));
}

/* 获取位图bit_idx位的值*/
int8_t bitmap_get(struct bitmap* btmp, uint32_t bit_idx)
{
	uint32_t byte_idx = bit_idx / 8;
	uint32_t bit_odd = bit_idx % 8;
	return btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd);
}
