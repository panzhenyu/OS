#ifndef _USER_TSS_H
#define _USER_TSS_H

#include "thread.h"

void tss_init();
void update_tss_esp(struct task_struct* pthread);

#endif