/* ==========================================
 * PIC - 可编程中断控制器（8259A）
 * ========================================== */

#ifndef PIC_H
#define PIC_H

#include "types.h"

/* PIC端口 */
#define PIC_MASTER_CMD      0x20
#define PIC_MASTER_DATA     0x21
#define PIC_SLAVE_CMD       0xA0
#define PIC_SLAVE_DATA      0xA1

/* PIC命令 */
#define PIC_EOI             0x20    /* 中断结束 */

/* ICW1（初始化命令字1） */
#define PIC_ICW1_ICW4       0x01    /* 需要ICW4 */
#define PIC_ICW1_SINGLE     0x02    /* 单PIC模式（0=级联） */
#define PIC_ICW1_INTERVAL4  0x04    /* 调用地址间隔（0=8字节） */
#define PIC_ICW1_LEVEL      0x08    /* 电平触发（0=边沿触发） */
#define PIC_ICW1_INIT       0x10    /* 初始化命令 */

/* ICW4（初始化命令字4） */
#define PIC_ICW4_8086       0x01    /* 8086/88模式 */
#define PIC_ICW4_AUTO       0x02    /* 自动EOI */
#define PIC_ICW4_BUF_SLAVE  0x08    /* 缓冲模式（从PIC） */
#define PIC_ICW4_BUF_MASTER 0x0C    /* 缓冲模式（主PIC） */
#define PIC_ICW4_SFNM       0x10    /* 特殊全嵌套模式 */

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化PIC并重映射中断号 */
void pic_init();

/* 发送中断结束信号（EOI） */
void pic_send_eoi(uint8_t irq);

/* 屏蔽某个IRQ */
void pic_irq_mask(uint8_t irq);

/* 开启某个IRQ */
void pic_irq_unmask(uint8_t irq);

/* 设置中断屏蔽字 */
void pic_set_mask(uint8_t master_mask, uint8_t slave_mask);

#endif /* PIC_H */
