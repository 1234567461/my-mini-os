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
#include "memory.h"
#include "heap.h"
#include "task.h"
#include "system.h"
#include "klog.h"
#include "user.h"
#include "vfs.h"

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
    "  meminfo  - Show memory usage information\n"
    "  ps       - List running processes\n"
    "  uname    - Show system information\n"
    "  hostname - Show system hostname\n"
    "  whoami   - Show current user and permission level\n"
    "  su       - Switch user (su <username> [password])\n"
    "  calc     - Simple calculator (calc <num1> <op> <num2>)\n"
    "  dmesg    - Show kernel boot log\n"
    "  reboot   - Reboot the system\n"
    "  ls       - List directory contents\n"
    "  cat      - Display file contents\n"
    "  mkdir    - Create a directory\n"
    "  rm       - Remove a file\n"
    "  cd       - Change directory\n"
    "  pwd      - Print working directory\n"
    "  touch    - Create empty file\n";

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
    vga_puts("  - Physical memory management\n");
    vga_puts("  - Paging support\n");
    vga_puts("  - Heap allocator (kmalloc/kfree)\n");
    vga_puts("  - Task scheduler (Round Robin)\n");
    vga_puts("  - Simple shell with many commands\n\n");
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
    vga_puts("v0.4.0\n");
    vga_puts("Kernel: 32-bit protected mode\n");
    vga_puts("Memory: 128MB physical, 4MB large page support\n");
    vga_puts("Features: memory isolation, per-process address space\n");
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
 * 函数：cmd_meminfo
 * 功能：显示内存使用信息
 * ========================================== */
