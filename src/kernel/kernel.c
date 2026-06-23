/* ==========================================
 * 内核主文件 - kernel.c
 * 功能：
 *   1. 内核主函数 kernel_main
 *   2. 初始化各个子系统
 *   3. 启动Shell和测试进程
 * ========================================== */

#include "vga.h"
#include "idt.h"
#include "pic.h"
#include "isr.h"
#include "pit.h"
#include "keyboard.h"
#include "memory.h"
#include "paging.h"
#include "heap.h"
#include "process.h"
#include "syscall.h"
#include "fs.h"
#include "shell.h"
#include "types.h"

/* ==========================================
 * 外部函数声明
 * ========================================== */
extern void sti(void);
extern void hlt(void);

/* ==========================================
 * 测试进程1：计数器
 * ========================================== */
static void test_process1(void)
{
    int count = 0;
    while (1) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_printf("[Process 1] Count: %d\n", count++);
        /* 主动让出CPU */
        for (int i = 0; i < 1000000; i++) {
            asm volatile ("nop");
        }
        process_yield();
    }
}

/* ==========================================
 * 测试进程2：另一个计数器
 * ========================================== */
static void test_process2(void)
{
    int count = 0;
    while (1) {
        vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
        vga_printf("[Process 2] Count: %d\n", count++);
        /* 主动让出CPU */
        for (int i = 0; i < 1000000; i++) {
            asm volatile ("nop");
        }
        process_yield();
    }
}

/* ==========================================
 * 测试进程3：显示时间
 * ========================================== */
static void test_process3(void)
{
    while (1) {
        vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        vga_printf("[Process 3] Ticks: %d\n", pit_get_ticks());
        for (int i = 0; i < 500000; i++) {
            asm volatile ("nop");
        }
        process_yield();
    }
}

/* ==========================================
 * 函数：kernel_main
 * 功能：内核主函数，从汇编入口调用
 * ========================================== */
void kernel_main(void)
{
    /* 初始化VGA */
    vga_init();

    /* 显示标题 */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  __  __         _  __ _       ___  ____  \n");
    vga_puts(" |  \\/  |_   _  | |/ /(_)_ __ / _ \\/ ___| \n");
    vga_puts(" | |\\/| | | | | | ' / | | '_ \\ | | \\___ \\ \n");
    vga_puts(" | |  | | |_| | | . \\ | | | | | |_| |___) |\n");
    vga_puts(" |_|  |_|\\__, | |_|\\_\\|_|_| |_|\\___/|____/ \n");
    vga_puts("         |___/                              \n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_putc('\n');

    /* 显示系统信息 */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("My Mini OS v0.3.0 (Phase 3)\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("32-bit Protected Mode Kernel\n");
    vga_putc('\n');

    /* 初始化中断系统 */
    vga_puts("Initializing interrupt system...\n");

    /* 初始化IDT */
    idt_init();
    vga_puts("  [OK] IDT (Interrupt Descriptor Table)\n");

    /* 初始化PIC */
    pic_init();
    vga_puts("  [OK] PIC (Programmable Interrupt Controller)\n");

    /* 初始化ISR和IRQ */
    isr_init();
    vga_puts("  [OK] ISR (Interrupt Service Routines)\n");
    vga_puts("  [OK] IRQ (Hardware Interrupts)\n");

    /* 初始化时钟 */
    pit_init(100);  /* 100Hz */
    vga_puts("  [OK] PIT (Programmable Interval Timer) - 100Hz\n");

    /* 初始化键盘 */
    keyboard_init();
    vga_puts("  [OK] PS/2 Keyboard Driver\n");

    /* 初始化内存管理 */
    vga_puts("Initializing memory management...\n");
    pmm_init();
    vga_puts("  [OK] Physical Memory Manager (bitmap)\n");

    /* 初始化分页 */
    paging_init();
    vga_puts("  [OK] Paging (virtual memory)\n");

    /* 初始化堆内存分配器 */
    heap_init();
    vga_puts("  [OK] Heap Memory Allocator (malloc/free)\n");

    /* 初始化进程管理 */
    process_init();
    vga_puts("  [OK] Process Manager (scheduler + context switch)\n");

    /* 初始化系统调用 */
    syscall_init();
    vga_puts("  [OK] System Call Interface (int 0x80)\n");

    /* 初始化文件系统 */
    fs_init();
    vga_puts("  [OK] Virtual File System (ramfs)\n");

    /* 开中断 */
    sti();
    vga_puts("  [OK] Interrupts enabled\n");

    vga_putc('\n');
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("All systems initialized successfully!\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_putc('\n');

    /* ==========================================
     * 创建测试进程
     * ========================================== */
    vga_puts("Creating test processes...\n");

    process_create("counter1", test_process1, 1);
    process_create("counter2", test_process2, 1);
    process_create("timer", test_process3, 1);

    vga_printf("Total processes: %d\n\n", process_get_ready_count());

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Starting scheduler... (watch the processes run!)\n");
    vga_puts("Press any key to enter shell\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* 启动调度器（第一次调度） */
    schedule();

    /* 注意：第一次调度后，不会回到这里 */
    /* 但是如果所有进程都退出了，可能会回来 */

    /* 启动Shell作为主进程 */
    shell_start();

    /* 不应该到这里 */
    while (1) {
        hlt();
    }
}
