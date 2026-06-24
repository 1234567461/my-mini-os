/* ==========================================
 * DHCP客户端头文件 - dhcp.h
 * 功能：
 *   1. DHCP客户端协议实现
 *   2. 自动获取IP地址
 *   3. 租约管理
 * ========================================== */
#ifndef DHCP_H
#define DHCP_H

#include "network.h"
#include "udp.h"

/* ==========================================
 * DHCP常量
 * ========================================== */
#define DHCP_SERVER_PORT    67
#define DHCP_CLIENT_PORT    68

#define DHCP_OP_REQUEST     1
#define DHCP_OP_REPLY       2

#define DHCP_HTYPE_ETHER    1
#define DHCP_HLEN_ETHER     6

/* DHCP消息类型 */
#define DHCP_DISCOVER       1
#define DHCP_OFFER          2
#define DHCP_REQUEST        3
#define DHCP_DECLINE        4
#define DHCP_ACK            5
#define DHCP_NAK            6
#define DHCP_RELEASE        7
#define DHCP_INFORM         8

/* DHCP选项 */
#define DHCP_OPT_PAD            0
#define DHCP_OPT_SUBNET_MASK    1
#define DHCP_OPT_ROUTER         3
#define DHCP_OPT_DNS            6
#define DHCP_OPT_HOSTNAME       12
#define DHCP_OPT_DOMAIN         15
#define DHCP_OPT_REQUESTED_IP   50
#define DHCP_OPT_LEASE_TIME     51
#define DHCP_OPT_MSG_TYPE       53
#define DHCP_OPT_SERVER_ID      54
#define DHCP_OPT_PARAM_LIST     55
#define DHCP_OPT_RENEWAL_TIME   58
#define DHCP_OPT_REBIND_TIME    59
#define DHCP_OPT_END            255

/* DHCP超时 */
#define DHCP_TIMEOUT_MS     5000    /* 请求超时 */
#define DHCP_RETRY_MAX      3       /* 最大重试 */
#define DHCP_LEASE_DEFAULT  86400   /* 默认租约（秒） */

/* ==========================================
 * DHCP消息结构
 * ========================================== */
typedef struct __attribute__((packed)) {
    uint8_t  op;           /* 消息类型 */
    uint8_t  htype;        /* 硬件类型 */
    uint8_t  hlen;         /* 硬件地址长度 */
    uint8_t  hops;         /* 跳数 */
    uint32_t xid;          /* 事务ID */
    uint16_t secs;         /* 经过秒数 */
    uint16_t flags;        /* 标志 */
    ip_addr_t ciaddr;      /* 客户端IP */
    ip_addr_t yiaddr;      /* 你的IP */
    ip_addr_t siaddr;      /* 服务器IP */
    ip_addr_t giaddr;      /* 网关IP */
    uint8_t  chaddr[16];   /* 客户端硬件地址 */
    uint8_t  sname[64];    /* 服务器名 */
    uint8_t  file[128];    /* 启动文件名 */
    uint32_t magic_cookie; /* 魔术Cookie */
    uint8_t  options[308]; /* 选项（可变长） */
} dhcp_message_t;

#define DHCP_MAGIC_COOKIE   0x63825363

/* ==========================================
 * DHCP客户端状态
 * ========================================== */
typedef enum {
    DHCP_STATE_INIT = 0,
    DHCP_STATE_SELECTING,
    DHCP_STATE_REQUESTING,
    DHCP_STATE_BOUND,
    DHCP_STATE_RENEWING,
    DHCP_STATE_REBINDING
} dhcp_state_t;

/* ==========================================
 * DHCP租约信息
 * ========================================== */
typedef struct {
    ip_addr_t ip;           /* 分配的IP */
    ip_addr_t netmask;      /* 子网掩码 */
    ip_addr_t gateway;      /* 网关 */
    ip_addr_t dns;          /* DNS服务器 */
    ip_addr_t server_ip;    /* DHCP服务器IP */
    uint32_t lease_time;    /* 租约时间（秒） */
    uint32_t renewal_time;  /* 续约时间（秒） */
    uint32_t rebind_time;   /* 重绑时间（秒） */
    uint32_t start_time;    /* 租约开始时间 */
} dhcp_lease_t;

/* ==========================================
 * DHCP客户端上下文
 * ========================================== */
typedef struct {
    net_interface_t *iface;      /* 网络接口 */
    dhcp_state_t state;          /* 状态机状态 */
    uint32_t xid;                /* 事务ID */
    dhcp_lease_t lease;          /* 当前租约 */
    uint32_t timeout;            /* 超时时间戳 */
    uint8_t retry_count;         /* 重试计数 */
} dhcp_client_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化DHCP客户端 */
void dhcp_init(net_interface_t *iface);

/* 开始DHCP获取IP */
int dhcp_start(void);

/* 释放当前租约 */
int dhcp_release(void);

/* 续约租约 */
int dhcp_renew(void);

/* 获取当前DHCP状态 */
dhcp_state_t dhcp_get_state(void);

/* 获取当前租约信息 */
dhcp_lease_t *dhcp_get_lease(void);

/* DHCP定时器处理 */
void dhcp_timer_tick(void);

/* 显示DHCP状态 */
void dhcp_show_status(void);

#endif /* DHCP_H */
