/**
 * @file serial.h
 * @brief 16550 UART 串口驱动
 * @version v0.6.0
 *
 * 支持 COM1-COM4 串口，用于调试输出和串口通信
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <types.h>

// 串口端口地址
#define SERIAL_COM1_PORT    0x3F8
#define SERIAL_COM2_PORT    0x2F8
#define SERIAL_COM3_PORT    0x3E8
#define SERIAL_COM4_PORT    0x2E8

// 寄存器偏移
#define SERIAL_REG_RBR      0   // 接收缓冲寄存器（读）
#define SERIAL_REG_THR      0   // 发送保持寄存器（写）
#define SERIAL_REG_DLL      0   // 除数锁存器低字节
#define SERIAL_REG_IER      1   // 中断使能寄存器
#define SERIAL_REG_DLM      1   // 除数锁存器高字节
#define SERIAL_REG_IIR      2   // 中断识别寄存器（读）
#define SERIAL_REG_FCR      2   // FIFO 控制寄存器（写）
#define SERIAL_REG_LCR      3   // 线路控制寄存器
#define SERIAL_REG_MCR      4   // 调制解调器控制寄存器
#define SERIAL_REG_LSR      5   // 线路状态寄存器
#define SERIAL_REG_MSR      6   // 调制解调器状态寄存器
#define SERIAL_REG_SCR      7   // 暂存寄存器

// LCR（线路控制寄存器）位
#define SERIAL_LCR_DLAB     0x80    // 除数锁存器访问位
#define SERIAL_LCR_8BIT     0x03    // 8 数据位
#define SERIAL_LCR_7BIT     0x02    // 7 数据位
#define SERIAL_LCR_6BIT     0x01    // 6 数据位
#define SERIAL_LCR_5BIT     0x00    // 5 数据位
#define SERIAL_LCR_STOP     0x04    // 2 停止位（0=1位，1=2位）
#define SERIAL_LCR_PARITY   0x38    // 奇偶校验位掩码

// LSR（线路状态寄存器）位
#define SERIAL_LSR_DR       0x01    // 数据就绪
#define SERIAL_LSR_OE       0x02    // 溢出错误
#define SERIAL_LSR_PE       0x04    // 奇偶校验错误
#define SERIAL_LSR_FE       0x08    // 帧错误
#define SERIAL_LSR_BI       0x10    // 中断检测
#define SERIAL_LSR_THRE     0x20    // 发送保持寄存器为空
#define SERIAL_LSR_TEMT     0x40    // 发送器为空
#define SERIAL_LSR_FIFOERR  0x80    // FIFO 错误

// FCR（FIFO 控制寄存器）位
#define SERIAL_FCR_ENABLE   0x01    // 启用 FIFO
#define SERIAL_FCR_CLEAR_RX 0x02    // 清除接收 FIFO
#define SERIAL_FCR_CLEAR_TX 0x04    // 清除发送 FIFO
#define SERIAL_FCR_DMA      0x08    // DMA 模式选择
#define SERIAL_FCR_TRIG_1   0x00    // 1 字节触发
#define SERIAL_FCR_TRIG_4   0x40    // 4 字节触发
#define SERIAL_FCR_TRIG_8   0x80    // 8 字节触发
#define SERIAL_FCR_TRIG_14  0xC0    // 14 字节触发

// IER（中断使能寄存器）位
#define SERIAL_IER_RX       0x01    // 接收数据可用中断
#define SERIAL_IER_TX       0x02    // 发送保持寄存器空中断
#define SERIAL_IER_LS       0x04    // 线路状态变化中断
#define SERIAL_IER_MS       0x08    // 调制解调器状态变化中断

// MCR（调制解调器控制寄存器）位
#define SERIAL_MCR_DTR      0x01    // 数据终端就绪
#define SERIAL_MCR_RTS      0x02    // 请求发送
#define SERIAL_MCR_OUT1     0x04    // 输出 1
#define SERIAL_MCR_OUT2     0x08    // 输出 2（用于中断）
#define SERIAL_MCR_LOOP     0x10    // 回环模式

// 常用波特率（除数 = 115200 / 波特率）
#define SERIAL_BAUD_115200  1
#define SERIAL_BAUD_57600   2
#define SERIAL_BAUD_38400   3
#define SERIAL_BAUD_19200   6
#define SERIAL_BAUD_9600    12
#define SERIAL_BAUD_4800    24
#define SERIAL_BAUD_2400    48
#define SERIAL_BAUD_1200    96

/**
 * @brief 初始化串口
 * @param port 串口端口地址（如 SERIAL_COM1_PORT）
 * @param baud_divisor 波特率除数
 * @return 成功返回 0，失败返回 -1
 */
int serial_init(uint16_t port, uint16_t baud_divisor);

/**
 * @brief 检测串口是否存在
 * @param port 串口端口地址
 * @return 存在返回 true，不存在返回 false
 */
bool serial_detect(uint16_t port);

/**
 * @brief 发送一个字节
 * @param port 串口端口地址
 * @param data 要发送的字节
 */
void serial_send_byte(uint16_t port, uint8_t data);

/**
 * @brief 接收一个字节（阻塞）
 * @param port 串口端口地址
 * @return 接收到的字节
 */
uint8_t serial_receive_byte(uint16_t port);

/**
 * @brief 尝试接收一个字节（非阻塞）
 * @param port 串口端口地址
 * @param data 输出接收到的字节
 * @return 成功返回 true，无数据返回 false
 */
bool serial_try_receive(uint16_t port, uint8_t *data);

/**
 * @brief 发送字符串
 * @param port 串口端口地址
 * @param str 要发送的字符串
 */
void serial_send_string(uint16_t port, const char *str);

/**
 * @brief 检查发送是否就绪
 * @param port 串口端口地址
 * @return 就绪返回 true
 */
bool serial_is_transmit_ready(uint16_t port);

/**
 * @brief 检查是否有数据可读
 * @param port 串口端口地址
 * @return 有数据返回 true
 */
bool serial_is_data_available(uint16_t port);

/**
 * @brief 初始化 COM1 作为调试串口
 * @return 成功返回 0
 */
int serial_debug_init(void);

/**
 * @brief 调试输出字符
 * @param c 字符
 */
void serial_debug_putchar(char c);

/**
 * @brief 调试输出字符串
 * @param str 字符串
 */
void serial_debug_print(const char *str);

#endif // SERIAL_H
