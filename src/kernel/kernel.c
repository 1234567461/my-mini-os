/* ==========================================
 * 内核主文件 - kernel.c
 * 功能：
 *   1. 内核主函数 kernel_main
 *   2. 初始化各个子系统
 *   3. 启动Shell
 * ========================================== */

#include "vga.h"
#include "idt.h"
#include "pic.h"
#include "isr.h"
#include "pit.h"
#include "keyboard.h"
#include "shell.h"
#include "types.h"
#include "memory.h"
#include "paging.h"
#include "heap.h"
#include "task.h"

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
    vga_puts("My Mini OS v0.3.0\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("32-bit Protected Mode Kernel\n");
    vga_putc('\n');

    /* 初始化内存管理 */
    vga_puts("Initializing memory management...\n");

    /* 初始化物理内存管理器 */
    pmm_init();
    vga_puts("  [✓] Physical Memory Manager (bitmap)\n");

    /* 初始化分页机制 */
    paging_init();
    vga_puts("  [✓] Paging (4GB virtual address space)\n");

    /* 初始化堆内存分配器 */
    heap_init();
    vga_puts("  [✓] Heap Allocator (kmalloc/kfree)\n");

    /* 初始化进程调度器 */
    task_init();
    vga_puts("  [✓] Task Scheduler (Round Robin)\n");

    vga_putc('\n');

    /* 初始化中断系统 */
    vga_puts("Initializing interrupt system...\n");

    /* 初始化IDT */
    idt_init();
    vga_puts("  [✓] IDT (Interrupt Descriptor Table)\n");

    /* 初始化PIC */
    pic_init();
    vga_puts("  [✓] PIC (Programmable Interrupt Controller)\n");

    /* 初始化ISR和IRQ */
    isr_init();
    vga_puts("  [✓] ISR (Interrupt Service Routines)\n");
    vga_puts("  [✓] IRQ (Hardware Interrupts)\n");

    /* 初始化时钟 */
    pit_init(100);  /* 100Hz */
    vga_puts("  [✓] PIT (Programmable Interval Timer) - 100Hz\n");

    /* 初始化键盘 */
    keyboard_init();
    vga_puts("  [✓] PS/2 Keyboard Driver\n");

    /* 开中断 */
    sti();
    vga_puts("  [✓] Interrupts enabled\n");

    vga_putc('\n');
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("All systems initialized successfully!\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* 启动Shell */
    shell_start();

    /* 不应该到这里 */
    while (1) {
        hlt();
    }
}
