/* ==========================================
 * 内核主文件 - kernel.c
 * 功能：
 *   1. 内核主函数 kernel_main
 *   2. VGA文本模式打印函数
 *   3. 清屏、光标移动等基础功能
 * ========================================== */

/* VGA文本缓冲区地址：0xb8000
 * 每个字符占2字节：
 *   第1字节：ASCII字符
 *   第2字节：属性（颜色等）
 *     高4位：背景色
 *     低4位：前景色
 *
 * 颜色代码：
 *   0 = 黑    8 = 深灰
 *   1 = 蓝    9 = 浅蓝
 *   2 = 绿    A = 浅绿
 *   3 = 青    B = 浅青
 *   4 = 红    C = 浅红
 *   5 = 紫    D = 浅紫
 *   6 = 棕    E = 黄
 *   7 = 浅灰  F = 白
 */

#define VGA_BUFFER 0xb8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* 全局变量：当前光标位置 */
static int cursor_row = 0;
static int cursor_col = 0;

/* 全局变量：当前颜色 */
static unsigned char current_color = 0x0F;  // 黑底白字

/* ==========================================
 * 函数：vga_entry
 * 功能：构造一个VGA字符（字符+属性）
 * ========================================== */
static unsigned short vga_entry(unsigned char ch, unsigned char color)
{
    return (unsigned short)ch | (unsigned short)color << 8;
}

/* ==========================================
 * 函数：vga_color
 * 功能：构造颜色属性字节
 * ========================================== */
static unsigned char vga_color(unsigned char fg, unsigned char bg)
{
    return fg | bg << 4;
}

/* ==========================================
 * 函数：clear_screen
 * 功能：清屏（用空格填充整个屏幕）
 * ========================================== */
void clear_screen(void)
{
    unsigned short *buffer = (unsigned short *)VGA_BUFFER;
    unsigned short blank = vga_entry(' ', current_color);

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        buffer[i] = blank;
    }

    cursor_row = 0;
    cursor_col = 0;
}

/* ==========================================
 * 函数：set_color
 * 功能：设置当前文字颜色
 * ========================================== */
void set_color(unsigned char fg, unsigned char bg)
{
    current_color = vga_color(fg, bg);
}

/* ==========================================
 * 函数：move_cursor
 * 功能：移动光标到指定位置
 * ========================================== */
void move_cursor(int row, int col)
{
    cursor_row = row;
    cursor_col = col;
}

/* ==========================================
 * 函数：newline
 * 功能：换行
 * ========================================== */
static void newline(void)
{
    cursor_row++;
    cursor_col = 0;

    /* 如果超出屏幕底部，就滚动屏幕（简单处理，先不实现滚动） */
    if (cursor_row >= VGA_HEIGHT) {
        cursor_row = VGA_HEIGHT - 1;
        // TODO: 实现屏幕滚动
    }
}

/* ==========================================
 * 函数：putchar
 * 功能：在当前光标位置打印一个字符
 * ========================================== */
void putchar(char ch)
{
    unsigned short *buffer = (unsigned short *)VGA_BUFFER;

    /* 处理换行符 */
    if (ch == '\n') {
        newline();
        return;
    }

    /* 处理回车符 */
    if (ch == '\r') {
        cursor_col = 0;
        return;
    }

    /* 普通字符 */
    int index = cursor_row * VGA_WIDTH + cursor_col;
    buffer[index] = vga_entry(ch, current_color);

    cursor_col++;

    /* 如果到了行尾，自动换行 */
    if (cursor_col >= VGA_WIDTH) {
        newline();
    }
}

/* ==========================================
 * 函数：print_string
 * 功能：打印一个字符串
 * ========================================== */
void print_string(const char *str)
{
    for (int i = 0; str[i] != '\0'; i++) {
        putchar(str[i]);
    }
}

/* ==========================================
 * 函数：print_int
 * 功能：打印整数（十进制）
 * ========================================== */
void print_int(int num)
{
    if (num == 0) {
        putchar('0');
        return;
    }

    /* 处理负数 */
    if (num < 0) {
        putchar('-');
        num = -num;
    }

    /* 把数字转换为字符串（逆序） */
    char buf[32];
    int i = 0;

    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    /* 逆序打印 */
    for (int j = i - 1; j >= 0; j--) {
        putchar(buf[j]);
    }
}

