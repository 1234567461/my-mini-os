/* ==========================================
 * VGA文本模式驱动
 * ========================================== */

#ifndef VGA_H
#define VGA_H

#include "types.h"

/* VGA文本缓冲区地址 */
#define VGA_BUFFER      0xB8000
#define VGA_WIDTH       80
#define VGA_HEIGHT      25

/* 颜色定义 */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15
} vga_color_t;

/* 初始化VGA */
void vga_init();

/* 清屏 */
void vga_clear();

/* 设置颜色 */
void vga_set_color(vga_color_t fg, vga_color_t bg);

/* 获取当前颜色 */
uint8_t vga_get_color();

/* 打印一个字符 */
void vga_putc(char c);

/* 打印字符串 */
void vga_puts(const char *str);

/* 打印格式化字符串（简化版） */
void vga_printf(const char *fmt, ...);

/* 设置光标位置 */
void vga_set_cursor(uint8_t x, uint8_t y);

/* 获取光标位置 */
void vga_get_cursor(uint8_t *x, uint8_t *y);

/* 滚动屏幕 */
void vga_scroll();

/* 向指定位置写字符 */
void vga_write_at(uint8_t x, uint8_t y, char c, uint8_t color);

#endif /* VGA_H */
