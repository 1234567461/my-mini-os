/* ==========================================
 * PIT - 可编程间隔定时器实现
 * ========================================== */

#include "pit.h"
#include "isr.h"
#include "pic.h"
#include "types.h"

/* 系统滴答计数器 */
static volatile uint32_t tick_count = 0;

/* 当前时钟频率 */
static uint32_t current_frequency = PIT_DEFAULT_HZ;

/* ==========================================
 * 时钟中断处理函数
 * ========================================== */
static void pit_handler(isr_regs_t *regs) {
    (void)regs;  /* 未使用的参数 */
    tick_count++;
}

/* ==========================================
 * 初始化PIT
 * ========================================== */
void pit_init(uint32_t frequency) {
    if (frequency == 0) {
        frequency = PIT_DEFAULT_HZ;
    }

    current_frequency = frequency;

    /* 计算分频系数 */
    uint32_t divisor = PIT_FREQUENCY / frequency;

    /* 设置控制字：
     * 0x36 = 0011 0110
     *   通道0
     *   读写低字节再高字节
     *   模式3（方波发生器）
     *   二进制计数
     */
    outb(PIT_CMD, 0x36);

    /* 发送分频系数（先低字节，后高字节） */
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);

    /* 注册中断处理函数 */
    isr_register_handler(32, pit_handler);  /* IRQ0 = 中断号32 */

    /* 开启IRQ0（时钟） */
    pic_irq_unmask(0);
}

/* ==========================================
 * 获取系统滴答数
 * ========================================== */
uint32_t pit_get_ticks() {
    return tick_count;
}

/* ==========================================
 * 简单的延时函数（毫秒）
 * ========================================== */
void pit_sleep(uint32_t ms) {
    uint32_t start = tick_count;
    uint32_t ticks_needed = (ms * current_frequency) / 1000;

    while (tick_count - start < ticks_needed) {
        /* 等待 */
        asm volatile ("hlt");  /* 停机等待中断，省电 */
    }
}
