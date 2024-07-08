#pragma once
#include "hardware.h"
#include "util.h"
#include "common.h"

struct FADT *rsdp_to_fadt(struct RSDP *rsdp);

int init_acpi_pm_timer(struct RSDP *rsdp);
void pm_timer_wait_millisec(unsigned int msec);
