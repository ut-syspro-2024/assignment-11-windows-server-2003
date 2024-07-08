#include "lapic_timer.h"
#include "pm_timer.h"
#include "interrupt.h"

#define COUNT_MAX 0xffffffff

volatile unsigned int *lvt_timer = (unsigned int *)0xfee00320;
volatile unsigned int *initial_count = (unsigned int *)0xfee00380;
volatile unsigned int *current_count = (unsigned int *)0xfee00390;
volatile unsigned int *divide_config = (unsigned int *)0xfee003e0;

unsigned int lapic_timer_freq_khz = 0;

volatile unsigned int *lapic_eoi = (unsigned int *)0xfee000b0;


unsigned long long (*reserved_callback)(unsigned long long);

#define MODE_ONESHOT 0
#define MODE_PERIODIC 1
#define MODE_TSC_DEADLINE 2

void set_lvl_timer_value(int mode, int interrupt_id) {
	u32 delivery_status = *lvt_timer & (1 << 12);
	if (interrupt_id == -1) 
		*lvt_timer = delivery_status | (1U << 16) | (mode << 17);
	else *lvt_timer = (interrupt_id & 0xFF) | delivery_status | (mode << 17);
}
void set_divide_config(int n) {
	u32 base = *divide_config & ~0b1011;
	if (n == 1) base |= 0b1011;
	else if (n == 2) base |= 0b0000;
	else if (n == 4) base |= 0b0001;
	*divide_config = base;
}


unsigned int measure_lapic_freq_khz() {
  // Initialize LAPIC One-Shot timer
  
  u32 init_value = 0x80000000;
  set_lvl_timer_value(MODE_ONESHOT, -1);
  set_divide_config(1);
  *initial_count = init_value;
  
  pm_timer_wait_millisec(1000);
  
  u32 after_value = *current_count;
  if (after_value == 0) {
	  puts("lapic timer went zero\n");
	  return -1;
  }
  return (lapic_timer_freq_khz = (init_value - after_value) / 1000);
}

void lapic_periodic_exec(unsigned int msec, void *callback) {
  if (!lapic_timer_freq_khz) {
    puts("Call measure_lapic_freq_khz() beforehand\r\n");
    return;
  }

  reserved_callback = callback;

  // Set LAPIC Periodic Timer
  set_lvl_timer_value(MODE_PERIODIC, LAPIC_INTERRUPT_ID);
  set_divide_config(1);
  *initial_count = msec * lapic_timer_freq_khz;
}

// returns new sp to be used for popping general registers(and then hardware poppping)
unsigned long long lapic_intr_handler_internal(unsigned long long sp) {
  unsigned long long res = reserved_callback(sp); // new sp
  *lapic_eoi = 0;
  return res;
}
