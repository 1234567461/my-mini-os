/* ==========================================
 * 系统功能实现 v0.4.0
 * 功能：
 *   1. 系统重启（通过8042键盘控制器）
 *   2. 系统停机
 *   3. CPU特性检测
 * ========================================== */
#include "system.h"
#include "types.h"

/* ==========================================
 * 8042 键盘控制器端口
 * ========================================== */
#define KEYBOARD_DATA_PORT     0x60
#define KEYBOARD_COMMAND_PORT  0x64

/* 8042 命令 */
#define KEYBOARD_CMD_READ_OUTPUT   0xD0
#define KEYBOARD_CMD_WRITE_OUTPUT  0xD1
#define KEYBOARD_CMD_RESET_CPU     0xFE

/* ==========================================
 * 从8042状态端口读取状态
 * ========================================== */
static inline uint8_t keyboard_read_status() {
    uint8_t status;
    asm volatile ("inb %1, %0" : "=a"(status) : "dN"(KEYBOARD_COMMAND_PORT));
    return status;
}

/* ==========================================
 * 等待8042输入缓冲区为空
 * ========================================== */
static void keyboard_wait_input() {
    /* 状态寄存器位1为0表示输入缓冲区为空，可以写入 */
    while (keyboard_read_status() & 0x02) {
        asm volatile ("nop");
    }
}

/* ==========================================
 * 等待8042输出缓冲区有数据
 * ========================================== */
static void keyboard_wait_output() {
    /* 状态寄存器位0为1表示输出缓冲区有数据，可以读取 */
    while (!(keyboard_read_status() & 0x01)) {
        asm volatile ("nop");
    }
}

/* ==========================================
 * 向8042发送命令
 * ========================================== */
static void keyboard_send_command(uint8_t cmd) {
    keyboard_wait_input();
    asm volatile ("outb %0, %1" : : "a"(cmd), "dN"(KEYBOARD_COMMAND_PORT));
}

/* ==========================================
 * 向8042数据端口写入数据
 * ========================================== */
static void keyboard_write_data(uint8_t data) {
    keyboard_wait_input();
    asm volatile ("outb %0, %1" : : "a"(data), "dN"(KEYBOARD_DATA_PORT));
}

/* ==========================================
 * 从8042数据端口读取数据
 * ========================================== */
static uint8_t keyboard_read_data() {
    keyboard_wait_output();
    uint8_t data;
    asm volatile ("inb %1, %0" : "=a"(data) : "dN"(KEYBOARD_DATA_PORT));
    return data;
}

/* ==========================================
 * 系统重启
 * 通过8042键盘控制器的CPU复位线实现
 * ========================================== */
void system_reboot() {
    /* 方法1：通过8042发送复位命令（最可靠） */
    
    /* 清空键盘缓冲区 */
    while (keyboard_read_status() & 0x01) {
        keyboard_read_data();
    }

    /* 发送复位命令（脉冲复位线） */
    keyboard_send_command(KEYBOARD_CMD_RESET_CPU);

    /* 等待重启，如果没成功就用备用方法 */
    for (int i = 0; i < 100000; i++) {
        asm volatile ("nop");
    }

    /* 备用方法：triple fault（让系统崩溃重启）
     * 加载一个空的IDT，然后触发一个中断
     */
    asm volatile (
        "lidt (0)\n"    /* 加载空IDT */
        "int $3\n"      /* 触发断点异常，导致triple fault */
    );

    /* 如果还没重启，就死循环 */
    while (1) {
        asm volatile ("hlt");
    }
}

/* ==========================================
 * 系统停机
 * ========================================== */
void system_halt() {
    asm volatile ("cli");  /* 关中断 */
    while (1) {
        asm volatile ("hlt");
    }
}

/* ==========================================
 * 检测CPU是否支持大页（4MB）
 * 通过CPUID指令检测PSE（Page Size Extension）位
 * ========================================== */
bool cpu_supports_large_pages() {
    uint32_t eax, edx;
    
    /* 检查CPUID是否可用 */
    /* 简单起见，假设支持CPUID（386以后都支持） */
    
    /* 调用CPUID功能1，获取特性标志 */
    asm volatile (
        "mov $1, %%eax\n"
        "cpuid\n"
        "mov %%edx, %0\n"
        : "=r"(edx)
        :
        : "eax", "ebx", "ecx"
    );
    
    /* 检查第3位（PSE - Page Size Extension） */
    return (edx & (1 << 3)) != 0;
}
