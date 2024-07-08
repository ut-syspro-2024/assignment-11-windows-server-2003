#include "interrupt.h"
#include "lapic_timer.h"
#include "common.h"

struct InterruptDescriptor {
  unsigned short offset_lo;
  unsigned short segment;
  int attribute_ist: 3;
  int attribute_dummy0: 5;
  int attribute_type: 4;
  int attribute_dummy1: 1;
  int attribute_dpl: 2;
  int attribute_p: 1;
  unsigned short offset_mid;
  unsigned int offset_hi;
  unsigned int reserved;
} __attribute__((packed));

struct InterruptDescriptor IDT[256];

void lapic_intr_handler();

void syscall_handler();

struct idt_10byte {
	u16 len;
	u64 address;
} __attribute__((packed));

static void load_idt_to_idtr() {
  // Set idtr
  volatile struct idt_10byte idt;
  idt.len = sizeof(IDT);
  idt.address = (u64) IDT;
  
  asm volatile ("lidt %0" : : "m"(idt));
}

static void set_interrupt(int int_id, void *func, u16 cs) {
	IDT[int_id].offset_lo = (u64) func & ((1 << 16) - 1);
	IDT[int_id].offset_mid = (u64) func >> 16 & ((1 << 16) - 1);
	IDT[int_id].offset_hi = (u64) func >> 32;
	IDT[int_id].segment = cs;
	
	IDT[int_id].attribute_ist = 0;
	IDT[int_id].attribute_dpl = 0;
	IDT[int_id].attribute_type = 0x0E; // Interrupt Gate
	IDT[int_id].attribute_p = 1;
}

void init_intr() {
  // Get segment register value
  unsigned short cs;
  asm volatile ("mov %%cs, %0" : "=r"(cs));

  void* lapic_intr_handler_addr;
  GET_FUNC_ADDRESS(lapic_intr_handler, lapic_intr_handler_addr);

  void* syscall_handler_addr;
  GET_FUNC_ADDRESS(syscall_handler, syscall_handler_addr);
  
  void *page_fault_handler_addr;
  GET_FUNC_ADDRESS(page_fault_handler, page_fault_handler_addr);

  // Register Local APIC handler
  // Register Sycall handler
  set_interrupt(LAPIC_INTERRUPT_ID, lapic_intr_handler_addr, cs);
  set_interrupt(SYSCALL_INTERRUPT_ID, syscall_handler_addr, cs);
  set_interrupt(PAGE_FAULT_INTERRUPT_ID, page_fault_handler_addr, cs);

  // Tell CPU the location of IDT
  load_idt_to_idtr();

  // Set IF bit in RFLAGS register
  asm volatile ("sti");
}
