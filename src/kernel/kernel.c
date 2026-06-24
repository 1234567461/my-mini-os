/* ==========================================
 * 内核主文件 - kernel.c v0.7.0
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
#include "syscall.h"
#include "shell.h"
#include "types.h"
#include "task.h"
#include "klog.h"
#include "vfs.h"
#include "ramfs.h"
#include "user.h"
#include "system.h"
#include "block.h"
#include "ata.h"
#include "fat16.h"
#include "fat32.h"
#include "mbr.h"
#include "rtc.h"
#include "serial.h"
#include "mouse.h"
#include "ipc.h"
#include "gdt.h"

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
        task_yield();
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
        task_yield();
    }
}

/* ==========================================
 * 测试进程3：显示时间
 * ========================================== */
static void test_process3(void)
{
    while (1) {
        vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
        vga_printf("[Process 3] Ticks: %d\n", pit_get_ticks());
        for (int i = 0; i < 500000; i++) {
            asm volatile ("nop");
        }
        task_yield();
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

    /* 初始化内核日志系统（最早初始化） */
    klog_init();
    klog_log("boot", "My Mini OS v0.7.0 starting up...");
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
    vga_puts("My Mini OS v0.6.0\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("32-bit Protected Mode Kernel\n");
    vga_puts("Memory: 128MB physical, 4MB large page support\n");
    vga_putc('\n');

    /* 初始化内存管理 */
    vga_puts("Initializing memory management...\n");
    klog_log("mem", "Initializing memory management subsystem");

    /* 初始化GDT和TSS */
    gdt_init();
    tss_init();
    vga_puts("  [✓] GDT and TSS (Global Descriptor Table)\n");
    klog_log("mem", "GDT and TSS initialized");

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

    /* 初始化实时时钟 */
    rtc_init();
    klog_log("drv", "RTC real-time clock initialized");

    /* 初始化串口 */
    serial_init_com1();
    klog_log("drv", "Serial port COM1 initialized (115200 baud)");

    /* 初始化鼠标 */
    mouse_init();
    klog_log("drv", "PS/2 mouse driver initialized");

    /* 开中断 */
    sti();
    vga_puts("  [✓] Interrupts enabled\n");
    klog_log("int", "Interrupts enabled (IF flag set)");

    vga_putc('\n');

    /* 初始化块设备层和磁盘驱动 */
    vga_puts("Initializing storage subsystem...\n");
    klog_log("drv", "Initializing storage subsystem");

    /* 初始化块设备层 */
    block_init();
    vga_puts("  [✓] Block Device Layer (buffer cache)\n");
    klog_log("drv", "Block device layer initialized (64 buffers)");

    /* 初始化ATA/IDE磁盘驱动 */
    ata_init();
    vga_puts("  [✓] ATA/IDE Disk Driver (PIO mode)\n");
    klog_log("drv", "ATA/IDE disk driver initialized (PIO mode, 28-bit LBA)");

    /* 注册ATA为块设备 */
    ata_register_as_block_device();
    klog_log("drv", "ATA disk registered as block device 0");

    /* 检测磁盘分区 */
    vga_puts("Detecting disk partitions...\n");
    klog_log("fs", "Detecting disk partitions");

    partition_info_t partitions[4];
    int part_count = mbr_list_partitions(0, partitions, 4);
    if (part_count > 0) {
        vga_printf("  Found %d partition(s)\n", part_count);
        klog_logf("fs", "Found %d MBR partition(s)", part_count);
        
        for (int i = 0; i < part_count; i++) {
            vga_printf("    Partition %d: %s, %d MB, LBA start: %d\n",
                       partitions[i].index,
                       partitions[i].type_name,
                       partitions[i].size_mb,
                       partitions[i].start_lba);
        }
    } else {
        vga_puts("  No valid MBR partition table found\n");
        klog_log("fs", "No valid MBR partition table found");
    }

    /* 尝试挂载FAT文件系统 */
    vga_puts("Detecting file systems...\n");
    klog_log("fs", "Detecting file systems on disk");

    /* 尝试FAT16 */
    vfs_filesystem_t *fat16_fs = fat16_init(0);
    if (fat16_fs != NULL) {
        vga_puts("  [✓] FAT16 file system detected\n");
        klog_log("fs", "FAT16 file system detected on device 0");
    }

    /* 尝试FAT32 */
    vfs_filesystem_t *fat32_fs = fat32_init(0);
    if (fat32_fs != NULL) {
        vga_puts("  [✓] FAT32 file system detected\n");
        klog_log("fs", "FAT32 file system detected on device 0");
    }

    vga_putc('\n');

    /* 初始化IPC子系统 */
    ipc_init();
    klog_log("ipc", "IPC subsystem initialized (pipes & signals)");

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("All systems initialized successfully!\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    klog_log("boot", "All systems initialized successfully");
    klog_log("boot", "Starting user shell...");

    /* ==========================================
     * 创建测试进程
     * ========================================== */
    vga_puts("Creating test processes...\n");

    task_create("counter1", test_process1, 1);
    task_create("counter2", test_process2, 1);
    task_create("timer", test_process3, 1);

    vga_printf("Total processes: %d\n\n", get_task_count());

    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
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
    klog_log("boot", "WARNING: Shell returned, halting");
    while (1) {
        hlt();
    }
}
