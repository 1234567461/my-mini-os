/* ==========================================
 * 键盘驱动实现
 * ========================================== */

#include "keyboard.h"
#include "isr.h"
#include "pic.h"
#include "vga.h"
#include "types.h"

/* ==========================================
 * 扫描码到ASCII的转换表（普通状态）
 * 索引 = 扫描码
 * ========================================== */
static const char scancode_normal[] = {
    0,    0,    '1',  '2',  '3',  '4',  '5',  '6',  /* 00-07 */
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t', /* 08-0F */
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  /* 10-17 */
    'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',  /* 18-1F */
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  /* 20-27 */
    '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',  /* 28-2F */
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',  /* 30-37 */
    0,    ' ',  0,    0,    0,    0,    0,    0,    /* 38-3F */
    0,    0,    0,    0,    0,    0,    0,    '7',  /* 40-47 */
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',  /* 48-4F */
    '2',  '3',  '0',  '.',  0,    0,    0,    0,    /* 50-57 */
};

/* ==========================================
 * 扫描码到ASCII的转换表（Shift状态）
 * ========================================== */
static const char scancode_shift[] = {
    0,    0,    '!',  '@',  '#',  '$',  '%',  '^',  /* 00-07 */
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t', /* 08-0F */
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  /* 10-17 */
    'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',  /* 18-1F */
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  /* 20-27 */
    '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',  /* 28-2F */
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',  /* 30-37 */
    0,    ' ',  0,    0,    0,    0,    0,    0,    /* 38-3F */
    0,    0,    0,    0,    0,    0,    0,    '7',  /* 40-47 */
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',  /* 48-4F */
    '2',  '3',  '0',  '.',  0,    0,    0,    0,    /* 50-57 */
};

/* ==========================================
 * 键盘状态
 * ========================================== */
static keyboard_state_t kb_state = {
    .shift = false,
    .ctrl = false,
    .alt = false,
    .caps_lock = false,
    .num_lock = false,
    .scroll_lock = false
};

/* ==========================================
 * 环形缓冲区
 * ========================================== */
static char keyboard_buffer[KEYBOARD_BUF_SIZE];
static volatile uint32_t buf_head = 0;  /* 写入位置 */
static volatile uint32_t buf_tail = 0;  /* 读取位置 */

/* ==========================================
 * 向缓冲区写入一个字符
 * ========================================== */
static void keyboard_buf_put(char c) {
    uint32_t next = (buf_head + 1) % KEYBOARD_BUF_SIZE;
    if (next != buf_tail) {  /* 缓冲区未满 */
        keyboard_buffer[buf_head] = c;
        buf_head = next;
    }
}

/* ==========================================
 * 从缓冲区读取一个字符
 * ========================================== */
static char keyboard_buf_get() {
    if (buf_head == buf_tail) {
        return 0;  /* 缓冲区空 */
    }
    char c = keyboard_buffer[buf_tail];
    buf_tail = (buf_tail + 1) % KEYBOARD_BUF_SIZE;
    return c;
}

/* ==========================================
 * 处理特殊键（按下）
 * ========================================== */
static void handle_special_key_down(uint8_t scancode) {
    switch (scancode) {
        case 0x2A:  /* 左Shift */
        case 0x36:  /* 右Shift */
            kb_state.shift = true;
            break;
        case 0x1D:  /* Ctrl */
            kb_state.ctrl = true;
            break;
        case 0x38:  /* Alt */
            kb_state.alt = true;
            break;
        case 0x3A:  /* Caps Lock */
            kb_state.caps_lock = !kb_state.caps_lock;
            break;
        case 0x45:  /* Num Lock */
            kb_state.num_lock = !kb_state.num_lock;
            break;
        case 0x46:  /* Scroll Lock */
            kb_state.scroll_lock = !kb_state.scroll_lock;
            break;
    }
}

/* ==========================================
 * 处理特殊键（释放）
 * ========================================== */
static void handle_special_key_up(uint8_t scancode) {
    switch (scancode) {
        case 0x2A:  /* 左Shift */
        case 0x36:  /* 右Shift */
            kb_state.shift = false;
            break;
        case 0x1D:  /* Ctrl */
            kb_state.ctrl = false;
            break;
        case 0x38:  /* Alt */
            kb_state.alt = false;
            break;
    }
}

/* ==========================================
 * 键盘中断处理函数
 * ========================================== */
static void keyboard_handler(isr_regs_t *regs) {
    (void)regs;

    /* 读取扫描码 */
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    /* 判断是按下还是释放 */
    if (scancode & 0x80) {
        /* 释放（断码 = 通码 | 0x80） */
        handle_special_key_up(scancode & 0x7F);
    } else {
        /* 按下（通码） */
        handle_special_key_down(scancode);

        /* 转换为ASCII */
        char c = 0;

        if (scancode < sizeof(scancode_normal)) {
            if (kb_state.shift) {
                c = scancode_shift[scancode];
            } else {
                c = scancode_normal[scancode];
            }

            /* 处理Caps Lock（只对字母有效） */
            if (kb_state.caps_lock && c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            } else if (kb_state.caps_lock && c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';
            }
        }

        /* 如果是可打印字符或控制字符，放入缓冲区 */
        if (c != 0) {
            keyboard_buf_put(c);
        }
    }
}

/* ==========================================
 * 初始化键盘
 * ========================================== */
void keyboard_init() {
    /* 注册中断处理函数（IRQ1 = 中断号33） */
    isr_register_handler(33, keyboard_handler);

    /* 开启IRQ1（键盘） */
    pic_irq_unmask(1);
}

/* ==========================================
 * 检查缓冲区是否有数据
 * ========================================== */
bool keyboard_has_data() {
    return buf_head != buf_tail;
}

/* ==========================================
 * 从键盘读取一个字符（阻塞）
 * ========================================== */
char keyboard_getc() {
    while (!keyboard_has_data()) {
        asm volatile ("hlt");  /* 等待中断 */
    }
    return keyboard_buf_get();
}

/* ==========================================
 * 从键盘读取一行字符串
 * ========================================== */
void keyboard_gets(char *buf, size_t max_len) {
    size_t i = 0;
    char c;

    while (i < max_len - 1) {
        c = keyboard_getc();

        if (c == '\n' || c == '\r') {
            /* 回车，结束输入 */
            vga_putc('\n');
            break;
        } else if (c == '\b') {
            /* 退格 */
            if (i > 0) {
                i--;
                vga_putc('\b');
            }
        } else if (c >= ' ' && c < 127) {
            /* 可打印字符 */
            buf[i++] = c;
            vga_putc(c);
        }
    }

    buf[i] = '\0';
}

/* ==========================================
 * 获取键盘状态
 * ========================================== */
keyboard_state_t *keyboard_get_state() {
    return &kb_state;
}
