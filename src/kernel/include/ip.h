/* ==========================================
 * IP协议头文件 - ip.h
 * 功能：
 *   1. IP数据包封装/解封装
 *   2. IP路由查找
 *   3. IP校验和计算
 *   4. IP分片与重组（简化版）
 * ========================================== */
#ifndef IP_H
#define IP_H

#include "network.h"

/* ==========================================
 * IP协议常量
 * ========================================== */
#define IP_VERSION      4    /* IPv4 */
#define IP_HEADER_LEN   20   /* 基本IP头部长度（无选项） */
#define IP_DEFAULT_TTL  64   /* 默认TTL */

/* IP协议号 */
#define IP_PROTO_ICMP   1    /* ICMP */
#define IP_PROTO_TCP    6    /* TCP */
#define IP_PROTO_UDP    17   /* UDP */

/* IP标志位 */
#define IP_FLAG_DF      0x4000  /* 不分片 */
#define IP_FLAG_MF      0x2000  /* 更多分片 */

/* ==========================================
 * IP数据包头部结构
 * ========================================== */
typedef struct __attribute__((packed)) {
    uint8_t  ver_ihl;     /* 版本(4位) + 头部长度(4位) */
    uint8_t  tos;         /* 服务类型 */
    uint16_t total_len;   /* 总长度 */
    uint16_t id;          /* 标识 */
    uint16_t flags_frag;  /* 标志 + 片偏移 */
    uint8_t  ttl;         /* 生存时间 */
    uint8_t  protocol;    /* 协议 */
    uint16_t checksum;    /* 首部校验和 */
    ip_addr_t src_ip;     /* 源IP地址 */
    ip_addr_t dst_ip;     /* 目标IP地址 */
} ip_header_t;

/* ==========================================
 * IP数据包信息（解析后的结构）
 * ========================================== */
typedef struct {
    ip_header_t *header;           /* IP头部指针 */
    uint8_t     *payload;          /* 载荷数据指针 */
    uint32_t     payload_len;      /* 载荷长度 */
    uint8_t      header_len;       /* 头部长度（字节） */
    uint8_t      protocol;         /* 协议号 */
    ip_addr_t    src_ip;           /* 源IP */
    ip_addr_t    dst_ip;           /* 目标IP */
} ip_packet_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化IP子系统 */
void ip_init(void);

/* 发送IP数据包 */
int ip_send(net_interface_t *iface, ip_addr_t dst_ip, uint8_t protocol,
            const void *payload, uint32_t payload_len);

/* 处理收到的IP包 */
void ip_receive(net_interface_t *iface, const void *data, uint32_t len);

/* 计算IP校验和 */
uint16_t ip_checksum(const void *data, uint32_t len);

/* 计算伪首部校验和（用于TCP/UDP） */
uint16_t ip_pseudo_checksum(ip_addr_t src, ip_addr_t dst,
                            uint8_t protocol, uint16_t length);

/* 获取下一个IP包标识 */
uint16_t ip_next_id(void);

/* IP地址是否在本地子网 */
bool ip_is_local(ip_addr_t ip);

/* 判断是否为本机IP */
bool ip_is_mine(ip_addr_t ip);

#endif /* IP_H */
