/* ==========================================
 * GDT - 全局描述符表实现 v0.7.0
 * 功能：
 *   1. 内核代码/数据段描述符
 *   2. 用户代码/数据段描述符
 *   3. TSS（任务状态段）描述符
 *   4. 支持用户态切换
 * ========================================== */

#include "gdt.h"
#include "string.h"
#include "types.h"
#include "heap.h"

/* GDT表（最大256个描述符） */
#define GDT_MAX_ENTRIES 16
static gdt_entry_t gdt_entries[GDT_MAX_ENTRIES];

/* GDT指针 */
static gdt_ptr_t gdt_ptr;

/* TSS结构 */
static tss_t tss;

/* 段选择子 */
uint16_t kernel_code_sel = 0x08;   /* 内核代码段 */
uint16_t kernel_data_sel = 0x10;   /* 内核数据段 */
uint16_t user_code_sel = 0x1B;    /* 用户代码段 (RPL=3) */
uint16_t user_data_sel = 0x23;    /* 用户数据段 (RPL=3) */
uint16_t tss_sel = 0x28;          /* TSS段 */

/* ==========================================
 * 设置GDT描述符
 * ========================================== */
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    if (num >= GDT_MAX_ENTRIES) {
        return;
    }

    gdt_entry_t *entry = &gdt_entries[num];

    /* 基地址 */
    entry->base_low = base & 0xFFFF;
    entry->base_mid = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;

    /* 段界限 */
    entry->limit_low = limit & 0xFFFF;
    entry->limit_high = (limit >> 16) & 0x0F;

    /* 访问标志 */
    entry->access = access;

    /* 粒度和标志（放入limit_high的高4位） */
    entry->limit_high |= (flags << 4) & 0xF0;
}

/* ==========================================
 * 设置TSS描述符
 * ========================================== */
static void gdt_set_tss(int num, uint32_t base, uint32_t limit) {
    if (num >= GDT_MAX_ENTRIES) {
        return;
    }

    gdt_entry_t *entry = &gdt_entries[num];

    /* 基地址 */
    entry->base_low = base & 0xFFFF;
    entry->base_mid = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;

    /* 段界限 */
    entry->limit_low = limit & 0xFFFF;
    entry->limit_high = (limit >> 16) & 0x0F;

    /* TSS访问字节：存在、DPL=0、32位TSS */
    entry->access = 0x89;  /* present, DPL=0, 32-bit TSS */

    /* 标志：粒度=0，保留=0 */
    entry->limit_high &= 0x0F;
}

/* ==========================================
 * 初始化TSS
 * ========================================== */
void tss_init(void) {
    memset(&tss, 0, sizeof(tss_t));

    /* 设置内核栈指针（暂时用0） */
    tss.esp0 = 0;
    tss.ss0 = kernel_data_sel;

    /* 设置I/O位图偏移（不使用） */
    tss.io_bitmap_offset = sizeof(tss_t);
}

/* ==========================================
 * 更新TSS的内核栈指针
 * ========================================== */
void tss_update_kernel_stack(uint32_t kernel_stack) {
    tss.esp0 = kernel_stack;
}

/* ==========================================
 * 获取TSS基地址
 * ========================================== */
uint32_t tss_get_base(void) {
    return (uint32_t)&tss;
}

/* ==========================================
 * 加载GDT（汇编函数）
 * ========================================== */
extern void gdt_flush(uint32_t gdt_ptr);

/* ==========================================
 * 加载TR寄存器（汇编函数）
 * ========================================== */
extern void tss_flush(uint16_t sel);

/* ==========================================
 * 初始化GDT
 * ========================================== */
void gdt_init(void) {
    /* 设置GDT指针 */
    gdt_ptr.limit = sizeof(gdt_entry_t) * GDT_MAX_ENTRIES - 1;
    gdt_ptr.base = (uint32_t)gdt_entries;

    /* 清空GDT */
    memset(gdt_entries, 0, sizeof(gdt_entries));

    /* 0x00: 空描述符（必须） */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* 0x08: 内核代码段
     * 基址=0x00000000, 界限=0xFFFFF (4GB)
     * 存在、特权级0、代码段、可执行、可读
     */
    gdt_set_gate(1, 0x00000000, 0xFFFFF, 0x9A, 0x0F);

    /* 0x10: 内核数据段
     * 基址=0x00000000, 界限=0xFFFFF (4GB)
     * 存在、特权级0、数据段、可写
     */
    gdt_set_gate(2, 0x00000000, 0xFFFFF, 0x92, 0x0F);

    /* 0x18: 用户代码段
     * 基址=0x00000000, 界限=0xFFFFF (4GB)
     * 存在、特权级3、代码段、可执行、可读
     */
    gdt_set_gate(3, 0x00000000, 0xFFFFF, 0xFA, 0x0F);

    /* 0x20: 用户数据段
     * 基址=0x00000000, 界限=0xFFFFF (4GB)
     * 存在、特权级3、数据段、可写
     */
    gdt_set_gate(4, 0x00000000, 0xFFFFF, 0xF2, 0x0F);

    /* 0x28: TSS描述符
     * 基址=tss地址, 界限=sizeof(tss_t)
     */
    gdt_set_tss(5, (uint32_t)&tss, sizeof(tss_t) - 1);

    /* 加载GDT */
    gdt_flush((uint32_t)&gdt_ptr);

    /* 加载TR寄存器 */
    tss_flush(tss_sel);

    /* 更新TSS的内核栈 */
    tss_update_kernel_stack((uint32_t)0x90000);  /* 临时值，后续会更新 */
}

/* ==========================================
 * 获取当前特权的段选择子
 * ========================================== */
uint16_t gdt_get_cs(void) {
    uint16_t cs;
    asm volatile ("mov %%cs, %0" : "=r"(cs));
    return cs;
}

uint16_t gdt_get_ds(void) {
    uint16_t ds;
    asm volatile ("mov %%ds, %%ax; mov %%ax, %0" : "=r"(ds));
    return ds;
}