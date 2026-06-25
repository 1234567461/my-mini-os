/* ==========================================
 * Accessibility - 无障碍功能支持
 * 为视障、听障、语障人士提供辅助功能
 * ========================================== */

#ifndef ACCESSIBILITY_H
#define ACCESSIBILITY_H

#include "types.h"

/* 无障碍模式标志位 */
#define A11Y_NONE           0x0000
#define A11Y_HIGH_CONTRAST  0x0001  /* 高对比度模式 */
#define A11Y_LARGE_FONT     0x0002  /* 大字体模式 */
#define A11Y_SCREEN_READER  0x0004  /* 屏幕阅读器（串口输出） */
#define A11Y_VISUAL_BEEP    0x0008  /* 视觉提示（替代声音） */
#define A11Y_BOLD_CURSOR    0x0010  /* 加粗光标 */
#define A11Y_SLOW_KEYS      0x0020  /* 慢速按键（防抖） */
#define A11Y_STICKY_KEYS    0x0040  /* 粘滞键 */

/* 预设配色方案 */
typedef enum {
    COLOR_SCHEME_NORMAL = 0,      /* 正常配色 */
    COLOR_SCHEME_HIGH_CONTRAST,   /* 高对比度（黑底黄字） */
    COLOR_SCHEME_WHITE_ON_BLACK,  /* 黑底白字 */
    COLOR_SCHEME_BLACK_ON_WHITE,  /* 白底黑字 */
    COLOR_SCHEME_GREEN_ON_BLACK,  /* 黑底绿字（复古终端） */
    COLOR_SCHEME_REUCED_COLOR     /* 低饱和度（浅色模式） */
} color_scheme_t;

/* 无障碍配置结构体 */
typedef struct {
    uint32_t flags;           /* 功能标志位 */
    color_scheme_t color_scheme; /* 配色方案 */
    int font_scale;           /* 字体缩放（1=正常, 2=双倍） */
    int cursor_blink_rate;    /* 光标闪烁速度（毫秒） */
    int beep_flash_count;     /* 视觉提示闪烁次数 */
    int slow_keys_delay;      /* 慢速按键延迟（毫秒） */
} accessibility_config_t;

/* ==========================================
 * 初始化与配置
 * ========================================== */
void accessibility_init(void);

/* 获取当前配置 */
accessibility_config_t *accessibility_get_config(void);

/* 启用/禁用特定功能 */
void accessibility_enable(uint32_t flag);
void accessibility_disable(uint32_t flag);
int accessibility_is_enabled(uint32_t flag);

/* 设置配色方案 */
void accessibility_set_color_scheme(color_scheme_t scheme);

/* ==========================================
 * 视障辅助功能
 * ========================================== */

/* 屏幕阅读器：输出文字到串口 */
void screen_reader_speak(const char *text);
void screen_reader_printf(const char *fmt, ...);

/* 大字体模式 */
void large_font_enable(void);
void large_font_disable(void);

/* 高对比度模式 */
void high_contrast_enable(void);
void high_contrast_disable(void);

/* ==========================================
 * 听障辅助功能
 * ========================================== */

/* 视觉提示（用屏幕闪烁代替蜂鸣声） */
void visual_beep(void);
void visual_alert(const char *message);

/* ==========================================
 * 语障辅助功能
 * ========================================== */

/* 预设短语 */
#define MAX_PHRASES 16
#define MAX_PHRASE_LEN 64

void phrase_set(int index, const char *text);
const char *phrase_get(int index);
void phrase_speak(int index);  /* 输出短语到屏幕和串口 */

/* ==========================================
 * 无障碍菜单
 * ========================================== */

/* 显示无障碍设置菜单 */
void accessibility_menu(void);

/* 显示无障碍帮助 */
void accessibility_help(void);

/* 显示当前状态 */
void accessibility_show_status(void);

#endif /* ACCESSIBILITY_H */
