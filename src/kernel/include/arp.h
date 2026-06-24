/* ==========================================
 * ARP协议头文件 - arp.h
 * 功能：
 *   1. ARP缓存管理
 *   2. ARP请求/响应处理
 *   3. IP到MAC地址解析
 * ========================================== */
#ifndef ARP_H
#define ARP_H

#include "network.h"

/* ==========================================
 * ARP协议常量
 * ========================================== */
#define ARP_HW_ETHER    1   /* 以太网硬件类型 */
#define ARP_OP_REQUEST  1   /* ARP请求 */
#define ARP_OP_REPLY    2   /* ARP响应 */

#define ARP_CACHE_SIZE  32  /* ARP缓存条目数 */
#define ARP_TIMEOUT     300 /* ARP条目超时（秒） */
#define ARP_RETRY_MAX   3   /* 最大重试次数 */

/* ==========================================
 * ARP帧结构
 * ========================================== */
typedef struct __attribute__((packed)) {
    uint16_t hw_type;       /* 硬件类型 */
    uint16_t proto_type;    /* 协议类型 */
    uint8_t  hw_len;        /* 硬件地址长度 */
    uint8_t  proto_len;     /* 协议地址长度 */
    uint16_t opcode;        /* 操作码 */
    mac_addr_t sender_mac;  /* 发送者MAC */
    ip_addr_t  sender_ip;   /* 发送者IP */
    mac_addr_t target_mac;  /* 目标MAC */
    ip_addr_t  target_ip;   /* 目标IP */
} arp_packet_t;

#define ARP_PACKET_SIZE  28

/* ==========================================
 * ARP缓存条目
 * ========================================== */
typedef enum {
    ARP_ENTRY_FREE = 0,     /* 空闲 */
    ARP_ENTRY_PENDING,      /* 等待响应 */
    ARP_ENTRY_VALID         /* 有效 */
} arp_entry_state_t;

typedef struct {
    ip_addr_t ip;                  /* IP地址 */
    mac_addr_t mac;                /* MAC地址 */
    arp_entry_state_t state;       /* 条目状态 */
    uint32_t timestamp;            /* 创建/更新时间戳 */
    uint8_t retry_count;           /* 重试计数 */
} arp_entry_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化ARP子系统 */
void arp_init(void);

/* 查找IP对应的MAC地址 */
int arp_resolve(net_interface_t *iface, ip_addr_t ip, mac_addr_t *mac);

/* 处理收到的ARP包 */
void arp_receive(net_interface_t *iface, const arp_packet_t *arp, uint32_t len);

/* 发送ARP请求 */
int arp_send_request(net_interface_t *iface, ip_addr_t target_ip);

/* 发送ARP响应 */
int arp_send_reply(net_interface_t *iface, ip_addr_t target_ip, const mac_addr_t *target_mac);

/* 添加/更新ARP缓存条目 */
void arp_cache_update(ip_addr_t ip, const mac_addr_t *mac);

/* 删除ARP缓存条目 */
void arp_cache_remove(ip_addr_t ip);

/* 清空ARP缓存 */
void arp_cache_clear(void);

/* 显示ARP缓存表 */
void arp_cache_show(void);

/* 处理ARP超时 */
void arp_timeout_check(void);

#endif /* ARP_H */
