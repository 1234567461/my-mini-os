/* ==========================================
 * GDT - 全局描述符表头文件
 * ========================================== */

#ifndef GDT_H
#define GDT_H

#include "types.h"

/* GDT描述符结构 */
typedef struct {
    uint16_t limit_low;     /* 段界限低16位 */
    uint16_t base_low;      /* 基地址低16位 */
    uint8_t base_mid;       /* 基地址中间8位 */
    uint8_t access;         /* 访问标志 */
    uint8_t limit_high;     /* 段界限高4位 + flags */
    uint8_t base_high;      /* 基地址高8位 */
} __attribute__((packed)) gdt_entry_t;

/* GDT指针结构 */
typedef struct {
    uint16_t limit;         /* GDT大小 */
    uint32_t base;          /* GDT基地址 */
} __attribute__((packed)) gdt_ptr_t;

/* TSS结构（任务状态段） */
typedef struct {
    uint16_t prev_task;      /* 前一个任务链接 */
    uint16_t reserved1;
    uint32_t esp0;          /* 内核栈指针 */
    uint16_t ss0;           /* 内核栈段 */
    uint16_t reserved2;
    uint32_t esp1;          /* 备用内核栈 */
    uint16_t ss1;
    uint16_t reserved3;
    uint32_t esp2;          /* 备用内核栈 */
    uint16_t ss2;
    uint16_t reserved4;
    uint32_t cr3;           /* 页目录物理地址 */
    uint32_t eip;           /* 指令指针 */
    uint32_t eflags;        /* 标志寄存器 */
    uint32_t eax;           /* 通用寄存器 */
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;           /* 栈指针 */
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;           /* 索引寄存器 */
    uint16_t es;            /* 段寄存器 */
    uint16_t reserved5;
    uint16_t cs;            /* 代码段 */
    uint16_t reserved6;
    uint16_t ss;            /* 栈段 */
    uint16_t reserved7;
    uint16_t ds;            /* 数据段 */
    uint16_t reserved8;
    uint16_t fs;            /* 额外段 */
    uint16_t reserved9;
    uint16_t gs;            /* 额外段 */
    uint16_t reserved10;
    uint16_t ldt;           /* LDT选择子 */
    uint16_t reserved11;
    uint16_t trap_flag;     /* 陷阱标志 */
    uint16_t io_bitmap_offset; /* I/O位图偏移 */
} __attribute__((packed)) tss_t;

/* 外部变量声明 */
extern uint16_t kernel_code_sel;
extern uint16_t kernel_data_sel;
extern uint16_t user_code_sel;
extern uint16_t user_data_sel;
extern uint16_t tss_sel;

/* 函数声明 */
void gdt_init(void);
void tss_init(void);
void tss_update_kernel_stack(uint32_t kernel_stack);
uint32_t tss_get_base(void);
void gdt_flush(uint32_t gdt_ptr);
void tss_flush(uint16_t sel);

#endif /* GDT_H */