/* ==========================================
 * 网络基础头文件 - network.h
 * 功能：
 *   1. 网络基础类型定义
 *   2. MAC地址、IP地址结构
 *   3. 网络字节序转换
 *   4. 以太网帧结构
 * ========================================== */
#ifndef NETWORK_H
#define NETWORK_H

#include "types.h"

/* ==========================================
 * 网络字节序转换（x86是小端序）
 * ========================================== */
#define htons(x) ((uint16_t)(((x) >> 8) & 0xFF) | (((x) & 0xFF) << 8))
#define ntohs(x) htons(x)
#define htonl(x) ((uint32_t)(((x) >> 24) & 0xFF) | (((x) >> 8) & 0xFF00) | \
                  (((x) & 0xFF00) << 8) | (((x) & 0xFF) << 24))
#define ntohl(x) htonl(x)

/* ==========================================
 * MAC地址（6字节）
 * ========================================== */
#define MAC_ADDR_LEN    6

typedef struct {
    uint8_t addr[MAC_ADDR_LEN];
} mac_addr_t;

/* MAC地址比较 */
#define mac_equal(a, b) \
    ((a)->addr[0] == (b)->addr[0] && (a)->addr[1] == (b)->addr[1] && \
     (a)->addr[2] == (b)->addr[2] && (a)->addr[3] == (b)->addr[3] && \
     (a)->addr[4] == (b)->addr[4] && (a)->addr[5] == (b)->addr[5])

/* MAC地址格式化为字符串 */
#define mac_format(mac) \
    (mac)->addr[0], (mac)->addr[1], (mac)->addr[2], \
    (mac)->addr[3], (mac)->addr[4], (mac)->addr[5]

/* 广播MAC地址 */
extern const mac_addr_t MAC_BROADCAST;

/* ==========================================
 * IP地址（4字节）
 * ========================================== */
typedef uint32_t ip_addr_t;

/* IP地址构造宏 */
#define IP_ADDR(a, b, c, d) \
    ((ip_addr_t)((a) << 24) | (ip_addr_t)((b) << 16) | \
     (ip_addr_t)((c) << 8) | (ip_addr_t)(d))

/* IP地址提取宏 */
#define IP_A(ip) (((ip) >> 24) & 0xFF)
#define IP_B(ip) (((ip) >> 16) & 0xFF)
#define IP_C(ip) (((ip) >> 8) & 0xFF)
#define IP_D(ip) ((ip) & 0xFF)

/* 特殊IP地址 */
#define IP_ANY        0x00000000
#define IP_BROADCAST  0xFFFFFFFF
#define IP_LOOPBACK   0x7F000001  /* 127.0.0.1 */

/* ==========================================
 * 以太网帧类型
 * ========================================== */
#define ETH_TYPE_IP     0x0800  /* IPv4 */
#define ETH_TYPE_ARP    0x0806  /* ARP */
#define ETH_TYPE_RARP   0x8035  /* RARP */

/* 以太网帧头（14字节） */
typedef struct __attribute__((packed)) {
    mac_addr_t dest;        /* 目标MAC地址 */
    mac_addr_t src;         /* 源MAC地址 */
    uint16_t type;          /* 帧类型 */
} eth_header_t;

#define ETH_HEADER_SIZE   14
#define ETH_MTU           1500
#define ETH_MAX_FRAME     1514

/* ==========================================
 * 网络接口状态
 * ========================================== */
typedef enum {
    NET_IF_DOWN = 0,
    NET_IF_UP
} net_if_state_t;

/* ==========================================
 * 网络接口结构体
 * ========================================== */
#define IF_NAME_LEN   16

typedef struct net_interface {
    char name[IF_NAME_LEN];       /* 接口名称，如 "eth0" */
    
    mac_addr_t mac;               /* MAC地址 */
    ip_addr_t ip;                 /* IP地址 */
    ip_addr_t netmask;            /* 子网掩码 */
    ip_addr_t gateway;            /* 网关地址 */
    ip_addr_t dns;                /* DNS服务器 */
    
    net_if_state_t state;         /* 接口状态 */
    
    uint32_t rx_packets;          /* 接收包计数 */
    uint32_t tx_packets;          /* 发送包计数 */
    uint32_t rx_bytes;            /* 接收字节数 */
    uint32_t tx_bytes;            /* 发送字节数 */
    uint32_t rx_errors;           /* 接收错误 */
    uint32_t tx_errors;           /* 发送错误 */
    
    /* 驱动函数指针 */
    int (*send)(struct net_interface *iface, const void *data, uint32_t len);
    int (*recv)(struct net_interface *iface, void *buf, uint32_t len);
    void (*irq_handler)(struct net_interface *iface);
    
    /* 驱动私有数据 */
    void *driver_data;
} net_interface_t;

/* ==========================================
 * 网络数据包缓冲区
 * ========================================== */
#define NET_BUF_SIZE    1600

typedef struct {
    uint8_t data[NET_BUF_SIZE];
    uint32_t len;
} net_buf_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化网络子系统 */
void network_init(void);

/* 注册网络接口 */
int net_register_interface(net_interface_t *iface);

/* 获取默认网络接口 */
net_interface_t *net_get_default_interface(void);

/* 发送以太网帧 */
int net_send_frame(net_interface_t *iface, const mac_addr_t *dest,
                   uint16_t type, const void *payload, uint32_t len);

/* 接收以太网帧（由驱动调用） */
void net_receive_frame(net_interface_t *iface, const void *data, uint32_t len);

/* 解析IP地址字符串 */
ip_addr_t ip_parse(const char *str);

/* 格式化IP地址为字符串 */
void ip_format(ip_addr_t ip, char *buf);

#endif /* NETWORK_H */
