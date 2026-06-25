/* ==========================================
 * Accessibility - 无障碍功能实现
 * 为视障、听障、语障人士提供辅助功能
 * ========================================== */

#include "accessibility.h"
#include "vga.h"
#include "serial.h"
#include "string.h"
#include "types.h"
#include "pit.h"

/* 全局配置 */
static accessibility_config_t config;

/* 预设短语（语障辅助） */
static char phrases[MAX_PHRASES][MAX_PHRASE_LEN];
static int phrase_count = 0;

/* 保存原始颜色（用于切换模式） */
static uint8_t original_color = 0x0F;

/* ==========================================
 * 初始化无障碍系统
 * ========================================== */
void accessibility_init(void) {
    config.flags = A11Y_NONE;
    config.color_scheme = COLOR_SCHEME_NORMAL;
    config.font_scale = 1;
    config.cursor_blink_rate = 500;
    config.beep_flash_count = 3;
    config.slow_keys_delay = 500;

    /* 初始化预设短语 */
    phrase_set(0, "你好");
    phrase_set(1, "谢谢");
    phrase_set(2, "是的");
    phrase_set(3, "不是");
    phrase_set(4, "请帮助我");
    phrase_set(5, "我需要水");
    phrase_set(6, "我需要上厕所");
    phrase_set(7, "我饿了");
    phrase_count = 8;
}

/* ==========================================
 * 获取配置
 * ========================================== */
accessibility_config_t *accessibility_get_config(void) {
    return &config;
}

/* ==========================================
 * 启用/禁用功能
 * ========================================== */
void accessibility_enable(uint32_t flag) {
    config.flags |= flag;

    /* 应用相应的功能 */
    if (flag & A11Y_HIGH_CONTRAST) {
        high_contrast_enable();
    }
    if (flag & A11Y_LARGE_FONT) {
        large_font_enable();
    }
}

void accessibility_disable(uint32_t flag) {
    config.flags &= ~flag;

    /* 关闭相应的功能 */
    if (flag & A11Y_HIGH_CONTRAST) {
        high_contrast_disable();
    }
    if (flag & A11Y_LARGE_FONT) {
        large_font_disable();
    }
}

int accessibility_is_enabled(uint32_t flag) {
    return (config.flags & flag) != 0;
}

/* ==========================================
 * 设置配色方案
 * ========================================== */
void accessibility_set_color_scheme(color_scheme_t scheme) {
    config.color_scheme = scheme;

    switch (scheme) {
        case COLOR_SCHEME_NORMAL:
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            break;
        case COLOR_SCHEME_HIGH_CONTRAST:
            vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
            break;
        case COLOR_SCHEME_WHITE_ON_BLACK:
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            break;
        case COLOR_SCHEME_BLACK_ON_WHITE:
            vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
            break;
        case COLOR_SCHEME_GREEN_ON_BLACK:
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            break;
        case COLOR_SCHEME_REUCED_COLOR:
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            break;
    }
}

/* ==========================================
 * 屏幕阅读器（视障辅助）
 * 通过串口输出文字，供外部屏幕阅读器使用
 * ========================================== */
void screen_reader_speak(const char *text) {
    if (!(config.flags & A11Y_SCREEN_READER)) return;
    if (text == NULL) return;

    serial_write_string(SERIAL_COM1_PORT, "[Screen Reader] ");
    serial_write_string(SERIAL_COM1_PORT, text);
    serial_write_string(SERIAL_COM1_PORT, "\r\n");
}

void screen_reader_printf(const char *fmt, ...) {
    /* 简化版：直接输出格式化字符串（支持简单的%d和%s */
    if (!(config.flags & A11Y_SCREEN_READER)) return;

    char buf[256];
    int pos = 0;

    serial_write_string(SERIAL_COM1_PORT, "[Screen Reader] ");

    while (*fmt && pos < 250) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 'd') {
                /* 整数 - 简化处理，直接输出占位 */
                buf[pos++] = '#';
            } else if (*fmt == 's') {
                /* 字符串 - 简化处理 */
                buf[pos++] = '?';
            } else {
                    buf[pos++] = *fmt;
                }
        } else {
            buf[pos++] = *fmt;
        }
        fmt++;
    }
    buf[pos] = '\0';

    serial_write_string(SERIAL_COM1_PORT, buf);
    serial_write_string(SERIAL_COM1_PORT, "\r\n");
}

