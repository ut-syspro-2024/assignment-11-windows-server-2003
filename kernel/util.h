#pragma once
#include "common.h"

typedef struct {
	u8 r;
	u8 g;
	u8 b;
} color_t;

color_t get_color_from_code(u32 code);

#define COLOR_WHITE get_color_from_code(0xFFFFFF)
#define COLOR_BLACK get_color_from_code(0x000000)
#define COLOR_AYAME ((color_t) {111, 51, 129})

#define ASSERT(cond) if (!(cond)) { puts("assert: " #cond); while (1) { volatile int x = 0; (void) x; } }


u32 make_delay(int seed);
void init_draw();

void draw_char(char c, int offset_x, int offset_y, color_t color);

void my_strcpy(char *s, const char *t);
int my_memcmp(const char *s, const char *t, int len);


void putc(char c);
void puts(char *str);
void puth(unsigned long long value, unsigned char digits_len);
