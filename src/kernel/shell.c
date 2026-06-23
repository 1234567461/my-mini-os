/* ==========================================
 * Shell命令行 - shell.c
 * 功能：
 *   1. 命令行交互界面
 *   2. 内置命令处理
 * ========================================== */

#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "pit.h"
#include "string.h"
#include "types.h"
#include "fs.h"
#include "process.h"
#include "syscall.h"

/* 命令缓冲区大小 */
#define CMD_BUF_SIZE 128

/* ==========================================
 * 内置命令列表
 * ========================================== */
static const char *help_text =
    "Available commands:\n"
    "  help     - Show this help message\n"
    "  clear    - Clear the screen\n"
    "  echo     - Echo text back (echo <text>)\n"
    "  about    - About My Mini OS\n"
    "  uptime   - Show system uptime\n"
    "  color    - Change text color (color <fg> <bg>)\n"
    "  date     - Show system ticks\n"
    "  version  - Show OS version\n"
    "  reboot   - Reboot (not implemented yet)\n"
    "  ls       - List files in current directory\n"
    "  cat      - Show file contents (cat <filename>)\n"
    "  touch    - Create a new file (touch <filename>)\n"
    "  rm       - Delete a file (rm <filename>)\n"
    "  ps       - List running processes\n"
    "  pid      - Show current process ID\n"
    "  yield    - Yield CPU to other processes\n";

/* ==========================================
 * 函数：cmd_help
 * 功能：显示帮助信息
 * ========================================== */