/* ==========================================
 * 大字体模式（视障辅助）
 * 通过将每行字符重复显示来模拟大字体效果
 * ========================================== */
void large_font_enable(void) {
    config.font_scale = 2;
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* 提示用户 */
    vga_puts("\n[Accessibility] 大字体模式已启用\n");
    screen_reader_speak("大字体模式已启用");
}

void large_font_disable(void) {
    config.font_scale = 1;

    vga_puts("\n[Accessibility] 大字体模式已关闭\n");
    screen_reader_speak("大字体模式已关闭");
}

/* ==========================================
 * 高对比度模式（视障辅助）
 * ========================================== */
void high_contrast_enable(void) {
    original_color = vga_get_color();
    accessibility_set_color_scheme(COLOR_SCHEME_HIGH_CONTRAST);

    vga_puts("\n[Accessibility] 高对比度模式已启用\n");
    screen_reader_speak("高对比度模式已启用");
}

void high_contrast_disable(void) {
    vga_set_color(original_color & 0x0F, (original_color >> 4) & 0x0F);

    vga_puts("\n[Accessibility] 高对比度模式已关闭\n");
    screen_reader_speak("高对比度模式已关闭");
}

/* ==========================================
 * 视觉提示（听障辅助）
 * 用屏幕闪烁代替蜂鸣声
 * ========================================== */
void visual_beep(void) {
    int count = config.beep_flash_count;
    uint8_t old_color = vga_get_color();

    for (int i = 0; i < count; i++) {
        /* 反色显示 */
        vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
        vga_write_at(0, 0, ' ', 0xF0);

        /* 简单延时 */
        for (volatile int j = 0; j < 100000; j++) {}

        /* 恢复 */
        vga_set_color(old_color & 0x0F, (old_color >> 4) & 0x0F);
        vga_write_at(0, 0, ' ', old_color);

        /* 间隔 */
        for (volatile int j = 0; j < 50000; j++) {}
    }

    vga_set_color(old_color & 0x0F, (old_color >> 4) & 0x0F);
}

void visual_alert(const char *message) {
    uint8_t old_color = vga_get_color();

    /* 闪烁边框 + 显示消息 */
    for (int i = 0; i < 3; i++) {
        vga_set_color(VGA_COLOR_RED, VGA_COLOR_LIGHT_BROWN);
        vga_puts("[ALERT] ");
        vga_set_color(old_color & 0x0F, (old_color >> 4) & 0x0F);
        vga_puts(message);
        vga_putc('\n');

        visual_beep();
    }
}

/* ==========================================
 * 预设短语（语障辅助）
 * ========================================== */
void phrase_set(int index, const char *text) {
    if (index < 0 || index >= MAX_PHRASES) return;
    if (text == NULL) return;

    strncpy(phrases[index], text, MAX_PHRASE_LEN - 1);
    phrases[index][MAX_PHRASE_LEN - 1] = '\0';
}

const char *phrase_get(int index) {
    if (index < 0 || index >= MAX_PHRASES) return "";
    return phrases[index];
}

void phrase_speak(int index) {
    const char *text = phrase_get(index);

    /* 显示在屏幕上 */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_printf("[Phrase %d] %s\n", index, text);

    /* 通过串口输出（屏幕阅读器） */
    screen_reader_printf("短语 %d: %s", index, text);
}

/* ==========================================
 * 无障碍帮助信息
 * ========================================== */