static void cmd_meminfo(void)
{
    size_t total_pages = pmm_get_total_pages();
    size_t used_pages = pmm_get_used_pages();
    size_t free_pages = total_pages - used_pages;

    size_t total_mem_kb = (total_pages * PAGE_SIZE) / 1024;
    size_t used_mem_kb = (used_pages * PAGE_SIZE) / 1024;
    size_t free_mem_kb = (free_pages * PAGE_SIZE) / 1024;

    size_t heap_total_kb = heap_get_total() / 1024;
    size_t heap_used_kb = heap_get_used() / 1024;
    size_t heap_free_kb = heap_get_free() / 1024;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("Memory Information\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("==================\n\n");

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("Physical Memory:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_printf("  Total:  %d KB (%d pages)\n", total_mem_kb, total_pages);
    vga_printf("  Used:   %d KB (%d pages)\n", used_mem_kb, used_pages);
    vga_printf("  Free:   %d KB (%d pages)\n\n", free_mem_kb, free_pages);

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("Heap Memory:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_printf("  Total:  %d KB\n", heap_total_kb);
    vga_printf("  Used:   %d KB\n", heap_used_kb);
    vga_printf("  Free:   %d KB\n", heap_free_kb);
}

/* ==========================================
 * 函数：cmd_ps
 * 功能：显示进程列表
 * ========================================== */
static void cmd_ps(void)
{
    task_t *list = get_task_list();
    uint32_t count = get_task_count();

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("Process List\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("============\n\n");

    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_printf("%-5s %-16s %-10s %-8s %-8s\n", "PID", "NAME", "STATE", "PRIORITY", "SLICE");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("------------------------------------------------\n");

    task_t *p = list;
    while (p != NULL) {
        const char *state_str;
        switch (p->state) {
            case TASK_RUNNING:  state_str = "Running"; break;
            case TASK_READY:    state_str = "Ready"; break;
            case TASK_BLOCKED:  state_str = "Blocked"; break;
            case TASK_SLEEPING: state_str = "Sleeping"; break;
            case TASK_ZOMBIE:   state_str = "Zombie"; break;
            default:            state_str = "Unknown"; break;
        }

        /* 当前运行的进程用绿色显示 */
        if (p->state == TASK_RUNNING) {
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        }

        vga_printf("%-5d %-16s %-10s %-8d %-8d\n",
                   p->pid, p->name, state_str, p->priority, p->remaining_time);

        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        p = p->next;
    }

    vga_puts("\n");
    vga_printf("Total: %d processes\n", count);
}

/* ==========================================
 * 函数：cmd_uname
 * 功能：显示系统信息
 * ========================================== */
static void cmd_uname(void)
{
    vga_puts("MyMiniOS 0.3.0 i386 ");
    vga_puts("32-bit Protected Mode Kernel\n");
}

/* ==========================================
 * 函数：cmd_hostname
 * 功能：显示主机名
 * ========================================== */
static void cmd_hostname(void)
{
    vga_puts("mini-os\n");
}

/* ==========================================
 * 函数：cmd_whoami
 * 功能：显示当前用户和权限级别
 * ========================================== */
static void cmd_whoami(void)
{
    const char *username = user_get_name();
    user_level_t level = user_get_level();
    const char *level_name = user_level_name(level);

    vga_printf("User: %s\n", username);
    vga_printf("Permission level: %s (%d)\n", level_name, level);

    if (user_is_admin()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("  [Admin privileges]\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}

/* ==========================================
 * 函数：cmd_su
 * 功能：切换用户
 * ========================================== */
static void cmd_su(const char *args)
{
    char username[32];
    char password[32];
    const char *p = args;

    /* 跳过开头的空格 */
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    /* 解析用户名 */
    int i = 0;
    while (*p && *p != ' ' && *p != '\t' && i < 31) {
        username[i++] = *p++;
    }
    username[i] = '\0';

    /* 如果没有用户名，默认切换到 root */
    if (i == 0) {
        strncpy(username, "root", 31);
    }

    /* 解析密码 */
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    i = 0;
    while (*p && *p != ' ' && *p != '\t' && i < 31) {
        password[i++] = *p++;
    }
    password[i] = '\0';

    /* 如果没有密码，提示输入（简化版：直接提示需要密码） */
    if (i == 0) {
        vga_puts("Password: ");
        /* 注意：实际系统中应该关闭回显输入密码 */
        /* 这里简化处理，直接读取一行作为密码 */
        keyboard_gets(password, sizeof(password));
    }

    /* 执行切换 */
    int result = user_su(username, password);

    if (result == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_printf("Switched to user: %s\n", username);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else if (result == -1) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_printf("su: user '%s' does not exist\n", username);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else if (result == -2) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("su: Authentication failure\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}

/* ==========================================
 * 函数：cmd_calc
 * 功能：简单计算器
 * ========================================== */
static void cmd_calc(const char *args)
{
    int num1 = 0, num2 = 0;
    char op = 0;
    const char *p = args;

    /* 跳过开头的空格 */
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    /* 解析第一个数字 */
    int sign1 = 1;
    if (*p == '-') {
        sign1 = -1;
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        num1 = num1 * 10 + (*p - '0');
        p++;
    }
    num1 *= sign1;

    /* 跳过空格 */
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    /* 解析运算符 */
    if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
        op = *p;
        p++;
    } else {
        vga_puts("Usage: calc <num1> <op> <num2>\n");
        vga_puts("Operators: +, -, *, /\n");
        return;
    }

    /* 跳过空格 */
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    /* 解析第二个数字 */
    int sign2 = 1;
    if (*p == '-') {
        sign2 = -1;
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        num2 = num2 * 10 + (*p - '0');
        p++;
    }
    num2 *= sign2;

    /* 计算结果 */
    int result;
    switch (op) {
        case '+':
            result = num1 + num2;
            break;
        case '-':
            result = num1 - num2;
            break;
        case '*':
            result = num1 * num2;
            break;
        case '/':
            if (num2 == 0) {
                vga_puts("Error: Division by zero\n");
                return;
            }
            result = num1 / num2;
            break;
        default:
            vga_puts("Unknown operator\n");
            return;
    }

    vga_printf("%d %c %d = %d\n", num1, op, num2, result);
}

/* ==========================================
 * 函数：cmd_dmesg
 * 功能：显示内核启动日志
 * ========================================== */
static void cmd_dmesg(void)
{
    char log_buf[2048];
    int len = klog_read(log_buf, sizeof(log_buf) - 1);
    log_buf[len] = '\0';

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("Kernel Boot Log (dmesg)\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("========================\n\n");

    if (len == 0) {
        vga_puts("(no kernel messages yet)\n");
    } else {
        vga_puts(log_buf);
    }
}

/* ==========================================
 * 函数：cmd_reboot
 * 功能：重启系统
 * ========================================== */
static void cmd_reboot(void)
{
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_puts("Rebooting system...\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* 等待一下让用户看到消息 */
    for (volatile int i = 0; i < 1000000; i++) {
        asm volatile ("nop");
    }

    /* 执行重启 */
    system_reboot();
}

/* ==========================================
 * 函数：cmd_ls
 * 功能：列出目录内容
 * ========================================== */
static void cmd_ls(const char *args)
{
    const char *path = args;
    
    /* 如果没有参数，列出当前目录 */
    if (*path == '\0') {
        path = ".";
    }
    
    vfs_dirent_t dirents[64];
    int count = vfs_readdir(path, dirents, 64);
    
    if (count < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_printf("ls: cannot access '%s': No such directory\n", path);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    for (int i = 0; i < count; i++) {
        if (dirents[i].type == VFS_DIR) {
            /* 目录用蓝色显示 */
            vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
        } else {
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }
        vga_printf("%s  ", dirents[i].name);
    }
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_putc('\n');
}

/* ==========================================
 * 函数：cmd_cat
 * 功能：显示文件内容
 * ========================================== */
static void cmd_cat(const char *args)
{
    if (*args == '\0') {
        vga_puts("Usage: cat <filename>\n");
        return;
    }
    
    vfs_file_t *file = vfs_open(args, VFS_O_RDONLY);
    if (file == NULL) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_printf("cat: %s: No such file\n", args);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    char buf[512];
    int bytes_read;
    
    while ((bytes_read = vfs_read(file, buf, sizeof(buf) - 1)) > 0) {
        buf[bytes_read] = '\0';
        vga_puts(buf);
    }
    
    vfs_close(file);
}

/* ==========================================
 * 函数：cmd_mkdir
 * 功能：创建目录
 * ========================================== */
static void cmd_mkdir(const char *args)
{
    if (*args == '\0') {
        vga_puts("Usage: mkdir <directory>\n");
        return;
    }
    
    int result = vfs_mkdir(args);
    if (result < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_printf("mkdir: cannot create directory '%s'\n", args);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}

/* ==========================================
 * 函数：cmd_rm
 * 功能：删除文件
 * ========================================== */
static void cmd_rm(const char *args)
{
    if (*args == '\0') {
        vga_puts("Usage: rm <filename>\n");
        return;
    }
    
    int result = vfs_unlink(args);
    if (result < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_printf("rm: cannot remove '%s'\n", args);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}

/* ==========================================
 * 函数：cmd_cd
 * 功能：改变目录
 * ========================================== */
static void cmd_cd(const char *args)
{
    if (*args == '\0') {
        /* 没有参数，回到根目录 */
        vfs_chdir("/");
        return;
    }
    
    int result = vfs_chdir(args);
    if (result < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_printf("cd: %s: No such directory\n", args);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}

/* ==========================================
 * 函数：cmd_pwd
 * 功能：显示当前工作目录
 * ========================================== */
static void cmd_pwd(void)
{
    vga_puts(vfs_getcwd());
    vga_putc('\n');
}

/* ==========================================
 * 函数：cmd_touch
 * 功能：创建空文件
 * ========================================== */
static void cmd_touch(const char *args)
{
    if (*args == '\0') {
        vga_puts("Usage: touch <filename>\n");
        return;
    }
    
    int result = vfs_create(args);
    if (result < 0) {
        /* 如果文件已存在，不报错（和 Unix 行为一致） */
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_printf("touch: cannot create '%s'\n", args);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
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
    } else if (strcmp(cmd_name, "meminfo") == 0) {
        cmd_meminfo();
    } else if (strcmp(cmd_name, "ps") == 0) {
        cmd_ps();
    } else if (strcmp(cmd_name, "uname") == 0) {
        cmd_uname();
    } else if (strcmp(cmd_name, "hostname") == 0) {
        cmd_hostname();
    } else if (strcmp(cmd_name, "whoami") == 0) {
        cmd_whoami();
    } else if (strcmp(cmd_name, "su") == 0) {
        cmd_su(args);
    } else if (strcmp(cmd_name, "calc") == 0) {
        cmd_calc(args);
    } else if (strcmp(cmd_name, "dmesg") == 0) {
        /* dmesg 需要管理员权限 */
        if (!user_is_admin()) {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_puts("dmesg: Permission denied\n");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            vga_puts("  (requires admin privileges, use 'su root' to switch)\n");
            return;
        }
        cmd_dmesg();
    } else if (strcmp(cmd_name, "reboot") == 0) {
        /* reboot 需要管理员权限 */
        if (!user_is_admin()) {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_puts("reboot: Permission denied\n");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            vga_puts("  (requires admin privileges, use 'su root' to switch)\n");
            return;
        }
        cmd_reboot();
    } else if (strcmp(cmd_name, "ls") == 0) {
        cmd_ls(args);
    } else if (strcmp(cmd_name, "cat") == 0) {
        cmd_cat(args);
    } else if (strcmp(cmd_name, "mkdir") == 0) {
        cmd_mkdir(args);
    } else if (strcmp(cmd_name, "rm") == 0) {
        cmd_rm(args);
    } else if (strcmp(cmd_name, "cd") == 0) {
        cmd_cd(args);
    } else if (strcmp(cmd_name, "pwd") == 0) {
        cmd_pwd();
    } else if (strcmp(cmd_name, "touch") == 0) {
        cmd_touch(args);
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

    /* 初始化用户系统 */
    user_init();

    vga_puts("\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("Shell is ready! Type 'help' for commands.\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_printf("Logged in as: %s (type 'su root' to switch to admin)\n\n", user_get_name());

    while (1) {
        /* 显示提示符，根据用户权限显示不同符号 */
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_printf("%s@mini-os", user_get_name());
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        if (user_is_admin()) {
            vga_puts("# ");
        } else {
            vga_puts("$ ");
        }

        /* 读取用户输入 */
        keyboard_gets(cmd_buf, CMD_BUF_SIZE);

        /* 执行命令 */
        execute_command(cmd_buf);

        vga_puts("\n");
    }
}
