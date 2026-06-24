/* ==========================================
 * UDP协议头文件 - udp.h
 * 功能：
 *   1. UDP数据包封装/解封装
 *   2. UDP端口管理
 *   3. UDP Socket支持
 * ========================================== */
#ifndef UDP_H
#define UDP_H

#include "network.h"
#include "ip.h"

/* ==========================================
 * UDP常量
 * ========================================== */
#define UDP_HEADER_SIZE     8      /* UDP头部长度 */
#define UDP_MAX_PAYLOAD     1472   /* 最大载荷（MTU - IP头 - UDP头） */
#define UDP_MAX_PORTS       64     /* 最大监听端口数 */

/* ==========================================
 * UDP头部结构
 * ========================================== */
typedef struct __attribute__((packed)) {
    uint16_t src_port;    /* 源端口 */
    uint16_t dst_port;    /* 目标端口 */
    uint16_t length;      /* UDP长度（头部+数据） */
    uint16_t checksum;    /* 校验和 */
} udp_header_t;

/* ==========================================
 * UDP伪首部（用于校验和计算）
 * ========================================== */
typedef struct __attribute__((packed)) {
    ip_addr_t src_ip;
    ip_addr_t dst_ip;
    uint8_t   zero;
    uint8_t   protocol;
    uint16_t  udp_length;
} udp_pseudo_header_t;

/* ==========================================
 * UDP端口回调函数
 * ========================================== */
typedef void (*udp_callback_t)(net_interface_t *iface,
                               ip_addr_t src_ip, uint16_t src_port,
                               const void *data, uint32_t len);

/* ==========================================
 * UDP端口注册表
 * ========================================== */
typedef struct {
    uint16_t port;           /* 端口号 */
    udp_callback_t callback; /* 回调函数 */
    bool active;             /* 是否激活 */
} udp_port_entry_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化UDP子系统 */
void udp_init(void);

/* 发送UDP数据包 */
int udp_send(net_interface_t *iface, ip_addr_t dst_ip, uint16_t dst_port,
             uint16_t src_port, const void *data, uint32_t len);

/* 处理收到的UDP包 */
void udp_receive(net_interface_t *iface, ip_addr_t src_ip, ip_addr_t dst_ip,
                 const void *data, uint32_t len);

/* 注册UDP端口监听 */
int udp_bind(uint16_t port, udp_callback_t callback);

/* 解除UDP端口监听 */
int udp_unbind(uint16_t port);

/* 计算UDP校验和 */
uint16_t udp_checksum(ip_addr_t src_ip, ip_addr_t dst_ip,
                      const void *data, uint32_t len);

#endif /* UDP_H */
