#include "memory.h"
#include "sched.h"
#include "common.h"
#include "util.h"

struct Entry {
  unsigned int present : 1;
  unsigned int writable : 1;
  unsigned int user_accessible : 1;
  unsigned int write_through_caching : 1;
  unsigned int disable_cache : 1;
  unsigned int accessed : 1;
  unsigned int dirty : 1;
  unsigned int huge_page : 1;
  unsigned int global : 1;
  unsigned int available1 : 3;
  unsigned long long physical_address : 40;
  unsigned int available2 : 11;
  unsigned int no_execute : 1;
} __attribute__((packed));

#define N_PAGES_IN_TABLE 512

#define PDP_LEN ((u64) 1 << 39)
#define PD_LEN ((u64) 1 << 30)
#define PT_LEN ((u64) 1 << 21)
#define PAGE_LEN ((u64) 1 << 12)

#define TASK_VADDR_START ((u64) 0x040000000)
#define TASK_MEM_LEN     ((u64) 0x001000000)
#define FRAME_BUF_ADDR  ((u64) 0x0c0000000)
#define FRAME_BUF_LEN   ((u64) 0x000200000)
#define LAPIC_BUF_ADDR  ((u64) 0x0fee00000)
#define KERNEL_MEM_ADDR ((u64) 0x100000000)
#define KERNEL_MEM_LEN  ((u64) 0x001000000)

#define N_TASK_PTs (TASK_MEM_LEN / PT_LEN)
#define N_KERNEL_PTs (KERNEL_MEM_LEN / PT_LEN)

#define TASK0_PHYS_ADDR_START ((u64) 0x104000000)
#define TASK1_PHYS_ADDR_START ((u64) 0x105000000)
#define TASK2_PHYS_ADDR_START ((u64) 0x106000000)


unsigned long long task_cr3s[N_TASKS];
unsigned long long kernel_cr3;

struct Entry PML4s[N_TASKS][N_PAGES_IN_TABLE]
__attribute__((aligned(4096)));
struct Entry PDPs[N_TASKS][N_PAGES_IN_TABLE]
__attribute__((aligned(4096)));
struct Entry PDs[N_TASKS][N_PAGES_IN_TABLE]
__attribute__((aligned(4096)));
struct Entry PTs[N_TASKS][N_TASK_PTs][N_PAGES_IN_TABLE]
__attribute__((aligned(4096)));

struct Entry kernel_PD[N_PAGES_IN_TABLE] __attribute__((aligned(4096)));
struct Entry kernel_PTs[N_KERNEL_PTs][N_PAGES_IN_TABLE]
__attribute__((aligned(4096)));

struct Entry io_PD[N_PAGES_IN_TABLE] __attribute__((aligned(4096)));
struct Entry fb_PT[N_PAGES_IN_TABLE] __attribute__((aligned(4096)));
struct Entry lapic_PT[N_PAGES_IN_TABLE] __attribute__((aligned(4096)));

