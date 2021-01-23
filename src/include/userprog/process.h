#ifndef _USER_PROCESS_H
#define _USER_PROCESS_H

#include "thread.h"

#define USER_STACK3_VADDR (0xc0000000 - 0x1000)
#define USER_VADDR_START 0x08048000

void process_start(void *filename_);
void process_activate(const struct task_struct *pthread);
void page_dir_activate(const struct task_struct *pthread);
uint32_t* create_page_dir(void);
void create_user_vaddr_bitmap(struct task_struct *user_prog);
void process_execute(void *filename, const char *name, int prio);

#endif