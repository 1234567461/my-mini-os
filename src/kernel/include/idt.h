/* ==========================================
 * IDT - 中断描述符表
 * ========================================== */

#ifndef IDT_H
#define IDT_H

#include "types.h"

/* IDT描述符数量 */
#define IDT_ENTRIES     256

/* IDT门类型 */
#define IDT_TASK_GATE   0x05
#define IDT_INT_GATE    0x0E   /* 32位中断门 */
#define IDT_TRAP_GATE   0x0F   /* 32位陷阱门 */

/* 特权级 */
#define IDT_RING0       0x00
#define IDT_RING3       0x60

/* 存在位 */
#define IDT_PRESENT     0x80

/* ==========================================
 * IDT描述符结构
 * ==========================================
 * 每个IDT描述符8字节，结构如下：
 *
 *  31                   16 15                    0
 *  +---------------------+-----------------------+
 *  |  段选择子（16位）    |  偏移量低16位          |
 *  +---------------------+-----------------------+
 *  |  属性+偏移量高16位   |                       |
 *  +---------------------+-----------------------+
 *
 * 属性字节：
 *  7   6 5   4 3       0
 *  +---+-----+---------+
 *  | P | DPL | 类型    |
 *  +---+-----+---------+
 *
 * P   = 存在位（1=存在）
 * DPL = 特权级（0=内核，3=用户）
 * 类型 = 门类型
 * ========================================== */

typedef struct {
    uint16_t offset_low;    /* 偏移量低16位 */
    uint16_t selector;      /* 段选择子（GDT中的代码段） */
    uint8_t  zero;          /* 保留，必须为0 */
    uint8_t  type_attr;     /* 类型和属性 */
    uint16_t offset_high;   /* 偏移量高16位 */
} __attribute__((packed)) idt_entry_t;

/* IDT寄存器（用于lidt指令） */
typedef struct {
    uint16_t limit;         /* IDT大小 - 1 */
    uint32_t base;          /* IDT基地址 */
} __attribute__((packed)) idt_ptr_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化IDT */
void idt_init();

/* 设置IDT描述符 */
void idt_set_gate(uint8_t num, uint32_t offset, uint16_t selector, uint8_t type_attr);

/* 加载IDT（汇编实现） */
extern void idt_load();

#endif /* IDT_H */
