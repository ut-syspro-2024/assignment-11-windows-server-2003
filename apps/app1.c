#include "syscall.h"

unsigned long long text_section_top = 0x104000000;

void app1() {
  while (1) {
    char *str = "Hello from app1\r\n";

    syscall_puts(str);
	
	volatile int *x = (int *) 0x300000000;
	volatile int y = x[0];
	y = x[1];
	(void) y;

    volatile int i = 100000000;
    while (i--);
  }

  asm volatile ("jmp *%0" :: "m"(text_section_top));
}
