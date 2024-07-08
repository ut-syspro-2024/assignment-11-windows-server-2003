#include "hardware.h"
#include "segmentation.h"
#include "sched.h"
#include "interrupt.h"
#include "memory.h"
#include "util.h"
#include "font.h"
#include "pm_timer.h"
#include "lapic_timer.h"
#include "syscall.h"
#include "virtio.h"

#pragma GCC diagnostic ignored "-Wunused-parameter" // For l6 (kernel_param_dummy)


void test_syscall_puts(void);

void start(void *SystemTable __attribute__ ((unused)), struct HardwareInfo *_hardware_info, unsigned long long kernel_param_dummy) {
  // From here - Put this part at the top of start() function
  // Do not use _hardware_info directry since this data is located in UEFI-app space
  hardware_info = *_hardware_info;
  init_segmentation();
  init_virtual_memory();
  init_intr();
  // To here - Put this part at the top of start() function

  init_draw();
  if (init_acpi_pm_timer(hardware_info.rsdp) != 0) {
	  puts("init acpi failed\n");
	  while (1);
  }
  
  test_syscall_puts();
  
  init_virtio();
  
  /*
  for (int i = 0; i < 2; i++) {
	  puts("waiting 1 s...\n");
	  pm_timer_wait_millisec(1000);
  }
  for (int i = 0; i < 1; i++) {
	  puts("waiting 4 s...\n");
	  pm_timer_wait_millisec(4000);
  }*/
  for (int i = 0; i < 1; i++) {
	  puts("lapic freq: ");
	  puth(measure_lapic_freq_khz(), 8);
	  puts(" KHz\n");
  }
  
  /*
  // multitasking
  init_tasks();
  
  void *sched_handler;
  GET_FUNC_ADDRESS(schedule, sched_handler);
  lapic_periodic_exec(1000, sched_handler);
  
  start_task0();*/
  
  // Do not delete this line!
  while (1);
}

void test_syscall_puts() {
	unsigned long long ret;
	const char *str = "syspro! this is printed through puts syscall\r\n";
	asm volatile (
		"mov %[id], %%rdi\n"
		"mov %[str], %%rsi\n"
		"int $0x80\n"
		"mov %%rax, %[ret]\n"
		: [ret]"=r"(ret)
		: [id]"r"((unsigned long long)SYSCALL_PUTS),
		[str]"m"((unsigned long long)str)
	);
}

