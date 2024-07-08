#include "common.h"

u32 io_read32(u16 address);
u16 io_read16(u16 address);
u8 io_read8(u16 address);
void io_write32(u16 address, u32 value);
void io_write16(u16 address, u16 value);
void io_write8(u16 address, u8 value);
