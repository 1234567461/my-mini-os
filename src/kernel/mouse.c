/* ==========================================
 * PS/2 鼠标驱动实现 - mouse.c
 * 功能：
 *   1. PS/2 鼠标驱动
 *   2. 支持鼠标移动、按键检测
 *   3. 鼠标数据包解析
 * ========================================== */
#include "mouse.h"
#include "types.h"
#include "vga.h"
#include "isr.h"
#include "pic.h"

/* 鼠标状态 */
static mouse_state_t mouse_state;
static mouse_callback_t mouse_callback = NULL;

/* 鼠标数据包接收缓冲区 */
static uint8_t mouse_packet[3];
static uint8_t mouse_packet_index = 0;
static bool mouse_packet_ready = false;

/* 鼠标是否已初始化 */
static bool mouse_initialized = false;

/* ==========================================
 * 函数：mouse_wait_read
 * 功能：等待鼠标控制器就绪（可读）
 * ========================================== */
static void mouse_wait_read(void)
{
    while (!(inb(MOUSE_CMD_PORT) & 0x01)) {
        /* 忙等待 */
    }
}

/* ==========================================
 * 函数：mouse_wait_write
 * 功能：等待鼠标控制器就绪（可写）
 * ========================================== */
static void mouse_wait_write(void)
{
    while (inb(MOUSE_CMD_PORT) & 0x02) {
        /* 忙等待 */
    }
}

/* ==========================================
 * 函数：mouse_write
 * 功能：向鼠标发送数据
 * ========================================== */
static void mouse_write(uint8_t data)
{
    /* 等待控制器就绪 */
    mouse_wait_write();
    
    /* 发送命令：写入辅助设备 */
    outb(MOUSE_CMD_PORT, MOUSE_CMD_WRITE_AUX);
    
    /* 等待控制器就绪 */
    mouse_wait_write();
    
    /* 发送数据 */
    outb(MOUSE_DATA_PORT, data);
}

/* ==========================================
 * 函数：mouse_read
 * 功能：从鼠标读取数据
 * ========================================== */
static uint8_t mouse_read(void)
{
    /* 等待数据就绪 */
    mouse_wait_read();
    
    /* 读取数据 */
    return inb(MOUSE_DATA_PORT);
}

/* ==========================================
 * 函数：mouse_process_packet
 * 功能：处理鼠标数据包
 * ========================================== */
static void mouse_process_packet(void)
{
    uint8_t flags = mouse_packet[0];
    int8_t dx = (int8_t)mouse_packet[1];
    int8_t dy = (int8_t)mouse_packet[2];
    
    /* 检查溢出，如果溢出则忽略 */
    if (flags & (MOUSE_X_OVERFLOW | MOUSE_Y_OVERFLOW)) {
        return;
    }
    
    /* 处理符号位 */
    if (flags & MOUSE_X_SIGN) {
        dx = -((int8_t)(0xFF - dx + 1));
    }
    if (flags & MOUSE_Y_SIGN) {
        dy = -((int8_t)(0xFF - dy + 1));
    }
    
    /* 更新鼠标状态 */
    mouse_state.dx = dx;
    mouse_state.dy = -dy;  /* Y 轴反转 */
    mouse_state.x += dx;
    mouse_state.y -= dy;   /* Y 轴反转 */
    
    /* 限制坐标范围（假设 800x600 屏幕） */
    if (mouse_state.x < 0) mouse_state.x = 0;
    if (mouse_state.y < 0) mouse_state.y = 0;
    if (mouse_state.x > 800) mouse_state.x = 800;
    if (mouse_state.y > 600) mouse_state.y = 600;
    
    /* 更新按键状态 */
    mouse_state.buttons = flags & 0x07;
    mouse_state.left_pressed = (flags & MOUSE_LEFT_BUTTON) != 0;
    mouse_state.right_pressed = (flags & MOUSE_RIGHT_BUTTON) != 0;
    mouse_state.middle_pressed = (flags & MOUSE_MIDDLE_BUTTON) != 0;
    
    /* 调用回调函数 */
    if (mouse_callback != NULL) {
        mouse_callback(&mouse_state);
    }
}

