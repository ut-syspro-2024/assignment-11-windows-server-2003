#pragma once

#define GET_FUNC_ADDRESS(func, res) do {\
		asm volatile ("lea " #func " (%%rip), %0" : "=r"(res));\
	} while(0)

#define NULL ((void *) 0)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
