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
#include "klog.h"
#include "vfs.h"
#include "ramfs.h"

/* ==========================================
 * 函数：kernel_main
 * 功能：内核主函数，从汇编入口调用
 * ========================================== */
void kernel_main(void)
{
    /* 初始化VGA */
    vga_init();

    /* 初始化内核日志系统（最早初始化） */
    klog_init();
    klog_log("boot", "My Mini OS v0.4.0 starting up...");
    klog_log("boot", "Kernel loaded at 0x20000");

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
    vga_puts("My Mini OS v0.4.0\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("32-bit Protected Mode Kernel\n");
    vga_puts("Memory: 128MB physical, 4MB large page support\n");
    vga_putc('\n');

    /* 初始化内存管理 */
    vga_puts("Initializing memory management...\n");
    klog_log("mem", "Initializing memory management subsystem");

    /* 初始化物理内存管理器 */
    pmm_init();
    vga_puts("  [✓] Physical Memory Manager (bitmap)\n");
    klog_log("mem", "PMM initialized: 128MB physical memory, 32768 pages");

    /* 初始化分页机制 */
    paging_init();
    vga_puts("  [✓] Paging (4GB virtual address space, 4MB large pages)\n");
    klog_log("mem", "Paging initialized: 4GB virtual address space");
    klog_log("mem", "Kernel space: 0x00000000 - 0x003FFFFF (4MB)");
    klog_log("mem", "User space:   0x00400000 - 0x07FFFFFF (124MB)");

    /* 初始化堆内存分配器 */
    heap_init();
    vga_puts("  [✓] Heap Allocator (kmalloc/kfree)\n");
    klog_log("mem", "Heap allocator initialized (First Fit algorithm)");

    /* 初始化进程调度器 */
    task_init();
    vga_puts("  [✓] Task Scheduler (Round Robin)\n");
    klog_log("task", "Task scheduler initialized (Round Robin, 64 tasks max)");
    klog_log("task", "Idle task created (PID 0)");

    /* 初始化文件系统 */
    vga_puts("  [✓] Virtual File System (VFS)\n");
    klog_log("fs", "Initializing virtual file system");
    vfs_init();
    
    vfs_filesystem_t *ramfs = ramfs_init();
    if (ramfs != NULL) {
        vfs_mount_root(ramfs);
        vga_puts("  [✓] RAM File System (ramfs) mounted as root\n");
        klog_log("fs", "RAM filesystem mounted as root filesystem");
    }

    vga_putc('\n');

    /* 初始化中断系统 */
    vga_puts("Initializing interrupt system...\n");
    klog_log("int", "Initializing interrupt subsystem");

    /* 初始化IDT */
    idt_init();
    vga_puts("  [✓] IDT (Interrupt Descriptor Table)\n");
    klog_log("int", "IDT initialized (256 entries)");

    /* 初始化PIC */
    pic_init();
    vga_puts("  [✓] PIC (Programmable Interrupt Controller)\n");
    klog_log("int", "PIC initialized (master at 0x20, slave at 0xA0)");

    /* 初始化ISR和IRQ */
    isr_init();
    vga_puts("  [✓] ISR (Interrupt Service Routines)\n");
    vga_puts("  [✓] IRQ (Hardware Interrupts)\n");
    klog_log("int", "ISR handlers registered (0-31 CPU exceptions)");
    klog_log("int", "IRQ handlers registered (32-47 hardware interrupts)");

    /* 初始化时钟 */
    pit_init(100);  /* 100Hz */
    vga_puts("  [✓] PIT (Programmable Interval Timer) - 100Hz\n");
    klog_log("int", "PIT initialized: 100Hz system timer");

    /* 初始化键盘 */
    keyboard_init();
    vga_puts("  [✓] PS/2 Keyboard Driver\n");
    klog_log("drv", "PS/2 keyboard driver initialized");

    /* 开中断 */
    sti();
    vga_puts("  [✓] Interrupts enabled\n");
    klog_log("int", "Interrupts enabled (IF flag set)");

    vga_putc('\n');

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("All systems initialized successfully!\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    klog_log("boot", "All systems initialized successfully");
    klog_log("boot", "Starting user shell...");

    /* 启动Shell */
    shell_start();

    /* 不应该到这里 */
    klog_log("boot", "WARNING: Shell returned, halting");
    while (1) {
        hlt();
    }
}