u64 addr2page(u64 addr) {
	ASSERT(addr % PAGE_LEN == 0);
	return addr >> 12; // 4KiB page
}
u64 get_cr3_value(void *pml4e_addr) {
	return addr2page((u64) pml4e_addr) << 12;
}
u64 get_task_phys_addr(int task_id) {
	if (task_id == 0) return TASK0_PHYS_ADDR_START;
	if (task_id == 1) return TASK1_PHYS_ADDR_START;
	if (task_id == 2) return TASK2_PHYS_ADDR_START;
	return 0;
}
void init_entry(struct Entry *entry, void *addr) {
	entry->present = 1;
	entry->writable = 1;
	entry->physical_address = addr2page((u64) addr);
}
void init_virtual_memory() {
	// Save kernel cr3 register value
	asm volatile ("mov %%cr3, %0" : "=r"(kernel_cr3));
	
	// lapic PT
	ASSERT(LAPIC_BUF_ADDR % PAGE_LEN == 0);
	int lapic_page_index = LAPIC_BUF_ADDR % PT_LEN / PAGE_LEN;
	int lapic_pt_index = LAPIC_BUF_ADDR % PD_LEN / PT_LEN;
	init_entry(&lapic_PT[lapic_page_index], (void *) LAPIC_BUF_ADDR);
	
	// fb PT
	ASSERT(FRAME_BUF_ADDR % PAGE_LEN == 0);
	ASSERT(FRAME_BUF_LEN % PAGE_LEN == 0);
	int fb_page_base = FRAME_BUF_ADDR % PT_LEN / PAGE_LEN;
	int n_fb_pages = FRAME_BUF_LEN / PAGE_LEN;
	int fb_pt_index = FRAME_BUF_ADDR % PD_LEN / PT_LEN;
	for (int i = 0; i < n_fb_pages; i++) init_entry(&fb_PT[fb_page_base + i], (void *) (FRAME_BUF_ADDR + i * PAGE_LEN));
	
	// lapic + fb PD
	ASSERT(LAPIC_BUF_ADDR / PD_LEN == FRAME_BUF_ADDR / PD_LEN);
	int io_pd_index = LAPIC_BUF_ADDR % PDP_LEN / PD_LEN;
	init_entry(&io_PD[lapic_pt_index], lapic_PT);
	init_entry(&io_PD[fb_pt_index], fb_PT);
	
	// kernel PD/PT
	ASSERT(KERNEL_MEM_ADDR % PT_LEN == 0);
	int kernel_pt_base = KERNEL_MEM_ADDR % PD_LEN / PT_LEN;
	int kernel_pd_index = KERNEL_MEM_ADDR / PD_LEN;
	ASSERT(kernel_pt_base + N_KERNEL_PTs <= N_PAGES_IN_TABLE);
	for (u32 i = 0; i < N_KERNEL_PTs; i++) {
		for (u32 j = 0; j < N_PAGES_IN_TABLE; j++) init_entry(&kernel_PTs[i][j], (void *) (KERNEL_MEM_ADDR + (i * N_PAGES_IN_TABLE + j) * PAGE_LEN));
		init_entry(&kernel_PD[i + kernel_pt_base], kernel_PTs[i]);
	}
	
	for (int i = 0; i < N_TASKS; i++) {
		// init PML4s, PDPs, PDs, PTs
		int pdp_index = TASK_VADDR_START / PDP_LEN;
		int pd_index = TASK_VADDR_START % PDP_LEN / PD_LEN;
		int pt_index_base = TASK_VADDR_START % PD_LEN / PAGE_LEN;
		ASSERT(pt_index_base + N_TASK_PTs <= N_PAGES_IN_TABLE);
		ASSERT(TASK_VADDR_START % PAGE_LEN == 0);
		ASSERT(LAPIC_BUF_ADDR / PDP_LEN == TASK_VADDR_START / PDP_LEN);
		ASSERT(KERNEL_MEM_ADDR / PDP_LEN == TASK_VADDR_START / PDP_LEN);
		
		u64 phys_base = get_task_phys_addr(i);
		for (u32 j = 0; j < N_TASK_PTs; j++) {
			for (u32 k = 0; k < N_PAGES_IN_TABLE; k++)
				init_entry(&PTs[i][j][k], (void *) (phys_base + (j * N_PAGES_IN_TABLE + k) * PAGE_LEN));
			init_entry(&PDs[i][j + pt_index_base], PTs[i][j]);
		}
		// add lapic/fb to the same PDP
		init_entry(&PDPs[i][pd_index], PDs[i]);
		init_entry(&PDPs[i][io_pd_index], io_PD);
		init_entry(&PDPs[i][kernel_pd_index], kernel_PD);
		
		init_entry(&PML4s[i][pdp_index], PDPs[i]);
		
		// calculate CR3 value
		task_cr3s[i] = get_cr3_value(PML4s[i]);
	}
}

#define ADDITIONAL_PAGE_START ((u64) 0x300000000)
#define ADDITIONAL_PHYS_ADDR_START ((u64) 0x101000000)


struct Entry additional_PT[N_PAGES_IN_TABLE] __attribute__((aligned(4096)));
struct Entry additional_PD[N_PAGES_IN_TABLE] __attribute__((aligned(4096)));

void page_fault_handler_internal(u64 *sp) {
	u64 error_code = sp[15];
	unsigned long long error_addr;
	asm volatile ("mov %%cr2, %0" : "=r"(error_addr));
	
	if (error_addr >= ADDITIONAL_PAGE_START && error_addr < ADDITIONAL_PAGE_START + PAGE_LEN) {
		puts("! kernel: mapping additional page\n");
		ASSERT(ADDITIONAL_PAGE_START % PT_LEN == 0);
		init_entry(&additional_PT[0], (void *) ADDITIONAL_PHYS_ADDR_START);
		init_entry(&additional_PD[0], additional_PT);
		init_entry(&PDPs[0][ADDITIONAL_PAGE_START / PD_LEN], additional_PD); // app0
	} else {
		const int IS_WRITE = 1 << 1;
		const int IS_INST_FETCH = 1 << 4;
		const char *msg = (error_code & IS_INST_FETCH) ? "Page fault(instruction fetch)\n" :
						  (error_code & IS_WRITE) ? "Page fault(data write)\n" : "Page fault(data read)\n";
		puts((char *) msg);
		puts("addr:");
		puth(error_addr, 16);
		puts("\n");
		while (1) { volatile int x = 0; (void) x; }
	}
}
