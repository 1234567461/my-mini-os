/* ==========================================
 * ISR - 中断服务例程（C语言部分）
 * ========================================== */

#include "isr.h"
#include "idt.h"
#include "vga.h"
#include "string.h"
#include "pic.h"

/* 中断处理函数表 */
static isr_handler_t isr_handlers[256] = {NULL};

/* 异常名称（用于显示） */
static const char *exception_names[] = {
    "Division Error",
    "Debug Exception",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

/* ==========================================
 * 注册中断处理函数
 * ========================================== */
void isr_register_handler(uint8_t int_no, isr_handler_t handler) {
    isr_handlers[int_no] = handler;
}

/* ==========================================
 * 通用ISR处理函数（被汇编代码调用）
 * ========================================== */
void isr_handler(isr_regs_t *regs) {
    /* 如果有注册的处理函数，调用它 */
    if (isr_handlers[regs->int_no] != NULL) {
        isr_handlers[regs->int_no](regs);
    } else {
        /* 默认异常处理 */
        isr_exception_handler(regs);
    }
}

/* ==========================================
 * 通用IRQ处理函数（被汇编代码调用）
 * ========================================== */
void irq_handler(isr_regs_t *regs) {
    /* 计算IRQ号 */
    uint8_t irq = regs->int_no - 32;

    /* 如果有注册的处理函数，调用它 */
    if (isr_handlers[regs->int_no] != NULL) {
        isr_handlers[regs->int_no](regs);
    }

    /* 发送EOI信号，告诉PIC中断处理完了 */
    pic_send_eoi(irq);
}

/* ==========================================
 * 默认异常处理函数
 * ========================================== */
void isr_exception_handler(isr_regs_t *regs) {
    /* 保存当前颜色，设置为红色 */
    uint8_t old_color = vga_get_color();
    vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);

    vga_printf("\n*** CPU Exception: %s (int %d) ***\n",
               exception_names[regs->int_no], regs->int_no);

    if (regs->err_code != 0) {
        vga_printf("Error code: 0x%x\n", regs->err_code);
    }

    vga_printf("EIP: 0x%x  CS: 0x%x  EFLAGS: 0x%x\n",
               regs->eip, regs->cs, regs->eflags);

    vga_printf("EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n",
               regs->eax, regs->ebx, regs->ecx, regs->edx);

    vga_printf("ESI: 0x%x  EDI: 0x%x  EBP: 0x%x  ESP: 0x%x\n",
               regs->esi, regs->edi, regs->ebp, regs->esp);

    vga_printf("DS: 0x%x\n", regs->ds);

    vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
    vga_puts("\nSystem halted!\n");

    /* 恢复颜色 */
    /* vga_set_color(old_color); */

    /* 停机 */
    for (;;) {
        asm volatile ("cli; hlt");
    }
}

/* ==========================================
 * 初始化ISR
 * ========================================== */
void isr_init() {
    /* 清空处理函数表 */
    memset(isr_handlers, 0, sizeof(isr_handler_t) * 256);

    /* 设置所有CPU异常的IDT条目 */
    idt_set_gate(0,  (uint32_t)isr0,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(20, (uint32_t)isr20, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(21, (uint32_t)isr21, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(22, (uint32_t)isr22, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(23, (uint32_t)isr23, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(24, (uint32_t)isr24, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(25, (uint32_t)isr25, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(26, (uint32_t)isr26, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(27, (uint32_t)isr27, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(28, (uint32_t)isr28, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(29, (uint32_t)isr29, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(30, (uint32_t)isr30, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(31, (uint32_t)isr31, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);

    /* 设置IRQ的IDT条目（32-47） */
    idt_set_gate(32, (uint32_t)irq0,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(33, (uint32_t)irq1,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(34, (uint32_t)irq2,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(35, (uint32_t)irq3,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(36, (uint32_t)irq4,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(37, (uint32_t)irq5,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(38, (uint32_t)irq6,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(39, (uint32_t)irq7,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(40, (uint32_t)irq8,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(41, (uint32_t)irq9,  0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
    idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_INT_GATE | IDT_PRESENT | IDT_RING0);
}
