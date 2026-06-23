/* ==========================================
 * 键盘驱动
 * ========================================== */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

/* 键盘数据端口 */
#define KEYBOARD_DATA_PORT  0x60
#define KEYBOARD_CMD_PORT   0x64

/* 键盘缓冲区大小 */
#define KEYBOARD_BUF_SIZE   256

/* 特殊键状态 */
typedef struct {
    bool shift;      /* Shift键 */
    bool ctrl;       /* Ctrl键 */
    bool alt;        /* Alt键 */
    bool caps_lock;  /* Caps Lock */
    bool num_lock;   /* Num Lock */
    bool scroll_lock;/* Scroll Lock */
} keyboard_state_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化键盘 */
void keyboard_init();

/* 从键盘缓冲区读取一个字符（阻塞） */
char keyboard_getc();

/* 从键盘缓冲区读取一行字符串 */
void keyboard_gets(char *buf, size_t max_len);

/* 检查缓冲区是否有数据 */
bool keyboard_has_data();

/* 获取键盘状态 */
keyboard_state_t *keyboard_get_state();

#endif /* KEYBOARD_H */
