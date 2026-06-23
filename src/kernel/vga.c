/* ==========================================
 * VGA文本模式驱动实现
 * ========================================== */

#include "vga.h"
#include "string.h"
#include "types.h"

/* 全局变量 */
static uint8_t  cursor_x = 0;
static uint8_t  cursor_y = 0;
static uint8_t  current_color = 0x0F;  /* 黑底白字 */
static uint16_t *video_buffer = (uint16_t *)VGA_BUFFER;

/* ==========================================
 * 内部辅助函数
 * ========================================== */

/* 创建一个VGA字符（字符 + 属性） */
static uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | (uint16_t)color << 8;
}

/* 创建颜色属性字节 */
static uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return fg | bg << 4;
}

/* ==========================================
 * 公共接口
 * ========================================== */

/* 初始化VGA */
void vga_init() {
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    cursor_x = 0;
    cursor_y = 0;
}

/* 清屏 */
void vga_clear() {
    uint16_t blank = vga_entry(' ', current_color);
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            video_buffer[y * VGA_WIDTH + x] = blank;
        }
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_set_cursor(0, 0);
}

/* 设置颜色 */
void vga_set_color(vga_color_t fg, vga_color_t bg) {
    current_color = vga_entry_color(fg, bg);
}

/* 获取当前颜色 */
uint8_t vga_get_color() {
    return current_color;
}

/* 向指定位置写字符 */
void vga_write_at(uint8_t x, uint8_t y, char c, uint8_t color) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) return;
    video_buffer[y * VGA_WIDTH + x] = vga_entry(c, color);
}

/* 打印一个字符 */
void vga_putc(char c) {
    switch (c) {
        case '\n':
            /* 换行 */
            cursor_x = 0;
            cursor_y++;
            break;

        case '\r':
            /* 回车 */
            cursor_x = 0;
            break;

        case '\t':
            /* 制表符（4个空格） */
            cursor_x = (cursor_x + 4) & ~3;
            break;

        case '\b':
            /* 退格 */
            if (cursor_x > 0) {
                cursor_x--;
                vga_write_at(cursor_x, cursor_y, ' ', current_color);
            }
            break;

        default:
            /* 普通字符 */
            vga_write_at(cursor_x, cursor_y, c, current_color);
            cursor_x++;
            break;
    }

    /* 检查是否需要换行 */
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    /* 检查是否需要滚动 */
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
        cursor_y = VGA_HEIGHT - 1;
    }

    /* 更新硬件光标 */
    vga_set_cursor(cursor_x, cursor_y);
}

/* 打印字符串 */
void vga_puts(const char *str) {
    while (*str) {
        vga_putc(*str++);
    }
}

/* 滚动屏幕（向上滚动一行） */
void vga_scroll() {
    /* 把第1行到最后一行向上移动一行 */
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            video_buffer[y * VGA_WIDTH + x] = video_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }

    /* 最后一行清空 */
    uint16_t blank = vga_entry(' ', current_color);
    for (int x = 0; x < VGA_WIDTH; x++) {
        video_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }
}

/* 设置硬件光标位置 */
void vga_set_cursor(uint8_t x, uint8_t y) {
    uint16_t pos = y * VGA_WIDTH + x;

    /* 发送高字节 */
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)(pos >> 8));

    /* 发送低字节 */
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    cursor_x = x;
    cursor_y = y;
}

/* 获取光标位置 */
void vga_get_cursor(uint8_t *x, uint8_t *y) {
    *x = cursor_x;
    *y = cursor_y;
}

/* ==========================================
 * 简化版 printf
 * ========================================== */

/* 打印数字 */
static void print_num(int num, int base) {
    char buf[32];
    itoa(num, buf, base);
    vga_puts(buf);
}

/* 打印无符号数字 */
static void print_unsigned(uint32_t num, int base) {
    char buf[32];
    char *ptr = buf + 31;
    *ptr = '\0';

    if (num == 0) {
        vga_putc('0');
        return;
    }

    while (num > 0) {
        int digit = num % base;
        if (digit < 10) {
            *--ptr = '0' + digit;
        } else {
            *--ptr = 'a' + digit - 10;
        }
        num /= base;
    }

    vga_puts(ptr);
}

/* 简化版 printf */
void vga_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'c':
                    vga_putc((char)va_arg(args, int));
                    break;
                case 's':
                    vga_puts(va_arg(args, char *));
                    break;
                case 'd':
                case 'i':
                    print_num(va_arg(args, int), 10);
                    break;
                case 'u':
                    print_unsigned(va_arg(args, uint32_t), 10);
                    break;
                case 'x':
                case 'X':
                    print_unsigned(va_arg(args, uint32_t), 16);
                    break;
                case 'p':
                    vga_puts("0x");
                    print_unsigned(va_arg(args, uint32_t), 16);
                    break;
                case '%':
                    vga_putc('%');
                    break;
                default:
                    vga_putc('%');
                    vga_putc(*fmt);
                    break;
            }
        } else {
            vga_putc(*fmt);
        }
        fmt++;
    }

    va_end(args);
}
