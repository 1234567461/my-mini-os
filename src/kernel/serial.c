/* ==========================================
 * 串口驱动实现 - serial.c
 * 功能：
 *   1. COM1 串口驱动
 *   2. 支持发送和接收字符
 *   3. 波特率设置
 * ========================================== */
#include "serial.h"
#include "types.h"
#include "string.h"
#include "vga.h"
#include "stdarg.h"

/* 串口是否已初始化 */
static bool com1_initialized = false;

/* ==========================================
 * 函数：serial_init
 * 功能：初始化串口
 * 参数：
 *   port - 串口端口地址
 *   baud_divisor - 波特率除数
 * 返回：0=成功，-1=失败
 * ========================================== */
int serial_init(uint16_t port, uint16_t baud_divisor)
{
    /* 禁用所有中断 */
    outb(port + SERIAL_REG_IER, 0x00);

    /* 设置除数锁存（DLAB位） */
    outb(port + SERIAL_REG_LCR, SERIAL_LCR_DLAB);

    /* 设置波特率除数（低字节和高字节） */
    outb(port + SERIAL_REG_DATA, (uint8_t)(baud_divisor & 0xFF));
    outb(port + SERIAL_REG_IER, (uint8_t)((baud_divisor >> 8) & 0xFF));

    /* 设置线路控制：8位数据，1位停止位，无校验 */
    outb(port + SERIAL_REG_LCR, SERIAL_LCR_8BIT | SERIAL_LCR_1STOP | SERIAL_LCR_NOPARITY);

    /* 启用并清除FIFO，设置14字节触发阈值 */
    outb(port + SERIAL_REG_FCR, SERIAL_FCR_ENABLE | SERIAL_FCR_CLEAR_RX | 
         SERIAL_FCR_CLEAR_TX | SERIAL_FCR_14BYTE);

    /* 设置调制解调器控制：DTR + RTS + OUT2 */
    outb(port + SERIAL_REG_MCR, 0x0B);

    /* 回环测试 */
    outb(port + SERIAL_REG_MCR, 0x1E);  /* 设置回环模式 */
    outb(port + SERIAL_REG_DATA, 0xAE);  /* 发送测试字节 */
    
    /* 检查是否接收到相同的字节 */
    if (inb(port + SERIAL_REG_DATA) != 0xAE) {
        return -1;  /* 串口故障 */
    }

    /* 恢复正常模式 */
    outb(port + SERIAL_REG_MCR, 0x0F);

    return 0;
}

/* ==========================================
 * 函数：serial_init_com1
 * 功能：初始化COM1串口（默认115200波特率）
 * ========================================== */
void serial_init_com1(void)
{
    if (serial_init(SERIAL_COM1_PORT, SERIAL_BAUD_115200) == 0) {
        com1_initialized = true;
        vga_puts("  [✓] Serial Port COM1 (115200 baud)\n");
    } else {
        com1_initialized = false;
        vga_puts("  [ ] Serial Port COM1: not available\n");
    }
}

/* ==========================================
 * 函数：serial_is_transmit_empty
 * 功能：检查发送保持寄存器是否为空
 * ========================================== */
bool serial_is_transmit_empty(uint16_t port)
{
    return (inb(port + SERIAL_REG_LSR) & SERIAL_LSR_THRE) != 0;
}

/* ==========================================
 * 函数：serial_write_char
 * 功能：发送一个字符
 * ========================================== */
void serial_write_char(uint16_t port, char c)
{
    /* 等待发送就绪 */
    while (!serial_is_transmit_empty(port)) {
        /* 忙等待 */
    }

    /* 发送字符 */
    outb(port + SERIAL_REG_DATA, (uint8_t)c);
}

/* ==========================================
 * 函数：serial_write_string
 * 功能：发送字符串
 * ========================================== */
void serial_write_string(uint16_t port, const char *str)
{
    while (*str != '\0') {
        serial_write_char(port, *str++);
    }
}

