#pragma once

unsigned int measure_lapic_freq_khz(void);
void lapic_periodic_exec(unsigned int msec, void *callback);
unsigned long long lapic_intr_handler_internal();
