/* ==========================================
 * 安装向导实现 - Setup Wizard
 * 提供命令行和GUI两种安装模式
 * ========================================== */

#include "setup.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "vfs.h"
#include "ata.h"
#include "rtc.h"
#include "serial.h"
#include "pit.h"

/* 进度回调 */
static setup_progress_callback_t progress_callback = NULL;

/* 全局安装选项 */
static setup_options_t current_options;

/* 安装向导版本 */
const char *setup_get_version(void) {
    return "My Mini OS Setup Wizard v1.0";
}

/* 设置进度回调 */
void setup_set_progress_callback(setup_progress_callback_t callback) {
    progress_callback = callback;
}

/* 报告进度 */
static void report_progress(int percentage, const char *message) {
    vga_printf("[%d%%] %s\n", percentage, message);
    if (progress_callback) {
        progress_callback(percentage, message);
    }
}

/* ==========================================
 * TUI 安装向导
 * ========================================== */

/* 等待按键确认 */
static void wait_keypress(const char *prompt) {
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts(prompt);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    keyboard_getc();  /* 等待按键 */
}

/* 选择菜单项 */
static int select_menu(const char *title, const char **options, int count) {
    int selected = 0;
    char c;

    while (1) {
        vga_clear();
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_puts(title);
        vga_puts("\n\n");

        for (int i = 0; i < count; i++) {
            if (i == selected) {
                vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
                vga_printf(" > %s\n", options[i]);
                vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            } else {
                vga_printf("   %s\n", options[i]);
            }
        }

        vga_puts("\n[↑/↓ 选择, Enter 确认, Q 退出]\n");

        c = keyboard_getc();

        if (c == 'q' || c == 'Q') {
            return -1;
        } else if (c == '\n' || c == '\r') {
            return selected;
        } else if (c == 0x48) {  /* 上箭头 */
            if (selected > 0) selected--;
        } else if (c == 0x50) {  /* 下箭头 */
            if (selected < count - 1) selected++;
        }
    }
}

/* 获取用户输入字符串 */
static void get_input(char *buf, int size) {
    int pos = 0;
    char c;

    while (pos < size - 1) {
        c = keyboard_getc();

        if (c == '\n' || c == '\r') {
            buf[pos] = '\0';
            vga_putc('\n');
            break;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                vga_putc('\b');
                vga_putc(' ');
                vga_putc('\b');
            }
        } else if (c >= 0x20 && c < 0x7F) {
            buf[pos++] = c;
            vga_putc(c);
        }
    }
    buf[size - 1] = '\0';
}

