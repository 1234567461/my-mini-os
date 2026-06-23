/**
 * @file serial.c
 * @brief 16550 UART 串口驱动实现
 * @version v0.6.0
 */

#include <serial.h>
#include <types.h>
#include <vga.h>
#include <string.h>

// 调试串口（默认 COM1）
#define DEBUG_SERIAL_PORT SERIAL_COM1_PORT

/**
 * @brief 读取串口寄存器
 */
static inline uint8_t serial_read_reg(uint16_t port, uint8_t reg)
{
    return inb(port + reg);
}

/**
 * @brief 写入串口寄存器
 */
static inline void serial_write_reg(uint16_t port, uint8_t reg, uint8_t value)
{
    outb(port + reg, value);
}

/**
 * @brief 检测串口是否存在
 * 
 * 通过写入暂存寄存器然后读回来检测
 */
bool serial_detect(uint16_t port)
{
    // 写入暂存寄存器
    serial_write_reg(port, SERIAL_REG_SCR, 0xAA);
    if (serial_read_reg(port, SERIAL_REG_SCR) != 0xAA) {
        return false;
    }
    
    serial_write_reg(port, SERIAL_REG_SCR, 0x55);
    if (serial_read_reg(port, SERIAL_REG_SCR) != 0x55) {
        return false;
    }
    
    return true;
}

/**
 * @brief 检查发送是否就绪
 */
bool serial_is_transmit_ready(uint16_t port)
{
    return (serial_read_reg(port, SERIAL_REG_LSR) & SERIAL_LSR_THRE) != 0;
}

/**
 * @brief 检查是否有数据可读
 */
bool serial_is_data_available(uint16_t port)
{
    return (serial_read_reg(port, SERIAL_REG_LSR) & SERIAL_LSR_DR) != 0;
}

/**
 * @brief 初始化串口
 */
int serial_init(uint16_t port, uint16_t baud_divisor)
{
    // 检测串口是否存在
    if (!serial_detect(port)) {
        return -1;
    }
    
    // 禁用所有中断
    serial_write_reg(port, SERIAL_REG_IER, 0x00);
    
    // 启用 DLAB（设置波特率）
    serial_write_reg(port, SERIAL_REG_LCR, SERIAL_LCR_DLAB);
    
    // 设置波特率除数
    serial_write_reg(port, SERIAL_REG_DLL, baud_divisor & 0xFF);
    serial_write_reg(port, SERIAL_REG_DLM, (baud_divisor >> 8) & 0xFF);
    
    // 设置数据位：8 数据位，1 停止位，无奇偶校验
    serial_write_reg(port, SERIAL_REG_LCR, SERIAL_LCR_8BIT);
    
    // 启用并清除 FIFO，设置 14 字节触发
    serial_write_reg(port, SERIAL_REG_FCR, 
        SERIAL_FCR_ENABLE | 
        SERIAL_FCR_CLEAR_RX | 
        SERIAL_FCR_CLEAR_TX | 
        SERIAL_FCR_TRIG_14);
    
    // 设置 DTR、RTS、OUT2（启用中断）
    serial_write_reg(port, SERIAL_REG_MCR, 
        SERIAL_MCR_DTR | 
        SERIAL_MCR_RTS | 
        SERIAL_MCR_OUT2);
    
    // 测试串口（回环模式）
    serial_write_reg(port, SERIAL_REG_MCR, 
        SERIAL_MCR_LOOP | 
        SERIAL_MCR_OUT1 | 
        SERIAL_MCR_OUT2);
    
    // 发送测试字节
    serial_write_reg(port, SERIAL_REG_THR, 0xAE);
    
    // 检查是否能收到
    if (serial_read_reg(port, SERIAL_REG_RBR) != 0xAE) {
        return -1;
    }
    
    // 退出回环模式，恢复正常操作
    serial_write_reg(port, SERIAL_REG_MCR, 
        SERIAL_MCR_DTR | 
        SERIAL_MCR_RTS | 
        SERIAL_MCR_OUT2);
    
    return 0;
}

/**
 * @brief 发送一个字节
 */
void serial_send_byte(uint16_t port, uint8_t data)
{
    // 等待发送就绪
    while (!serial_is_transmit_ready(port)) {
        // 忙等待
    }
    
    serial_write_reg(port, SERIAL_REG_THR, data);
}

/**
 * @brief 接收一个字节（阻塞）
 */
uint8_t serial_receive_byte(uint16_t port)
{
    // 等待数据就绪
    while (!serial_is_data_available(port)) {
        // 忙等待
    }
    
    return serial_read_reg(port, SERIAL_REG_RBR);
}

/**
 * @brief 尝试接收一个字节（非阻塞）
 */
bool serial_try_receive(uint16_t port, uint8_t *data)
{
    if (!data) {
        return false;
    }
    
    if (!serial_is_data_available(port)) {
        return false;
    }
    
    *data = serial_read_reg(port, SERIAL_REG_RBR);
    return true;
}

/**
 * @brief 发送字符串
 */
void serial_send_string(uint16_t port, const char *str)
{
    if (!str) {
        return;
    }
    
    while (*str) {
        if (*str == '\n') {
            serial_send_byte(port, '\r');
        }
        serial_send_byte(port, (uint8_t)*str);
        str++;
    }
}

/**
 * @brief 初始化 COM1 作为调试串口
 */
int serial_debug_init(void)
{
    int result = serial_init(DEBUG_SERIAL_PORT, SERIAL_BAUD_115200);
    
    if (result == 0) {
        vga_printf("[SERIAL] Debug serial initialized on COM1 (115200 baud)\n");
        serial_debug_print("\n=== My Mini OS Debug Serial ===\n");
    } else {
        vga_printf("[SERIAL] COM1 not found\n");
    }
    
    return result;
}

/**
 * @brief 调试输出字符
 */
void serial_debug_putchar(char c)
{
    serial_send_byte(DEBUG_SERIAL_PORT, c);
}

/**
 * @brief 调试输出字符串
 */
void serial_debug_print(const char *str)
{
    serial_send_string(DEBUG_SERIAL_PORT, str);
}
