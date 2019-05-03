#include "string.h"
#include "debug.h"
#include "global.h"

//初始化dst起始处连续的size个大小的内存
void memset(void* dst, uint8_t value, uint32_t size)
{
	ASSERT(dst != NULL);
	
	uint8_t* p = (uint8_t*)dst;
	while(size-- > 0)
	{
		*p = value;
		p++;
	}
}

//将src起始处连续的size个大小的内存复制给dst
void memcpy(void* dst, const void* src, uint32_t size)
{
	ASSERT(dst != NULL && src != NULL);
	
	uint8_t* pd = (uint8_t*)dst;
       	const uint8_t* ps = (uint8_t*)src;
	while(size-- > 0)
	{
		*pd = *ps;
		pd++;
		ps++;
	}
}

//比较m1与m2起始处连续的size个内存的大小，m1 > m2时返回1，相等时返回0，否则返回-1
int memcmp(const void* m1, const void* m2, uint32_t size)
{
	ASSERT(m1 != NULL && m2 != NULL);
	const uint8_t *pm1 = (uint8_t*)m1, *pm2 = (uint8_t*)m2;
	while(size-- > 0)
	{
		if(*pm1 != *pm2)
			return *pm1 > *pm2 ? 1 : -1;
		pm1++;
		pm2++;
	}
	return 0;
}

//将字符串从src复制到dst
char* strcpy(char* dst, const char* src)
{
	ASSERT(dst != NULL && src != NULL);
	char* r = dst;
	while((*dst++ = *src++));
	return r;
}

//比较两个字符串，s1中的字符大于s2中的字符时返回1，相等时返回0，否则返回-1
int strcmp(const char* s1, const char* s2)
{
	ASSERT(s1 != NULL && s2 != NULL);
	while(*s1 && *s1 == *s2)
	{
		++s1;
		++s2;
	}
	return *s1 < *s2 ? -1 : *s1 > *s2;

}

//将src拼接到dst后
char* strcat(char* dst, const char* src)
{
	ASSERT(dst != NULL && src != NULL);
	char* r = dst;
	while(*dst++);
	--dst;
	while((*dst++ = *src++));
	return r;
}

//从左到右查找str中首次出现ch的地址
char* strchr(const char* str, uint8_t ch)
{
	ASSERT(str != NULL);
	const uint8_t* p = (uint8_t*)str;
	while(*p)
	{
		if(*p == ch)
			return (char*)p;
		p++;
	}
	return NULL;
}

//从右向左查找str中首次出现ch的地址
char* strrchr(const char* str, uint8_t ch)
{
	ASSERT(str != NULL);
	const uint8_t* p = (uint8_t*)str;
	const uint8_t* r = NULL;
	while(*p)
	{
		if(*p == ch)
			r = p;
		p++;
	}
	return (char*)r;
}

//返回字符串中ch出现的次数
uint32_t strchrs(const char* str, uint8_t ch)
{
	ASSERT(str != NULL);
	const uint8_t* p = (uint8_t*)str;
	uint32_t ch_cnt = 0;
	while(*p)
	{
		if(*p == ch)
			ch_cnt++;
		p++;
	}
	return ch_cnt;
}

//返回字符串长度
uint32_t strlen(const char* str)
{
	ASSERT(str != NULL);
	const char* p = str;
	while(*p++);				// 空循环体减少执行的指令数，效率更高，因此不采用整型变量计数
	return (uint32_t)(p - str - 1);
}