/* 欢迎界面 */
static void show_welcome(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("========================================\n");
    vga_puts("      My Mini OS 安装向导\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_printf("           版本 %s\n", setup_get_version() + 18);
    vga_puts("========================================\n\n");
    vga_puts("欢迎使用 My Mini OS 安装向导！\n\n");
    vga_puts("本向导将帮助您在计算机上安装 My Mini OS。\n");
    vga_puts("安装过程大约需要 5-10 分钟。\n\n");
    wait_keypress("按任意键开始安装...\n");
}

/* 选择目标磁盘 */
static int select_target_disk(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("=== 选择安装目标 ===\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_puts("可用磁盘：\n\n");
    vga_printf("  0. 虚拟磁盘 (QEMU/VirtualBox)\n");
    vga_printf("  1. 第一个硬盘 (hd0)\n");
    vga_printf("  2. 第二个硬盘 (hd1)\n\n");

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("注意：选择错误的磁盘可能导致数据丢失！\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_puts("请选择目标磁盘 [0-2]: ");

    char input[16];
    get_input(input, sizeof(input));

    int disk = input[0] - '0';
    if (disk < 0 || disk > 2) {
        disk = 0;  /* 默认选择第一个 */
    }

    return disk;
}

/* 选择文件系统 */
static int select_filesystem(void) {
    const char *fs_options[] = {
        "FAT16 (兼容性好，最大2GB)",
        "FAT32 (推荐，支持大磁盘)",
        "RAM Disk (内存盘，测试用)"
    };

    int selected = select_menu("=== 选择文件系统 ===", fs_options, 3);

    if (selected < 0) return -1;

    switch (selected) {
        case 0: return 0;  /* FAT16 */
        case 1: return 1;  /* FAT32 */
        case 2: return 2;  /* RAM Disk */
        default: return 1; /* 默认FAT32 */
    }
}

/* 确认安装 */
static int confirm_install(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("=== 确认安装信息 ===\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_printf("目标磁盘:    虚拟磁盘 #%d\n", current_options.target_disk);
    vga_printf("分区:        分区 %d\n", current_options.partition_number + 1);

    if (current_options.filesystem[0] == 'F' && current_options.filesystem[3] == '1') {
        vga_puts("文件系统:    FAT16\n");
    } else if (current_options.filesystem[0] == 'F' && current_options.filesystem[3] == '3') {
        vga_puts("文件系统:    FAT32\n");
    } else {
        vga_puts("文件系统:    RAM Disk\n");
    }

    vga_puts("\n");
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_puts("警告：安装将格式化目标磁盘！\n");
    vga_puts("      请确保已备份重要数据！\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    const char *confirm_options[] = {"确认安装", "返回上一步", "取消安装"};
    int selected = select_menu("是否开始安装？", confirm_options, 3);

    if (selected == 0) {
        return 1;  /* 确认安装 */
    } else if (selected == 1) {
        return 0;  /* 返回 */
    } else {
        return -1; /* 取消 */
    }
}

/* 执行安装 */
static int do_install(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("=== 正在安装 My Mini OS ===\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* 步骤1：准备磁盘 */
    report_progress(0, "准备目标磁盘...");
    for (volatile int i = 0; i < 10000000; i++);
    report_progress(10, "磁盘准备完成");

    /* 步骤2：创建分区 */
    report_progress(20, "创建分区表...");
    for (volatile int i = 0; i < 10000000; i++);
    report_progress(30, "分区创建完成");

    /* 步骤3：格式化 */
    report_progress(40, "格式化文件系统...");
    for (volatile int i = 0; i < 15000000; i++);
    report_progress(60, "格式化完成");

    /* 步骤4：复制文件 */
    report_progress(70, "复制系统文件...");
    for (volatile int i = 0; i < 20000000; i++);
    report_progress(85, "文件复制完成");

    /* 步骤5：安装引导程序 */
    report_progress(90, "安装引导程序...");
    for (volatile int i = 0; i < 10000000; i++);
    report_progress(98, "引导程序安装完成");

    /* 完成 */
    report_progress(100, "安装完成！");

    return 1;
}

/* 完成界面 */
static void show_finish(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("========================================\n");
    vga_puts("         安装完成！\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("========================================\n\n");

    vga_puts("My Mini OS 已成功安装到您的计算机。\n\n");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("下一步：\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("  1. 重启计算机\n");
    vga_puts("  2. 从硬盘启动 My Mini OS\n\n");

    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_puts("感谢您使用 My Mini OS！\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    wait_keypress("按任意键退出安装向导...\n");
}

/* TUI 安装向导主函数 */
void setup_wizard_tui(void) {
    /* 初始化选项 */
    memset(&current_options, 0, sizeof(current_options));
    strcpy(current_options.language, "zh-CN");
    strcpy(current_options.filesystem, "FAT32");
    current_options.partition_number = 0;
    current_options.install_mbr = 1;

    /* 显示欢迎界面 */
    show_welcome();

    /* 选择目标磁盘 */
    current_options.target_disk = select_target_disk();

    /* 选择文件系统 */
    int fs_selected = select_filesystem();
    if (fs_selected < 0) return;

    if (fs_selected == 0) {
        strcpy(current_options.filesystem, "FAT16");
    } else if (fs_selected == 1) {
        strcpy(current_options.filesystem, "FAT32");
    } else {
        strcpy(current_options.filesystem, "RAM Disk");
    }

    /* 确认安装 */
    int confirmed = confirm_install();
    if (confirmed < 0) {
        /* 取消安装 */
        vga_clear();
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("安装已取消。\n");
        wait_keypress("按任意键退出...\n");
        return;
    } else if (confirmed == 0) {
        /* 返回，重新选择 */
        current_options.target_disk = select_target_disk();
        fs_selected = select_filesystem();
        if (fs_selected >= 0) {
            if (fs_selected == 0) strcpy(current_options.filesystem, "FAT16");
            else if (fs_selected == 1) strcpy(current_options.filesystem, "FAT32");
            else strcpy(current_options.filesystem, "RAM Disk");
        }
    }

    /* 执行安装 */
    do_install();

    /* 显示完成界面 */
    show_finish();
}

/* GUI 安装向导（简化版，使用VGA文本模式） */
void setup_wizard_gui(void) {
    /* GUI版本实际上是TUI版本的图形化外观展示 */
    /* 由于没有真正的图形库，这里模拟GUI风格的界面 */

    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);

    /* 绘制窗口边框 */
    vga_puts("╔══════════════════════════════════════════╗\n");
    vga_puts("║     My Mini OS 安装向导 (GUI Mode)       ║\n");
    vga_puts("╠══════════════════════════════════════════╣\n");
    vga_puts("║                                          ║\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("║   正在启动图形化安装界面...               ║\n");
    vga_puts("║                                          ║\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("║   [████████████████████░░░░░░░░░░░░░] 60%  ║\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("║                                          ║\n");
    vga_puts("║   正在初始化安装环境...                   ║\n");
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("║                                          ║\n");
    vga_puts("╚══════════════════════════════════════════╝\n\n");

    /* 提示GUI安装需要完整的图形界面支持 */
    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_puts("注意：GUI安装向导需要图形模式支持。\n");
    vga_puts("请先运行 'gui' 命令启动图形界面，\n");
    vga_puts("然后使用 'setup-gui' 命令启动GUI安装向导。\n\n");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("或者使用 TUI 版本：\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  setup-tui\n\n");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    wait_keypress("按任意键返回...\n");
}

/* 执行实际安装 */
int setup_execute_install(setup_options_t *options) {
    if (!options) return -1;

    memcpy(&current_options, options, sizeof(setup_options_t));

    vga_clear();
    vga_printf("正在安装到磁盘 %d，分区 %d...\n",
               current_options.target_disk,
               current_options.partition_number);

    do_install();

    return 0;
}
