/* ==========================================
 * 内核日志实现 v0.4.0
 * 功能：
 *   1. 环形缓冲区存储内核日志
 *   2. printk 函数（简化版格式化）
 *   3. dmesg 命令支持
 * ========================================== */
#include "klog.h"
#include "string.h"
#include "vga.h"
#include "stdarg.h"

/* 环形缓冲区 */
static char klog_buffer[KLOG_BUF_SIZE];

/* 缓冲区写入位置 */
static int klog_write_pos = 0;

/* 缓冲区中数据长度 */
static int klog_data_len = 0;

/* 是否已初始化 */
static bool klog_initialized = false;

/* ==========================================
 * 日志级别前缀
 * ========================================== */
static const char *log_level_prefix[] = {
    "[EMERG] ",
    "[ALERT] ",
    "[CRIT]  ",
    "[ERR]   ",
    "[WARN]  ",
    "[NOTICE]",
    "[INFO]  ",
    "[DEBUG] "
};

/* ==========================================
 * 向缓冲区写入一个字符
 * ========================================== */
static void klog_putc(char c) {
    klog_buffer[klog_write_pos] = c;
    klog_write_pos = (klog_write_pos + 1) % KLOG_BUF_SIZE;
    
    if (klog_data_len < KLOG_BUF_SIZE) {
        klog_data_len++;
    }
}

/* ==========================================
 * 向缓冲区写入字符串
 * ========================================== */
static void klog_puts(const char *s) {
    while (*s) {
        klog_putc(*s++);
    }
}

/* ==========================================
 * 向缓冲区写入数字（十六进制）
 * ========================================== */
static void klog_put_hex(uint32_t num) {
    char hex_chars[] = "0123456789ABCDEF";
    char buf[9];
    int i;
    
    buf[8] = '\0';
    for (i = 7; i >= 0; i--) {
        buf[i] = hex_chars[num & 0xF];
        num >>= 4;
    }
    
    klog_puts("0x");
    klog_puts(buf);
}

/* ==========================================
 * 向缓冲区写入数字（十进制）
 * ========================================== */
static void klog_put_dec(int num) {
    char buf[12];
    int i = 0;
    bool negative = false;
    
    if (num < 0) {
        negative = true;
        num = -num;
    }
    
    if (num == 0) {
        klog_putc('0');
        return;
    }
    
    while (num > 0 && i < 11) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    if (negative) {
        klog_putc('-');
    }
    
    while (i > 0) {
        klog_putc(buf[--i]);
    }
}

/* ==========================================
 * 简化版格式化输出
 * 支持：%s, %d, %x, %c, %%
 * ========================================== */
static void klog_vprintf(const char *fmt, va_list args) {
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's': {
                    const char *s = va_arg(args, const char *);
                    if (s == NULL) {
                        s = "(null)";
                    }
                    klog_puts(s);
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    klog_put_dec(d);
                    break;
                }
                case 'x': {
                    uint32_t x = va_arg(args, uint32_t);
                    klog_put_hex(x);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    klog_putc(c);
                    break;
                }
                case '%': {
                    klog_putc('%');
                    break;
                }
                default: {
                    klog_putc('%');
                    klog_putc(*fmt);
                    break;
                }
            }
        } else {
            klog_putc(*fmt);
        }
        fmt++;
    }
}

/* ==========================================
 * 初始化内核日志
 * ========================================== */
void klog_init() {
    memset(klog_buffer, 0, KLOG_BUF_SIZE);
    klog_write_pos = 0;
    klog_data_len = 0;
    klog_initialized = true;
    
    klog_log("klog", "Kernel log system initialized");
}

/* ==========================================
 * 记录模块日志（简化版）
 * 格式：[module] message\n
 * ========================================== */
void klog_log(const char *module, const char *message) {
    if (!klog_initialized) {
        return;
    }

    klog_putc('[');
    klog_puts(module);
    klog_putc(']');
    klog_putc(' ');
    klog_puts(message);
    klog_putc('\n');
}

/* ==========================================
 * 记录模块日志（printf风格）
 * 格式：[module] formatted_message\n
 * ========================================== */
void klog_logf(const char *module, const char *fmt, ...) {
    if (!klog_initialized) {
        return;
    }

    va_list args;

    klog_putc('[');
    klog_puts(module);
    klog_putc(']');
    klog_putc(' ');

    va_start(args, fmt);
    klog_vprintf(fmt, args);
    va_end(args);

    klog_putc('\n');
}

/* ==========================================
 * 打印内核日志（带级别）
 * 同时输出到VGA和日志缓冲区
 * ========================================== */
void printk(int level, const char *fmt, ...) {
    va_list args;
    
    /* 限制日志级别范围 */
    if (level < 0) level = 0;
    if (level > 7) level = 7;
    
    /* 写入日志级别前缀 */
    if (klog_initialized) {
        klog_puts(log_level_prefix[level]);
    }
    
    /* 写入日志内容 */
    va_start(args, fmt);
    if (klog_initialized) {
        klog_vprintf(fmt, args);
    }
    va_end(args);
    
    /* 同时输出到VGA（如果VGA已初始化） */
    /* 注意：这里简化处理，直接用vga_printf输出 */
    /* 由于vga_printf的格式化功能可能不同，这里先只输出到缓冲区 */
    /* 后续可以改进为同时输出 */
}

/* ==========================================
 * 打印内核日志（默认INFO级别）
 * ========================================== */
void printk_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printk(KLOG_INFO, fmt, args);
    va_end(args);
}

/* ==========================================
 * 读取内核日志（dmesg）
 * 返回：读取的字节数
 * ========================================== */
int klog_read(char *buf, int len) {
    if (buf == NULL || len <= 0) {
        return 0;
    }
    
    int read_len = klog_data_len;
    if (read_len > len) {
        read_len = len;
    }
    
    /* 从环形缓冲区读取数据 */
    int start = (klog_write_pos - klog_data_len + KLOG_BUF_SIZE) % KLOG_BUF_SIZE;
    
    for (int i = 0; i < read_len; i++) {
        buf[i] = klog_buffer[(start + i) % KLOG_BUF_SIZE];
    }
    
    return read_len;
}

/* ==========================================
 * 清空内核日志
 * ========================================== */
void klog_clear() {
    klog_write_pos = 0;
    klog_data_len = 0;
    memset(klog_buffer, 0, KLOG_BUF_SIZE);
}

/* ==========================================
 * 获取日志缓冲区（直接访问）
 * 注意：这是环形缓冲区，数据可能不是连续的
 * ========================================== */
const char *klog_get_buffer() {
    return klog_buffer;
}

/* ==========================================
 * 获取日志长度
 * ========================================== */
int klog_get_length() {
    return klog_data_len;
}
