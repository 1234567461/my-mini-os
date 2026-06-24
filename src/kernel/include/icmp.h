/* ==========================================
 * ICMP协议头文件 - icmp.h
 * 功能：
 *   1. ICMP Echo Request/Reply（ping）
 *   2. ICMP错误消息
 *   3. ICMP数据包处理
 * ========================================== */
#ifndef ICMP_H
#define ICMP_H

#include "network.h"
#include "ip.h"

/* ==========================================
 * ICMP类型
 * ========================================== */
#define ICMP_TYPE_ECHO_REPLY    0    /* Echo回复 */
#define ICMP_TYPE_DEST_UNREACH  3    /* 目标不可达 */
#define ICMP_TYPE_REDIRECT      5    /* 重定向 */
#define ICMP_TYPE_ECHO_REQUEST  8    /* Echo请求 */
#define ICMP_TYPE_TIME_EXCEED   11   /* 超时 */

/* ICMP代码 */
#define ICMP_CODE_NET_UNREACH   0    /* 网络不可达 */
#define ICMP_CODE_HOST_UNREACH  1    /* 主机不可达 */
#define ICMP_CODE_TTL_EXCEEDED  0    /* TTL超时 */

/* ==========================================
 * ICMP头部结构
 * ========================================== */
typedef struct __attribute__((packed)) {
    uint8_t  type;        /* 类型 */
    uint8_t  code;        /* 代码 */
    uint16_t checksum;    /* 校验和 */
    uint16_t id;          /* 标识（Echo请求/回复用） */
    uint16_t seq;         /* 序列号（Echo请求/回复用） */
} icmp_header_t;

#define ICMP_HEADER_SIZE  8

/* 不可达/超时等错误消息中，需要包含触发错误的原始IP包头部+前8字节数据 */
#define ICMP_ERROR_DATA_LEN  (IP_HEADER_LEN + 8)

/* ==========================================
 * Ping统计
 * ========================================== */
typedef struct {
    uint32_t sent;        /* 发送数 */
    uint32_t received;    /* 接收数 */
    uint32_t lost;        /* 丢失数 */
    uint32_t min_rtt;     /* 最小RTT（ms） */
    uint32_t max_rtt;     /* 最大RTT（ms） */
    uint32_t total_rtt;   /* 总RTT（ms） */
} ping_stats_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化ICMP子系统 */
void icmp_init(void);

/* 处理收到的ICMP包 */
void icmp_receive(net_interface_t *iface, ip_addr_t src_ip,
                  const void *data, uint32_t len);

/* 发送Echo请求（ping） */
int icmp_send_echo(net_interface_t *iface, ip_addr_t dst_ip,
                   uint16_t id, uint16_t seq, const void *data, uint32_t data_len);

/* 发送目标不可达 */
int icmp_send_dest_unreach(net_interface_t *iface, ip_addr_t dst_ip,
                           const void *orig_packet, uint32_t orig_len);

/* 发送TTL超时 */
int icmp_send_time_exceeded(net_interface_t *iface, ip_addr_t dst_ip,
                            const void *orig_packet, uint32_t orig_len);

/* 执行ping操作 */
int icmp_ping(net_interface_t *iface, ip_addr_t dst_ip, uint32_t count,
              uint32_t timeout_ms, ping_stats_t *stats);

#endif /* ICMP_H */
