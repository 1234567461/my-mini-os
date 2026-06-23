/* ==========================================
 * PIT - 可编程间隔定时器（8253/8254）
 * ========================================== */

#ifndef PIT_H
#define PIT_H

#include "types.h"

/* PIT端口 */
#define PIT_CHANNEL0    0x40    /* 通道0（连接到IRQ0） */
#define PIT_CHANNEL1    0x41    /* 通道1（已废弃） */
#define PIT_CHANNEL2    0x42    /* 通道2（扬声器） */
#define PIT_CMD         0x43    /* 控制字寄存器 */

/* PIT输入频率（Hz） */
#define PIT_FREQUENCY   1193180

/* 默认时钟频率（Hz） */
#define PIT_DEFAULT_HZ  100

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化PIT，设置时钟频率 */
void pit_init(uint32_t frequency);

/* 获取系统启动以来的滴答数 */
uint32_t pit_get_ticks();

/* 简单的延时函数（毫秒） */
void pit_sleep(uint32_t ms);

#endif /* PIT_H */
