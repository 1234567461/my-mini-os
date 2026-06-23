/* ==========================================
 * ISR - 中断服务例程
 * ========================================== */

#ifndef ISR_H
#define ISR_H

#include "types.h"

/* ==========================================
 * CPU异常中断号（0-31）
 * ========================================== */
#define ISR0   0   /* 除法错误 */
#define ISR1   1   /* 调试异常 */
#define ISR2   2   /* NMI中断 */
#define ISR3   3   /* 断点 */
#define ISR4   4   /* 溢出 */
#define ISR5   5   /* 边界检查 */
#define ISR6   6   /* 无效操作码 */
#define ISR7   7   /* 设备不可用 */
#define ISR8   8   /* 双重故障 */
#define ISR9   9   /* 协处理器段溢出 */
#define ISR10  10  /* 无效TSS */
#define ISR11  11  /* 段不存在 */
#define ISR12  12  /* 栈段故障 */
#define ISR13  13  /* 通用保护故障 */
#define ISR14  14  /* 页故障 */
#define ISR15  15  /* 保留 */
#define ISR16  16  /* x87浮点异常 */
#define ISR17  17  /* 对齐检查 */
#define ISR18  18  /* 机器检查 */
#define ISR19  19  /* SIMD浮点异常 */
/* 20-31 保留 */

/* ==========================================
 * 中断栈帧结构
 * ==========================================
 * 当中断发生时，CPU会自动压入一些寄存器，
 * 然后我们的汇编代码会再压入一些，
 * 最后形成这个结构，传给C处理函数。
 * ========================================== */
typedef struct {
    /* 我们手动压入的 */
    uint32_t ds;                                     /* 数据段选择子 */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pushad */

    /* 中断号和错误码（我们的汇编代码压入的） */
    uint32_t int_no;
    uint32_t err_code;

    /* CPU自动压入的 */
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed)) isr_regs_t;

/* ==========================================
 * 中断处理函数类型
 * ========================================== */
typedef void (*isr_handler_t)(isr_regs_t *regs);

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化ISR */
void isr_init();

/* 注册中断处理函数 */
void isr_register_handler(uint8_t int_no, isr_handler_t handler);

/* 异常处理函数（默认） */
void isr_exception_handler(isr_regs_t *regs);

/* ==========================================
 * 汇编中定义的ISR入口点
 * ========================================== */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

/* IRQ入口点（32-47） */
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

#endif /* ISR_H */
