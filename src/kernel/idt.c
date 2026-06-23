/* ==========================================
 * IDT - 中断描述符表实现
 * ========================================== */

#include "idt.h"
#include "string.h"

/* IDT表 */
static idt_entry_t idt_entries[IDT_ENTRIES];

/* IDT指针（全局，汇编代码会引用） */
idt_ptr_t idt_ptr;

/* ==========================================
 * 设置IDT描述符
 * ========================================== */
void idt_set_gate(uint8_t num, uint32_t offset, uint16_t selector, uint8_t type_attr) {
    idt_entries[num].offset_low  = offset & 0xFFFF;
    idt_entries[num].offset_high = (offset >> 16) & 0xFFFF;
    idt_entries[num].selector    = selector;
    idt_entries[num].zero        = 0;
    idt_entries[num].type_attr   = type_attr;
}

/* ==========================================
 * 初始化IDT
 * ========================================== */
void idt_init() {
    /* 设置IDT指针 */
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    /* 清空IDT表 */
    memset(&idt_entries, 0, sizeof(idt_entry_t) * IDT_ENTRIES);

    /* 加载IDT（汇编函数） */
    idt_load();
}
