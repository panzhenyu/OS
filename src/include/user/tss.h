#ifndef _USER_TSS_H
#define _USER_TSS_H

#include "thread.h"

void tss_init();
void update_tss_esp(const struct task_struct* pthread);

#endif