/* ==========================================
 * 函数：print_hex
 * 功能：打印十六进制数
 * ========================================== */
void print_hex(unsigned int num)
{
    const char *hex_digits = "0123456789ABCDEF";

    print_string("0x");

    for (int i = 28; i >= 0; i -= 4) {
        unsigned char digit = (num >> i) & 0xF;
        putchar(hex_digits[digit]);
    }
}

/* ==========================================
 * 函数：kernel_main
 * 功能：内核主函数，C语言代码从这里开始执行
 * ========================================== */
void kernel_main(void)
{
    /* 初始化：清屏 */
    clear_screen();

    /* ====== 显示欢迎信息 ====== */
    set_color(0x0A, 0x00);  // 绿字黑底
    print_string("  __  __         _   __  __  _       ___   ____ \n");
    print_string(" |  \\/  |_   _ (_) |  \\/  |(_)     / _ \\ / ___|\n");
    print_string(" | |\\/| | | | | | | |\\/| || |    | | | |\\___ \\\n");
    print_string(" | |  | | |_| | | | |  | || |    | |_| | ___) |\n");
    print_string(" |_|  |_|\\__, ||_| |_|  |_||_|     \\___/ |____/ \n");
    print_string("         |___/                                  \n");
    print_string("\n");

    /* ====== 系统信息 ====== */
    set_color(0x0F, 0x00);  // 白字黑底
    print_string("Welcome to My Mini OS!\n\n");

    set_color(0x0B, 0x00);  // 浅青
    print_string("[Kernel] ");
    set_color(0x0F, 0x00);
    print_string("Kernel loaded successfully!\n");

    set_color(0x0B, 0x00);
    print_string("[Kernel] ");
    set_color(0x0F, 0x00);
    print_string("Running in 32-bit protected mode\n");

    set_color(0x0B, 0x00);
    print_string("[Kernel] ");
    set_color(0x0F, 0x00);
    print_string("VGA text mode: ");
    print_int(VGA_WIDTH);
    print_string("x");
    print_int(VGA_HEIGHT);
    print_string("\n");

    set_color(0x0B, 0x00);
    print_string("[Kernel] ");
    set_color(0x0F, 0x00);
    print_string("VGA buffer address: ");
    print_hex(VGA_BUFFER);
    print_string("\n");

    print_string("\n");

    /* ====== 颜色演示 ====== */
    set_color(0x0E, 0x00);  // 黄色
    print_string("--- Color Demo ---\n");

    set_color(0x01, 0x00); print_string("Blue ");
    set_color(0x02, 0x00); print_string("Green ");
    set_color(0x03, 0x00); print_string("Cyan ");
    set_color(0x04, 0x00); print_string("Red ");
    set_color(0x05, 0x00); print_string("Purple ");
    set_color(0x06, 0x00); print_string("Brown ");
    set_color(0x07, 0x00); print_string("Gray ");
    set_color(0x08, 0x00); print_string("DarkGray ");
    set_color(0x09, 0x00); print_string("LightBlue ");
    set_color(0x0A, 0x00); print_string("LightGreen ");
    set_color(0x0B, 0x00); print_string("LightCyan ");
    set_color(0x0C, 0x00); print_string("LightRed ");
    set_color(0x0D, 0x00); print_string("LightPurple ");
    set_color(0x0E, 0x00); print_string("Yellow ");
    set_color(0x0F, 0x00); print_string("White ");

    print_string("\n\n");

    /* ====== 下一步提示 ====== */
    set_color(0x0E, 0x00);
    print_string("--- Next Steps ---\n");

    set_color(0x0F, 0x00);
    print_string("✓ 16-bit real mode bootloader\n");
    print_string("✓ 32-bit protected mode\n");
    print_string("✓ C language kernel\n");
    print_string("✓ VGA text output\n");
    print_string("\n");
    print_string("Coming soon: Interrupts, Memory management, Keyboard driver...\n");

    /* 永远停在这里 */
    while (1) {
        __asm__ volatile ("hlt");  // 停机指令，省电
    }
}
