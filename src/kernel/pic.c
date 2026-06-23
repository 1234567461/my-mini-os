/* ==========================================
 * PIC - 可编程中断控制器实现
 * ========================================== */

#include "pic.h"
#include "types.h"

/* ==========================================
 * 发送中断结束信号（EOI）
 * ========================================== */
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        /* 从PIC的中断，需要给两个PIC都发EOI */
        outb(PIC_SLAVE_CMD, PIC_EOI);
    }
    outb(PIC_MASTER_CMD, PIC_EOI);
}

/* ==========================================
 * 初始化PIC并重映射中断号
 * 主PIC：IRQ0-7 -> 中断号32-39
 * 从PIC：IRQ8-15 -> 中断号40-47
 * ========================================== */
void pic_init() {
    /* 保存原来的屏蔽字 */
    uint8_t master_mask = inb(PIC_MASTER_DATA);
    uint8_t slave_mask  = inb(PIC_SLAVE_DATA);

    /* ICW1：开始初始化 */
    outb(PIC_MASTER_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    outb(PIC_SLAVE_CMD,  PIC_ICW1_INIT | PIC_ICW1_ICW4);

    /* ICW2：设置中断号偏移 */
    outb(PIC_MASTER_DATA, 0x20);  /* 主PIC：中断号从32开始 */
    outb(PIC_SLAVE_DATA,  0x28);  /* 从PIC：中断号从40开始 */

    /* ICW3：级联设置 */
    outb(PIC_MASTER_DATA, 0x04);  /* 主PIC：IRQ2连接从PIC */
    outb(PIC_SLAVE_DATA,  0x02);  /* 从PIC：连接到主PIC的IRQ2 */

    /* ICW4：设置模式 */
    outb(PIC_MASTER_DATA, PIC_ICW4_8086);
    outb(PIC_SLAVE_DATA,  PIC_ICW4_8086);

    /* 恢复屏蔽字（先全部屏蔽，后面按需开启） */
    outb(PIC_MASTER_DATA, master_mask);
    outb(PIC_SLAVE_DATA,  slave_mask);
}

/* ==========================================
 * 屏蔽某个IRQ
 * ========================================== */
void pic_irq_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);
    outb(port, value);
}

/* ==========================================
 * 开启某个IRQ
 * ========================================== */
void pic_irq_unmask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

/* ==========================================
 * 设置中断屏蔽字
 * ========================================== */
void pic_set_mask(uint8_t master_mask, uint8_t slave_mask) {
    outb(PIC_MASTER_DATA, master_mask);
    outb(PIC_SLAVE_DATA,  slave_mask);
}