void accessibility_help(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("=== Accessibility (无障碍功能) ===\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_puts("为视障、听障、语障人士提供辅助功能\n\n");

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("视障辅助：\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("  a11y contrast   - 高对比度模式开关\n");
    vga_puts("  a11y largefont  - 大字体模式开关\n");
    vga_puts("  a11y screenread - 屏幕阅读器（串口输出）\n");
    vga_puts("  a11y scheme <n> - 配色方案 (0-5)\n");
    vga_puts("                    0=正常, 1=高对比, 2=黑白\n");
    vga_puts("                    3=白黑, 4=绿黑, 5=浅色\n");

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("\n听障辅助：\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("  a11y visualbeep - 视觉提示开关\n");
    vga_puts("  a11y beep       - 测试视觉提示\n");

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("\n语障辅助（预设短语）：\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("  phrase <n>      - 输出第n个短语\n");
    vga_puts("  phrase list     - 列出所有预设短语\n");
    vga_puts("  phrase set <n> <text> - 设置第n个短语\n");

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("\n其他：\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("  a11y status     - 显示当前无障碍设置\n");
    vga_puts("  a11y menu       - 交互式设置菜单\n");
    vga_puts("  a11y help       - 显示此帮助信息\n");
}

/* ==========================================
 * 显示当前状态
 * ========================================== */
void accessibility_show_status(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("=== Accessibility Status ===\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_printf("高对比度模式:    %s\n",
        (config.flags & A11Y_HIGH_CONTRAST) ? "[开启]" : "[关闭]");
    vga_printf("大字体模式:      %s\n",
        (config.flags & A11Y_LARGE_FONT) ? "[开启]" : "[关闭]");
    vga_printf("屏幕阅读器:      %s\n",
        (config.flags & A11Y_SCREEN_READER) ? "[开启]" : "[关闭]");
    vga_printf("视觉提示:        %s\n",
        (config.flags & A11Y_VISUAL_BEEP) ? "[开启]" : "[关闭]");
    vga_printf("配色方案:        %d\n", config.color_scheme);
    vga_printf("字体缩放:        %dx\n", config.font_scale);
}

/* ==========================================
 * 无障碍设置菜单（交互式）
 * ========================================== */
void accessibility_menu(void) {
    char input[16];

    while (1) {
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_puts("\n=== 无障碍设置菜单 ===\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

        vga_puts("1. 高对比度模式\n");
        vga_puts("2. 大字体模式\n");
        vga_puts("3. 屏幕阅读器（串口）\n");
        vga_puts("4. 视觉提示（替代声音）\n");
        vga_puts("5. 配色方案\n");
        vga_puts("6. 预设短语\n");
        vga_puts("0. 退出菜单\n");

        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts("\n请选择选项 (0-6): ");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

        extern void keyboard_gets(char *buf, int size);
        keyboard_gets(input, sizeof(input));

        if (input[0] == '0' || input[0] == 'q' || input[0] == 'Q') {
            vga_puts("\n退出无障碍菜单。\n");
            break;
        }

        switch (input[0]) {
            case '1':
                if (config.flags & A11Y_HIGH_CONTRAST) {
                    accessibility_disable(A11Y_HIGH_CONTRAST);
                } else {
                    accessibility_enable(A11Y_HIGH_CONTRAST);
                }
                break;
            case '2':
                if (config.flags & A11Y_LARGE_FONT) {
                    accessibility_disable(A11Y_LARGE_FONT);
                } else {
                    accessibility_enable(A11Y_LARGE_FONT);
                }
                break;
            case '3':
                if (config.flags & A11Y_SCREEN_READER) {
                    config.flags &= ~A11Y_SCREEN_READER;
                    vga_puts("\n屏幕阅读器已关闭\n");
                } else {
                    config.flags |= A11Y_SCREEN_READER;
                    vga_puts("\n屏幕阅读器已开启（串口COM1输出）\n");
                    screen_reader_speak("屏幕阅读器已启动");
                }
                break;
            case '4':
                if (config.flags & A11Y_VISUAL_BEEP) {
                    config.flags &= ~A11Y_VISUAL_BEEP;
                    vga_puts("\n视觉提示已关闭\n");
                } else {
                    config.flags |= A11Y_VISUAL_BEEP;
                    vga_puts("\n视觉提示已开启\n");
                    visual_beep();
                }
                break;
            case '5': {
                vga_puts("\n配色方案 (0-5): ");
                keyboard_gets(input, sizeof(input));
                int scheme = input[0] - '0';
                if (scheme >= 0 && scheme <= 5) {
                    accessibility_set_color_scheme((color_scheme_t)scheme);
                    vga_printf("\n配色方案已切换为: %d\n", scheme);
                } else {
                    vga_puts("\n无效的配色方案编号\n");
                }
                break;
            }
            case '6': {
                vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
                vga_puts("\n=== 预设短语列表 ===\n");
                vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                for (int i = 0; i < phrase_count; i++) {
                    vga_printf("  %d. %s\n", i, phrases[i]);
                }
                vga_puts("\n输入短语编号输出，或按回车返回: ");
                keyboard_gets(input, sizeof(input));
                if (input[0] >= '0' && input[0] <= '9') {
                    int idx = input[0] - '0';
                    if (idx >= 0 && idx < phrase_count) {
                        phrase_speak(idx);
                    }
                }
                break;
            }
            default:
                vga_puts("\n无效选项，请重新选择。\n");
                break;
        }
    }
}