static void cmd_help(void)
{
    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_puts("My Mini OS - Help\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts(help_text);
}

/* ==========================================
 * 函数：cmd_clear
 * 功能：清屏
 * ========================================== */
static void cmd_clear(void)
{
    vga_clear();
}

/* ==========================================
 * 函数：cmd_echo
 * 功能：回显文字
 * ========================================== */
static void cmd_echo(const char *args)
{
    vga_puts(args);
    vga_putc('\n');
}

/* ==========================================
 * 函数：cmd_about
 * 功能：显示关于信息
 * ========================================== */
static void cmd_about(void)
{
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("My Mini OS\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("A tiny operating system built from scratch.\n\n");
    vga_puts("Features:\n");
    vga_puts("  - 16-bit real mode bootloader\n");
    vga_puts("  - 32-bit protected mode kernel\n");
    vga_puts("  - C language kernel\n");
    vga_puts("  - VGA text output\n");
    vga_puts("  - Interrupt handling (IDT/ISR)\n");
    vga_puts("  - Programmable Interrupt Controller\n");
    vga_puts("  - PIT timer (system clock)\n");
    vga_puts("  - PS/2 keyboard driver\n");
    vga_puts("  - Simple shell\n\n");
    vga_puts("Made with <3 for learning purposes\n");
}

/* ==========================================
 * 函数：cmd_uptime
 * 功能：显示系统运行时间
 * ========================================== */
static void cmd_uptime(void)
{
    uint32_t ticks = pit_get_ticks();
    uint32_t seconds = ticks / 100;  /* 假设100Hz */
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;

    vga_printf("Uptime: %dh %dm %ds\n",
               hours, minutes % 60, seconds % 60);
    vga_printf("Ticks: %d\n", ticks);
}

/* ==========================================
 * 函数：cmd_version
 * 功能：显示版本信息
 * ========================================== */
static void cmd_version(void)
{
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("My Mini OS ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("v0.2.0\n");
    vga_puts("Kernel: 32-bit protected mode\n");
    vga_puts("Build: Learning Edition\n");
}

/* ==========================================
 * 函数：cmd_color
 * 功能：改变文字颜色
 * ========================================== */
static void cmd_color(const char *args)
{
    int fg = 0, bg = 0;
    const char *p = args;

    /* 跳过开头的空格 */
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    /* 解析前景色 */
    while (*p >= '0' && *p <= '9') {
        fg = fg * 10 + (*p - '0');
        p++;
    }

    /* 跳过中间的空格 */
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    /* 解析背景色 */
    while (*p >= '0' && *p <= '9') {
        bg = bg * 10 + (*p - '0');
        p++;
    }

    if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
        vga_puts("Usage: color <fg> <bg>\n");
        vga_puts("Colors: 0=black, 1=blue, 2=green, ..., 15=white\n");
        return;
    }

    vga_set_color(fg, bg);
    vga_puts("Color changed!\n");
}

/* ==========================================
 * 函数：cmd_ls
 * 功能：列出文件
 * ========================================== */
static void cmd_ls(void)
{
    char buf[512];
    fs_listdir("/", buf, sizeof(buf));
    vga_puts(buf);
}

/* ==========================================
 * 函数：cmd_cat
 * 功能：显示文件内容
 * ========================================== */
static void cmd_cat(const char *filename)
{
    if (!filename || *filename == '\0') {
        vga_puts("Usage: cat <filename>\n");
        return;
    }

    int fd = fs_open(filename, O_RDONLY);
    if (fd < 0) {
        vga_printf("cat: %s: No such file\n", filename);
        return;
    }

    char buf[256];
    int n;
    while ((n = fs_read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        vga_puts(buf);
    }

    fs_close(fd);
}

/* ==========================================
 * 函数：cmd_touch
 * 功能：创建文件
 * ========================================== */
static void cmd_touch(const char *filename)
{
    if (!filename || *filename == '\0') {
        vga_puts("Usage: touch <filename>\n");
        return;
    }

    if (fs_exists(filename)) {
        vga_printf("touch: %s: File already exists\n", filename);
        return;
    }

    int idx = fs_creat(filename);
    if (idx < 0) {
        vga_puts("touch: Failed to create file\n");
        return;
    }

    vga_printf("Created file: %s\n", filename);
}

/* ==========================================
 * 函数：cmd_rm
 * 功能：删除文件
 * ========================================== */
static void cmd_rm(const char *filename)
{
    if (!filename || *filename == '\0') {
        vga_puts("Usage: rm <filename>\n");
        return;
    }

    if (fs_unlink(filename) < 0) {
        vga_printf("rm: %s: No such file\n", filename);
        return;
    }

    vga_printf("Removed file: %s\n", filename);
}

/* ==========================================
 * 函数：cmd_ps
 * 功能：列出进程
 * ========================================== */
static void cmd_ps(void)
{
    process_t *current = process_get_current();

    vga_puts("PID  Name          State\n");
    vga_puts("---  ------------  -------\n");

    if (current) {
        const char *state_str = "Unknown";
        switch (current->state) {
            case PROCESS_RUNNING:  state_str = "Running"; break;
            case PROCESS_READY:    state_str = "Ready"; break;
            case PROCESS_BLOCKED:  state_str = "Blocked"; break;
            case PROCESS_TERMINATED: state_str = "Terminated"; break;
        }
        vga_printf("%3d  %-12s  %s\n", current->pid, current->name, state_str);
    }

    int ready_count = process_get_ready_count();
    vga_printf("\nReady processes: %d\n", ready_count);
}

/* ==========================================
 * 函数：cmd_pid
 * 功能：显示当前进程ID
 * ========================================== */
static void cmd_pid(void)
{
    process_t *current = process_get_current();
    if (current) {
        vga_printf("Current PID: %d\n", current->pid);
    } else {
        vga_puts("No current process\n");
    }
}

/* ==========================================
 * 函数：cmd_yield
 * 功能：让出CPU
 * ========================================== */
static void cmd_yield(void)
{
    process_yield();
    vga_puts("Yielded CPU\n");
}

/* ==========================================
 * 函数：execute_command
 * 功能：执行一条命令
 * ========================================== */
static void execute_command(const char *cmd)
{
    const char *p = cmd;

    /* 跳过开头的空格 */
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    /* 空命令，什么都不做 */
    if (*p == '\0') {
        return;
    }

    /* 提取命令名（第一个空格前的部分） */
    char cmd_name[32];
    int i = 0;
    while (*p && *p != ' ' && *p != '\t' && i < 31) {
        cmd_name[i++] = *p++;
    }
    cmd_name[i] = '\0';

    /* 参数部分 */
    const char *args = p;
    while (*args == ' ' || *args == '\t') {
        args++;
    }

    /* 匹配命令 */
    if (strcmp(cmd_name, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd_name, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(cmd_name, "echo") == 0) {
        cmd_echo(args);
    } else if (strcmp(cmd_name, "about") == 0) {
        cmd_about();
    } else if (strcmp(cmd_name, "uptime") == 0) {
        cmd_uptime();
    } else if (strcmp(cmd_name, "version") == 0) {
        cmd_version();
    } else if (strcmp(cmd_name, "color") == 0) {
        cmd_color(args);
    } else if (strcmp(cmd_name, "date") == 0) {
        vga_printf("System ticks: %d\n", pit_get_ticks());
    } else if (strcmp(cmd_name, "reboot") == 0) {
        vga_puts("Reboot not implemented yet. Press Ctrl+Alt+Del in QEMU.\n");
    } else if (strcmp(cmd_name, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(cmd_name, "cat") == 0) {
        cmd_cat(args);
    } else if (strcmp(cmd_name, "touch") == 0) {
        cmd_touch(args);
    } else if (strcmp(cmd_name, "rm") == 0) {
        cmd_rm(args);
    } else if (strcmp(cmd_name, "ps") == 0) {
        cmd_ps();
    } else if (strcmp(cmd_name, "pid") == 0) {
        cmd_pid();
    } else if (strcmp(cmd_name, "yield") == 0) {
        cmd_yield();
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_printf("Command not found: %s\n", cmd_name);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_puts("Type 'help' for available commands.\n");
    }
}

/* ==========================================
 * 函数：shell_start
 * 功能：启动Shell主循环
 * ========================================== */
void shell_start(void)
{
    char cmd_buf[CMD_BUF_SIZE];

    vga_puts("\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("Shell is ready! Type 'help' for commands.\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("\n");

    while (1) {
        /* 显示提示符 */
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts("mini-os> ");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

        /* 读取用户输入 */
        keyboard_gets(cmd_buf, CMD_BUF_SIZE);

        /* 执行命令 */
        execute_command(cmd_buf);

        vga_puts("\n");
    }
}
