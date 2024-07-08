#pragma once
#include "sched.h"

#define N_TASKS 3

void init_virtual_memory();
extern unsigned long long kernel_cr3;
extern unsigned long long task_cr3s[N_TASKS];
