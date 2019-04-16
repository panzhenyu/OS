#include "syscall.h"
#include "stdio.h"
#include "global.h"
#include "string.h"

typedef char* va_list;
#define va_start(ap, v) ap = (va_list)&v
#define va_arg(ap, t) *((t*)(ap += 4))
#define va_end(ap) ap = NULL

/* convert integer to string
   only convert positive */
static void itoa(uint32_t value, char **buff_ptr, uint8_t base)
{
    uint32_t v, t;
    v = value / base;
    t = value % base;
    if(v)
        itoa(v, buff_ptr, base);
    if(t < 10)
        *((*buff_ptr)++) = '0' + t;
    else
        *((*buff_ptr)++) = 'A' + t - 10;
}

uint32_t vsprintf(char *str, const char *format, va_list ap)
{
    char ch, type, *s;
    int32_t var;
    while((ch = *format++) != '\0')
    {
        if(ch != '%')
            *str++ = ch;
        else
        {
            type = *format++;
            switch(type)
            {
                case 'd':
                    var = va_arg(ap, int);
                    if(var < 0)
                    {
                        *str++ = '-';
                        var = ~var + 1;
                    }
                    itoa(var, &str, 10);
                    break;
                case 'x':
                    var = va_arg(ap, int);
                    if(var < 0)
                    {
                        *str++ = '-';
                        var = ~var + 1;
                    }
                    *str++ = '0';
                    *str++ = 'x';
                    itoa(var, &str, 16);
                    break;
                case 'c':
                    ch = va_arg(ap, char);
                    *str++ = ch;
                    break;
                case 's':
                    s = va_arg(ap, char*);
                    strcpy(str, s);
                    str += strlen(s);
                    break;
                case '%':
                    *str++ = '%';
                    break;
                default:
                    *str++ = type;
            }
        }
    }
    *str = '\0';
    return strlen(str);
}

/* 将数据格式化输出到str，用户进程、内核进程皆可调用 */
uint32_t sprintf(char *str, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    uint32_t len = vsprintf(str, format, ap);
    va_end(ap);
    return len;
}

/* 涉及系统调用，因此仅用户进程可以调用 */
int printf(const char *format, ...)
{
    char buff[1024] = {0};
    va_list ap;
    va_start(ap, format);
    vsprintf(buff, format, ap);
    va_end(ap);
    return write(buff);
}