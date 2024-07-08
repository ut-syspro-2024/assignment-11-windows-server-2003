#include "hardware.h"
#include "util.h"
#include "font.h"

color_t get_color_from_code(u32 code) {
	color_t res;
	res.r = code >> 16 & 0xFF;
	res.g = code >> 8  & 0xFF;
	res.b = code >> 0  & 0xFF;
	return res;
}

#define COLOR_WHITE get_color_from_code(0xFFFFFF)

#define MAX_LINES         (1920 / FONT_HEIGHT)
#define MAX_CHARS_IN_LINE (1080 / FONT_WIDTH)
static char terminal_buf[MAX_LINES][MAX_CHARS_IN_LINE + 1];
// actual numbers based on hardware info, should be <= MAX_LINES, MAX_CHARS_IN_LINE resp.
static int n_lines, n_chars_in_line;
// terminal state
static int cur_line = 0; // always  < n_lines
static int cur_pos_in_line = 0; // always  <= n_chars_in_line


u32 make_delay(int seed) {
	u32 res = 0;
	for (int i = 0; i < 100000000; i++) res += (seed * i ^ i >> 1);
	return res % 10000;
}
void my_strcpy(char *s, const char *t) {
	for (; *t; t++, s++) *s = *t;
	*s = '\0';
}
int my_memcmp(const char *s, const char *t, int len) {
	for (int i = 0; i < len; i++) if (s[i] != t[i]) return s[i] < t[i] ? -1 : 1;
	return 0;
}
void fill(color_t color) {
	for (unsigned int i = 0; i < hardware_info.fb.height; i++) {
		for (unsigned int j = 0; j < hardware_info.fb.width; j++) {
			struct Pixel *pixel = hardware_info.fb.base + hardware_info.fb.width * i + j;
			pixel->r = color.r;
			pixel->g = color.g;
			pixel->b = color.b;
		}
	}
}
void init_draw() {
	// init n_lines/n_chars_in_line
	n_lines = hardware_info.fb.height / FONT_HEIGHT;
	if (n_lines > MAX_LINES) n_lines = MAX_LINES;
	n_chars_in_line = hardware_info.fb.width / FONT_WIDTH;
	if (n_chars_in_line > MAX_CHARS_IN_LINE) n_chars_in_line = MAX_CHARS_IN_LINE;
	
	fill(COLOR_BLACK);
}
void draw_char(char c, int offset_x, int offset_y, color_t color) {
	if (c < 0 || c >= FONT_CODE_MAX) return;
	for (u32 i = 0; i < FONT_HEIGHT; i++) for (u32 j = 0; j < FONT_WIDTH; j++) {
		u32 x = offset_x + i;
		u32 y = offset_y + j;
		if (x >= hardware_info.fb.height) continue;
		if (y >= hardware_info.fb.width) continue;
		if (font[c][i] >> (FONT_WIDTH - 1 - j) & 1) {
			struct Pixel *pixel = hardware_info.fb.base + hardware_info.fb.width * x + y;
			pixel->r = color.r;
			pixel->g = color.g;
			pixel->b = color.b;
		}
	}
}

void redraw_terminal() {
	fill(COLOR_BLACK);
	for (int i = 0; i < n_lines; i++) {
		for (int j = 0; j < n_chars_in_line; j++) {
			if (!terminal_buf[i][j]) break;
			draw_char(terminal_buf[i][j], i * FONT_HEIGHT, j * FONT_WIDTH, COLOR_WHITE);
		}
	}
}
void putc(char c) {
	if (c == '\n' || cur_pos_in_line == n_chars_in_line) { // line break
		int redraw = 0;
		if (cur_line + 1 == n_lines) { // shift lines
			for (int i = 0; i + 1 < n_lines; i++) my_strcpy(terminal_buf[i], terminal_buf[i + 1]);
			my_strcpy(terminal_buf[n_lines - 1], "");
			redraw = 1;
		} else cur_line++;
		cur_pos_in_line = 0;
		
		if (c != '\n') {
			if (!redraw) draw_char(c, cur_line * FONT_HEIGHT, cur_pos_in_line * FONT_WIDTH, COLOR_WHITE);
			terminal_buf[cur_line][cur_pos_in_line++] = c;
		}
		
		if (redraw) redraw_terminal();
	} else {
		draw_char(c, cur_line * FONT_HEIGHT, cur_pos_in_line * FONT_WIDTH, COLOR_WHITE);
		terminal_buf[cur_line][cur_pos_in_line++] = c;
	}
}

void puts(char *str) {
	for (; *str; str++) putc(*str);
}

char tohexchar(int x) {
	return x < 10 ? '0' + x : 'A' + (x - 10);
}
void puth(unsigned long long value, unsigned char digits_len) {
#define MAX_LEN 16
	static char buf[MAX_LEN + 1];
	buf[MAX_LEN] = '\0';
	char *ptr = buf + MAX_LEN;
	if (value == 0) *--ptr = '0';
	while (value) *--ptr = tohexchar(value % 16), value /= 16;
	
	int remaining = digits_len - (buf + MAX_LEN - ptr);
	if (remaining < 0) remaining = 0;
	
	for (int i = 0; i < remaining; i++) putc('0');
	for (; *ptr; ptr++) putc(*ptr);
}
