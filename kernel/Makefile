CC = gcc
CFLAGS = -ggdb -Wall -Wextra -nostdinc -nostdlib -fno-builtin -fno-common
LDFLAGS =

all: kernel kernel.elf

kernel.elf: kernel.o segmentation.o hardware.o load_gdt.o font.o sched.o interrupt.o interrupt_handler.o lapic_timer.o syscall.o memory.o util.o pm_timer.o port_io.o virtio.o
	ld $(LDFLAGS) -T kernel.elf.ld -o $@ $+

kernel: kernel.o segmentation.o hardware.o load_gdt.o font.o sched.o interrupt.o interrupt_handler.o lapic_timer.o syscall.o memory.o util.o pm_timer.o port_io.o virtio.o
	ld $(LDFLAGS) -T kernel.ld -o $@ $+

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

%.o:%.s
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o kernel kernel.elf

.PHONY: all debug clean
