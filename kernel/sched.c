#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "util.h"
#include "common.h"
#include "lapic_timer.h"
#include "memory.h"

#define STACK_SIZE 0x2000 // 8 KB

// address of APPs
static u64 get_entrypoint(int task_id) {
	// return 0x104000000 + task_id * 0x1000000;
	return 0x040000000;
}
static u64 get_stack_start(int task_id) {
	return get_entrypoint(task_id) + 0x1000000;
}

typedef struct {
	u64 sp; // rsp at which all general regsisters have been just pushed
	u64 cr3;
} __attribute__((packed)) task_t; // refered from assembly


#define SET_SP(sp) asm volatile ("mov %0, %%rsp" :: "r"(sp))
#define SET_CR3(cr3) asm volatile ("mov %0, %%cr3" :: "a"(cr3))
static task_t tasks[N_TASKS];
static u32 cur_task;

#define GET_REGISTER(var, reg) \
	asm volatile ("mov " reg ", %0": "=r"(var));

// prepare initial stack
static void prepare_stack(int id, u64 rip) {
	u64 app_cr3 = task_cr3s[id];
	SET_CR3(app_cr3);
	
	u64 initial_sp = tasks[id].sp;
	
	// push SS
	tasks[id].sp -= 8;
	GET_REGISTER(*(u16 *) tasks[id].sp, "%%ss");
	// push RSP
	tasks[id].sp -= 8;
	*(u64 *) tasks[id].sp = initial_sp;
	// push RFLAGS
	unsigned long long sp_bak;
	u64 tmp_sp = tasks[id].sp;
	asm volatile (
		"mov %%rsp, %0\n"
		"mov %1, %%rsp\n"
		"pushfq\n" : "=r"(sp_bak) : "m"(tmp_sp)
	);
	asm volatile ("mov %0, %%rsp" :: "m"(sp_bak)); // restore sp
	tasks[id].sp -= 8;
	// push CS
	tasks[id].sp -= 8;
	GET_REGISTER(*(u16 *) tasks[id].sp, "%%cs");
	// push RIP
	tasks[id].sp -= 8;
	*(u64 *) tasks[id].sp = rip;
	
	// push RAX-RDX, RDI, RSI, RBP, +8 registers
	tasks[id].sp -= 8 * 15;
	
	SET_CR3(kernel_cr3);
}
void init_tasks() {
	for (int i = 0; i < N_TASKS; i++) {
		tasks[i].sp = get_stack_start(i);
		tasks[i].cr3 = task_cr3s[i];
		if (i != 0) prepare_stack(i, get_entrypoint(i));
	}
	cur_task = 0;
}

void start_task0() {
	cur_task = 0;
	SET_SP(tasks[cur_task].sp);
	SET_CR3(tasks[cur_task].cr3);
	unsigned long long entrypoint = get_entrypoint(cur_task);
	asm volatile ("jmp *%0" :: "m"(entrypoint));
}

unsigned long long schedule(unsigned long long sp) {
	tasks[cur_task].sp = sp;
	int next_task = (cur_task + 1) % N_TASKS;
	cur_task = next_task;
	return (u64) &tasks[next_task];
}
