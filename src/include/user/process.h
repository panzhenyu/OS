#ifndef _USER_PROCESS_H
#define _USER_PROCESS_H

#define USER_STACK3_VADDR (0xc0000000 - 0x1000)

void process_start(void *filename_);

#endif