/* ==========================================
 * 函数：serial_data_available
 * 功能：检查是否有数据可读
 * ========================================== */
bool serial_data_available(uint16_t port)
{
    return (inb(port + SERIAL_REG_LSR) & SERIAL_LSR_DR) != 0;
}

/* ==========================================
 * 函数：serial_read_char
 * 功能：读取一个字符（阻塞）
 * ========================================== */
char serial_read_char(uint16_t port)
{
    /* 等待数据就绪 */
    while (!serial_data_available(port)) {
        /* 忙等待 */
    }

    /* 读取字符 */
    return (char)inb(port + SERIAL_REG_DATA);
}

/* ==========================================
 * 函数：serial_read_char_nonblock
 * 功能：读取一个字符（非阻塞）
 * 返回：0=成功，-1=无数据
 * ========================================== */
int serial_read_char_nonblock(uint16_t port, char *c)
{
    if (!serial_data_available(port)) {
        return -1;
    }

    *c = (char)inb(port + SERIAL_REG_DATA);
    return 0;
}

/* ==========================================
 * 函数：serial_write
 * 功能：写入数据
 * ========================================== */
void serial_write(uint16_t port, const char *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        serial_write_char(port, buf[i]);
    }
}

/* ==========================================
 * 函数：serial_read
 * 功能：读取数据
 * 返回：实际读取的字节数
 * ========================================== */
uint32_t serial_read(uint16_t port, char *buf, uint32_t len)
{
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < len; i++) {
        if (serial_read_char_nonblock(port, &buf[i]) < 0) {
            break;
        }
        count++;
    }
    
    return count;
}

/* ==========================================
 * 函数：serial_printf
 * 功能：串口格式化输出（调试用）
 * ========================================== */
void serial_printf(const char *fmt, ...)
{
    if (!com1_initialized) {
        return;
    }

    char buf[256];
    va_list args;
    va_start(args, fmt);
    
    /* 简单的格式化输出（支持 %s, %d, %x, %c） */
    int i = 0;
    int j = 0;
    
    while (fmt[i] != '\0' && j < 255) {
        if (fmt[i] == '%') {
            i++;
            switch (fmt[i]) {
                case 's': {
                    const char *s = va_arg(args, const char *);
                    while (*s != '\0' && j < 255) {
                        buf[j++] = *s++;
                    }
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    char numbuf[16];
                    int n = 0;
                    bool negative = false;
                    
                    if (num < 0) {
                        negative = true;
                        num = -num;
                    }
                    
                    if (num == 0) {
                        numbuf[n++] = '0';
                    } else {
                        while (num > 0 && n < 16) {
                            numbuf[n++] = '0' + (num % 10);
                            num /= 10;
                        }
                    }
                    
                    if (negative && j < 255) {
                        buf[j++] = '-';
                    }
                    
                    while (n > 0 && j < 255) {
                        buf[j++] = numbuf[--n];
                    }
                    break;
                }
                case 'x': {
                    uint32_t num = va_arg(args, uint32_t);
                    char numbuf[16];
                    int n = 0;
                    const char *hex = "0123456789ABCDEF";
                    
                    if (num == 0) {
                        numbuf[n++] = '0';
                    } else {
                        while (num > 0 && n < 16) {
                            numbuf[n++] = hex[num & 0xF];
                            num >>= 4;
                        }
                    }
                    
                    while (n > 0 && j < 255) {
                        buf[j++] = numbuf[--n];
                    }
                    break;
                }
                case 'c': {
                    if (j < 255) {
                        buf[j++] = (char)va_arg(args, int);
                    }
                    break;
                }
                case '%': {
                    if (j < 255) {
                        buf[j++] = '%';
                    }
                    break;
                }
                default: {
                    if (j < 255) {
                        buf[j++] = fmt[i];
                    }
                    break;
                }
            }
        } else {
            buf[j++] = fmt[i];
        }
        i++;
    }
    
    buf[j] = '\0';
    va_end(args);
    
    serial_write_string(SERIAL_COM1_PORT, buf);
}