/* ==========================================
 * 函数：mouse_handler
 * 功能：鼠标中断处理函数
 * ========================================== */
void mouse_handler(void)
{
    if (!mouse_initialized) {
        return;
    }
    
    uint8_t data = inb(MOUSE_DATA_PORT);
    
    /* 接收数据包 */
    if (mouse_packet_index == 0) {
        /* 第一个字节：检查是否有第3位（总是1），用于同步 */
        if (data & MOUSE_ALWAYS_1) {
            mouse_packet[0] = data;
            mouse_packet_index = 1;
        }
    } else if (mouse_packet_index == 1) {
        mouse_packet[1] = data;
        mouse_packet_index = 2;
    } else if (mouse_packet_index == 2) {
        mouse_packet[2] = data;
        mouse_packet_index = 0;
        mouse_packet_ready = true;
        
        /* 处理数据包 */
        mouse_process_packet();
    }
    
    /* 发送 EOI */
    pic_send_eoi(12);
}

/* ==========================================
 * 函数：mouse_get_state
 * 功能：获取鼠标状态
 * ========================================== */
void mouse_get_state(mouse_state_t *state)
{
    if (state == NULL) {
        return;
    }
    
    *state = mouse_state;
}

/* ==========================================
 * 函数：mouse_register_callback
 * 功能：注册鼠标回调函数
 * ========================================== */
void mouse_register_callback(mouse_callback_t callback)
{
    mouse_callback = callback;
}

/* ==========================================
 * 函数：mouse_init
 * 功能：初始化 PS/2 鼠标
 * ========================================== */
void mouse_init(void)
{
    uint8_t status;
    
    /* 初始化鼠标状态 */
    memset(&mouse_state, 0, sizeof(mouse_state_t));
    mouse_packet_index = 0;
    mouse_packet_ready = false;
    
    /* 启用辅助设备（鼠标） */
    mouse_wait_write();
    outb(MOUSE_CMD_PORT, MOUSE_CMD_ENABLE_AUX);
    
    /* 读取配置字节 */
    mouse_wait_write();
    outb(MOUSE_CMD_PORT, MOUSE_CMD_READ);
    mouse_wait_read();
    status = inb(MOUSE_DATA_PORT);
    
    /* 启用鼠标中断（IRQ12） */
    status |= 0x02;  /* 设置位 1：启用辅助设备中断 */
    
    /* 写回配置字节 */
    mouse_wait_write();
    outb(MOUSE_CMD_PORT, MOUSE_CMD_WRITE);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, status);
    
    /* 设置默认值 */
    mouse_write(MOUSE_SET_DEFAULTS);
    mouse_read();  /* 读取 ACK */
    
    /* 设置采样率 100Hz */
    mouse_write(MOUSE_SET_SAMPLING);
    mouse_read();  /* 读取 ACK */
    mouse_write(100);
    mouse_read();  /* 读取 ACK */
    
    /* 设置分辨率 4 计数/毫米 */
    mouse_write(MOUSE_SET_RESOLUTION);
    mouse_read();  /* 读取 ACK */
    mouse_write(2);  /* 2 = 4 计数/毫米 */
    mouse_read();  /* 读取 ACK */
    
    /* 设置 1:1 缩放 */
    mouse_write(MOUSE_SET_SCALING_1_1);
    mouse_read();  /* 读取 ACK */
    
    /* 启用数据包 */
    mouse_write(MOUSE_ENABLE_PACKET);
    mouse_read();  /* 读取 ACK */
    
    /* 注册中断处理函数（IRQ12 = 中断号 44） */
    isr_register_handler(44, mouse_handler);
    
    /* 启用 IRQ12 */
    pic_irq_mask(12);
    
    mouse_initialized = true;
    
    vga_puts("  [✓] PS/2 Mouse Driver\n");
}
