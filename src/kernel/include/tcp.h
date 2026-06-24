/* ==========================================
 * TCP协议头文件 - tcp.h
 * 功能：
 *   1. TCP连接管理（三次握手/四次挥手）
 *   2. TCP数据可靠传输
 *   3. TCP流量控制（简化版）
 * ========================================== */
#ifndef TCP_H
#define TCP_H

#include "network.h"
#include "ip.h"

/* ==========================================
 * TCP常量
 * ========================================== */
#define TCP_HEADER_SIZE     20      /* 最小TCP头部长度 */
#define TCP_MAX_PAYLOAD     1460    /* 最大载荷（MSS） */
#define TCP_MAX_CONNECTIONS 16      /* 最大连接数 */

/* TCP标志位 */
#define TCP_FLAG_FIN    0x01    /* 完成 */
#define TCP_FLAG_SYN    0x02    /* 同步 */
#define TCP_FLAG_RST    0x04    /* 复位 */
#define TCP_FLAG_PSH    0x08    /* 推送 */
#define TCP_FLAG_ACK    0x10    /* 确认 */
#define TCP_FLAG_URG    0x20    /* 紧急 */

/* TCP状态 */
typedef enum {
    TCP_CLOSED = 0,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT
} tcp_state_t;

/* ==========================================
 * TCP头部结构
 * ========================================== */
typedef struct __attribute__((packed)) {
    uint16_t src_port;    /* 源端口 */
    uint16_t dst_port;    /* 目标端口 */
    uint32_t seq_num;     /* 序列号 */
    uint32_t ack_num;     /* 确认号 */
    uint8_t  data_offset; /* 数据偏移(4位) + 保留(4位) */
    uint8_t  flags;       /* 标志位 */
    uint16_t window;      /* 窗口大小 */
    uint16_t checksum;    /* 校验和 */
    uint16_t urgent_ptr;  /* 紧急指针 */
} tcp_header_t;

/* ==========================================
 * TCP连接结构体
 * ========================================== */
#define TCP_RECV_BUF_SIZE  4096
#define TCP_SEND_BUF_SIZE  4096

typedef struct tcp_connection {
    /* 连接标识 */
    ip_addr_t local_ip;
    uint16_t  local_port;
    ip_addr_t remote_ip;
    uint16_t  remote_port;
    
    /* 状态 */
    tcp_state_t state;
    
    /* 序列号管理 */
    uint32_t send_next;       /* SND.NXT - 下一个发送序列号 */
    uint32_t send_unack;      /* SND.UNA - 第一个未确认序列号 */
    uint32_t recv_next;       /* RCV.NXT - 下一个期望接收序列号 */
    
    /* 窗口 */
    uint16_t send_window;     /* 发送窗口 */
    uint16_t recv_window;     /* 接收窗口 */
    
    /* 缓冲区 */
    uint8_t recv_buf[TCP_RECV_BUF_SIZE];
    uint32_t recv_buf_len;
    uint8_t send_buf[TCP_SEND_BUF_SIZE];
    uint32_t send_buf_len;
    
    /* 定时器 */
    uint32_t rtt_estimate;    /* RTT估计（ms） */
    uint32_t rto;             /* 重传超时 */
    uint32_t last_send_time;  /* 上次发送时间 */
    uint8_t  retransmit_count;/* 重传次数 */
    
    /* 连接选项 */
    uint16_t mss;             /* 最大段大小 */
    
    /* 回调 */
    void (*on_data)(struct tcp_connection *conn, const void *data, uint32_t len);
    void (*on_connect)(struct tcp_connection *conn);
    void (*on_close)(struct tcp_connection *conn);
    
    /* 用户数据 */
    void *user_data;
    
    /* 连接有效标志 */
    bool active;
} tcp_connection_t;

/* ==========================================
 * TCP伪首部（用于校验和计算）
 * ========================================== */
typedef struct __attribute__((packed)) {
    ip_addr_t src_ip;
    ip_addr_t dst_ip;
    uint8_t   zero;
    uint8_t   protocol;
    uint16_t  tcp_length;
} tcp_pseudo_header_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化TCP子系统 */
void tcp_init(void);

/* 创建TCP连接 */
tcp_connection_t *tcp_create_connection(void);

/* 释放TCP连接 */
void tcp_destroy_connection(tcp_connection_t *conn);

/* 主动打开（客户端） */
int tcp_connect(tcp_connection_t *conn, ip_addr_t remote_ip, uint16_t remote_port);

/* 被动打开（服务器） */
int tcp_listen(tcp_connection_t *conn, uint16_t local_port);

/* 发送数据 */
int tcp_send(tcp_connection_t *conn, const void *data, uint32_t len);

/* 接收数据 */
int tcp_recv(tcp_connection_t *conn, void *buf, uint32_t len);

/* 关闭连接 */
int tcp_close(tcp_connection_t *conn);

/* 处理收到的TCP包 */
void tcp_receive(net_interface_t *iface, ip_addr_t src_ip, ip_addr_t dst_ip,
                 const void *data, uint32_t len);

/* 计算TCP校验和 */
uint16_t tcp_checksum(ip_addr_t src_ip, ip_addr_t dst_ip,
                      const void *data, uint32_t len);

/* TCP定时器处理（由时钟中断调用） */
void tcp_timer_tick(void);

/* 获取连接状态名称 */
const char *tcp_state_name(tcp_state_t state);

#endif /* TCP_H */
