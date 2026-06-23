/* ==========================================
 * 串口驱动头文件 - serial.h
 * 功能：
 *   1. COM1 串口驱动
 *   2. 支持发送和接收字符
 *   3. 波特率设置
 * ========================================== */
#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

/* 串口端口地址 */
#define SERIAL_COM1_PORT    0x3F8
#define SERIAL_COM2_PORT    0x2F8
#define SERIAL_COM3_PORT    0x3E8
#define SERIAL_COM4_PORT    0x2E8

/* 串口寄存器偏移 */
#define SERIAL_REG_DATA     0   /* 数据寄存器（读/写） */
#define SERIAL_REG_IER      1   /* 中断使能寄存器 */
#define SERIAL_REG_IIR      2   /* 中断识别寄存器（读） */
#define SERIAL_REG_FCR      2   /* FIFO控制寄存器（写） */
#define SERIAL_REG_LCR      3   /* 线路控制寄存器 */
#define SERIAL_REG_MCR      4   /* 调制解调器控制寄存器 */
#define SERIAL_REG_LSR      5   /* 线路状态寄存器 */
#define SERIAL_REG_MSR      6   /* 调制解调器状态寄存器 */
#define SERIAL_REG_SCRATCH  7   /* 暂存寄存器 */

/* 线路状态寄存器位 */
#define SERIAL_LSR_DR       0x01  /* 数据就绪 */
#define SERIAL_LSR_OE       0x02  /* 溢出错误 */
#define SERIAL_LSR_PE       0x04  /* 奇偶校验错误 */
#define SERIAL_LSR_FE       0x08  /* 帧错误 */
#define SERIAL_LSR_BI       0x10  /* 中断检测 */
#define SERIAL_LSR_THRE     0x20  /* 发送保持寄存器为空 */
#define SERIAL_LSR_TEMT     0x40  /* 发送器为空 */
#define SERIAL_LSR_EF       0x80  /* 错误字符FIFO */

/* FIFO控制寄存器位 */
#define SERIAL_FCR_ENABLE   0x01  /* 使能FIFO */
#define SERIAL_FCR_CLEAR_RX 0x02  /* 清除接收FIFO */
#define SERIAL_FCR_CLEAR_TX 0x04  /* 清除发送FIFO */
#define SERIAL_FCR_DMA      0x08  /* DMA模式选择 */
#define SERIAL_FCR_14BYTE   0xC0  /* 14字节触发阈值 */

/* 线路控制寄存器位 */
#define SERIAL_LCR_8BIT     0x03  /* 8位数据 */
#define SERIAL_LCR_1STOP    0x00  /* 1位停止位 */
#define SERIAL_LCR_NOPARITY 0x00  /* 无校验 */
#define SERIAL_LCR_DLAB     0x80  /* 除数锁存访问位 */

/* 常用波特率除数值（115200 / 波特率） */
#define SERIAL_BAUD_115200  1
#define SERIAL_BAUD_57600   2
#define SERIAL_BAUD_38400   3
#define SERIAL_BAUD_19200   6
#define SERIAL_BAUD_9600    12
#define SERIAL_BAUD_4800    24
#define SERIAL_BAUD_2400    48
#define SERIAL_BAUD_1200    96

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化串口 */
int serial_init(uint16_t port, uint16_t baud_divisor);

/* 初始化COM1（默认115200波特率） */
void serial_init_com1(void);

/* 检查发送是否就绪 */
bool serial_is_transmit_empty(uint16_t port);

/* 发送一个字符 */
void serial_write_char(uint16_t port, char c);

/* 发送字符串 */
void serial_write_string(uint16_t port, const char *str);

/* 检查是否有数据可读 */
bool serial_data_available(uint16_t port);

/* 读取一个字符（阻塞） */
char serial_read_char(uint16_t port);

/* 读取一个字符（非阻塞） */
int serial_read_char_nonblock(uint16_t port, char *c);

/* 写入数据 */
void serial_write(uint16_t port, const char *buf, uint32_t len);

/* 读取数据 */
uint32_t serial_read(uint16_t port, char *buf, uint32_t len);

/* 串口printf（调试用） */
void serial_printf(const char *fmt, ...);

#endif /* SERIAL_H */
