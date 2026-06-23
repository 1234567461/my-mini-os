/* ==========================================
 * PS/2 鼠标驱动头文件 - mouse.h
 * 功能：
 *   1. PS/2 鼠标驱动
 *   2. 支持鼠标移动、按键检测
 *   3. 鼠标数据包解析
 * ========================================== */
#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"

/* PS/2 控制器端口 */
#define MOUSE_DATA_PORT     0x60
#define MOUSE_CMD_PORT      0x64

/* PS/2 控制器命令 */
#define MOUSE_CMD_READ      0x20  /* 读取配置字节 */
#define MOUSE_CMD_WRITE     0x60  /* 写入配置字节 */
#define MOUSE_CMD_DISABLE_AUX 0xA7  /* 禁用辅助设备（鼠标） */
#define MOUSE_CMD_ENABLE_AUX  0xA8  /* 启用辅助设备（鼠标） */
#define MOUSE_CMD_TEST_AUX    0xA9  /* 测试辅助设备 */
#define MOUSE_CMD_WRITE_AUX   0xD4  /* 向辅助设备发送数据 */

/* 鼠标命令 */
#define MOUSE_SET_RESOLUTION  0xE8
#define MOUSE_SET_SAMPLING    0xF3
#define MOUSE_ENABLE_PACKET   0xF4
#define MOUSE_DISABLE_PACKET  0xF5
#define MOUSE_SET_DEFAULTS    0xF6
#define MOUSE_IDENTIFY        0xF2
#define MOUSE_SET_SCALING_1_1 0xE6
#define MOUSE_SET_SCALING_2_1 0xE7
#define MOUSE_RESEND          0xFE
#define MOUSE_RESET           0xFF

/* 鼠标应答 */
#define MOUSE_ACK             0xFA
#define MOUSE_NAK             0xFE
#define MOUSE_ERROR           0xFC

/* 鼠标数据包字节1的标志位 */
#define MOUSE_LEFT_BUTTON     0x01
#define MOUSE_RIGHT_BUTTON    0x02
#define MOUSE_MIDDLE_BUTTON   0x04
#define MOUSE_ALWAYS_1        0x08
#define MOUSE_X_SIGN          0x10
#define MOUSE_Y_SIGN          0x20
#define MOUSE_X_OVERFLOW      0x40
#define MOUSE_Y_OVERFLOW      0x80

/* 鼠标状态结构体 */
typedef struct {
    int32_t x;           /* X 坐标 */
    int32_t y;           /* Y 坐标 */
    int32_t dx;          /* X 方向位移 */
    int32_t dy;          /* Y 方向位移 */
    uint8_t buttons;     /* 按键状态 */
    bool left_pressed;   /* 左键是否按下 */
    bool right_pressed;  /* 右键是否按下 */
    bool middle_pressed; /* 中键是否按下 */
} mouse_state_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化 PS/2 鼠标 */
void mouse_init(void);

/* 鼠标中断处理函数 */
void mouse_handler(void);

/* 获取鼠标状态 */
void mouse_get_state(mouse_state_t *state);

/* 等待鼠标控制器就绪（读） */
static void mouse_wait_read(void);

/* 等待鼠标控制器就绪（写） */
static void mouse_wait_write(void);

/* 向鼠标发送命令 */
static void mouse_write(uint8_t data);

/* 从鼠标读取数据 */
static uint8_t mouse_read(void);

/* 鼠标数据包回调函数类型 */
typedef void (*mouse_callback_t)(mouse_state_t *state);

/* 注册鼠标回调函数 */
void mouse_register_callback(mouse_callback_t callback);

#endif /* MOUSE_H */
