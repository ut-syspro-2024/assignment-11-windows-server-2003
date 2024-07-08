#include "pm_timer.h"
#include "port_io.h"


const unsigned int freq_hz = 3579545;
unsigned short pm_timer_blk;
char pm_timer_is_32;

#define TMR_VAL_EXT (1 << 8)

struct FADT *rsdp_to_fadt(struct RSDP *rsdp) {
	struct XSDT *xsdt = (struct XSDT *) rsdp->xsdt_address;
	int n_other_entries = (xsdt->sdth.length - 36) / 8;
	for (int i = 0; i < n_other_entries; i++) {
		struct SDTH *cur_sdth = (struct SDTH *) xsdt->other_tables[i];
		if (my_memcmp(cur_sdth->signature, "FACP", 4) == 0) return (struct FADT *) cur_sdth;
	}
	return NULL;
}

int init_acpi_pm_timer(struct RSDP *rsdp) {
	struct FADT *fadt = rsdp_to_fadt(rsdp);
	if (!fadt) {
		puts("FADT not found\n");
		return 1;
	}
	
	puth(fadt->PM_TMR_BLK, 8);
	puts("\n");
	pm_timer_blk = fadt->PM_TMR_BLK;
	pm_timer_is_32 = (fadt->flags & TMR_VAL_EXT) != 0;
	puts("ACPI PM Timer initialized. width: ");
	puts(pm_timer_is_32 ? "32bit\n" : "24bit\n");
	return 0;
}
static u32 get_mask() {
	return pm_timer_is_32 ? 0xFFFFFFFF : 0xFFFFFF;
}
static u32 get_highest_bit() {
	return pm_timer_is_32 ? 0x80000000 : 0x800000;
}
static u32 read_pm_timer() {
	u32 cur_timer = io_read32(pm_timer_blk);
	return cur_timer & get_mask();
}

// if cur is in [target - 1/2*period, target), return -1
// if cur is in [target, target + 1/2*period), return 1
int get_timer_half_state(u32 cur, u32 target) {
	return ((cur - target) & get_mask() & get_highest_bit()) ? -1 : 1;
}

void pm_timer_wait_millisec(unsigned int msec) {
	u32 t0 = read_pm_timer(); // start time
	u32 t1 = (t0 + (u64) msec * freq_hz / 1000); // target time
	t1 &= get_mask();
	
	int cur_state = get_timer_half_state(t0, t1);
	while (1) {
		u32 cur = read_pm_timer();
		int next_state = get_timer_half_state(cur, t1);
		// transition from -1 to 1 means timer passed t1(unless there is >1/2*period time gap between polling)
		if (cur_state == -1 && next_state == 1) break; 
		cur_state = next_state;
	}
}
