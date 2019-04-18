#include "stdint.h"
#include "syscall.h"
#include "syscall-init.h"

#define _syscall0(NUMBER) ({    \
    int retval;                 \
    asm volatile (              \
        "int $0x80"             \
        : "=a"(retval)          \
        : "a"(NUMBER)           \
        : "memory"              \
    );                          \
    retval;                     \
})

#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({          \
    int retval;                                         \
    asm volatile (                                      \
        "int $0x80"                                     \
        : "=a"(retval)                                  \
        : "a"(NUMBER), "b"(ARG1), "c"(ARG2), "d"(ARG3)  \
        : "memory"                                      \
    );                                                  \
    retval;                                             \
})

uint32_t getpid()
{
    return _syscall0(SYS_GETPID);
}

uint32_t write(char *str)
{
    return _syscall3(SYS_WRITE, str, 0, 0);
}