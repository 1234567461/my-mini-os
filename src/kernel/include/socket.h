/* ==========================================
 * Socket API头文件 - socket.h
 * 功能：
 *   1. BSD Socket风格API
 *   2. 支持TCP和UDP
 *   3. 提供用户态网络编程接口
 * ========================================== */
#ifndef SOCKET_H
#define SOCKET_H

#include "network.h"
#include "tcp.h"
#include "udp.h"

/* ==========================================
 * Socket类型
 * ========================================== */
typedef enum {
    SOCK_STREAM = 1,    /* TCP流式Socket */
    SOCK_DGRAM  = 2     /* UDP数据报Socket */
} socket_type_t;

/* ==========================================
 * 地址族
 * ========================================== */
#define AF_INET     2   /* IPv4 */

/* ==========================================
 * Socket地址结构
 * ========================================== */
typedef struct {
    uint16_t sin_family;     /* 地址族（AF_INET） */
    uint16_t sin_port;       /* 端口号（网络字节序） */
    ip_addr_t sin_addr;      /* IP地址（网络字节序） */
    uint8_t  sin_zero[8];    /* 填充 */
} sockaddr_in_t;

/* ==========================================
 * Socket选项
 * ========================================== */
#define SOL_SOCKET      1   /* Socket层选项 */
#define SO_REUSEADDR    2   /* 允许重用本地地址 */
#define SO_KEEPALIVE    9   /* 保持连接 */
#define SO_RCVBUF       8   /* 接收缓冲区大小 */
#define SO_SNDBUF       7   /* 发送缓冲区大小 */

/* ==========================================
 * Socket文件描述符
 * ========================================== */
#define MAX_SOCKETS     32

typedef enum {
    SOCKET_STATE_FREE = 0,
    SOCKET_STATE_CREATED,
    SOCKET_STATE_BOUND,
    SOCKET_STATE_LISTENING,
    SOCKET_STATE_CONNECTED,
    SOCKET_STATE_CLOSED
} socket_state_t;

typedef struct {
    int fd;                    /* 文件描述符 */
    socket_type_t type;        /* Socket类型 */
    socket_state_t state;      /* 状态 */
    
    /* 本地地址 */
    ip_addr_t local_ip;
    uint16_t local_port;
    
    /* 远端地址（TCP连接用） */
    ip_addr_t remote_ip;
    uint16_t remote_port;
    
    /* TCP连接 */
    tcp_connection_t *tcp_conn;
    
    /* UDP绑定 */
    uint16_t udp_port;
    
    /* 接收缓冲区 */
    uint8_t recv_buf[4096];
    uint32_t recv_buf_start;
    uint32_t recv_buf_end;
    uint32_t recv_buf_len;
    
    /* 非阻塞标志 */
    bool non_blocking;
    
    /* 错误码 */
    int error;
} socket_t;

/* ==========================================
 * 错误码
 * ========================================== */
#define SOCKET_OK           0
#define SOCKET_ERR_INVALID  -1   /* 无效参数 */
#define SOCKET_ERR_NOSOCK   -2   /* 无可用Socket */
#define SOCKET_ERR_ADDRINUSE -3  /* 地址已使用 */
#define SOCKET_ERR_CONNREFUSED -4 /* 连接被拒绝 */
#define SOCKET_ERR_TIMEDOUT -5   /* 超时 */
#define SOCKET_ERR_NOTCONN  -6   /* 未连接 */
#define SOCKET_ERR_ALREADY  -7   /* 已连接/已监听 */
#define SOCKET_ERR_WOULDBLOCK -8 /* 操作将阻塞 */
#define SOCKET_ERR_NOMEM    -9   /* 内存不足 */

/* ==========================================
 * 函数声明（BSD Socket API）
 * ========================================== */

/* 初始化Socket子系统 */
void socket_init(void);

/* 创建Socket */
int socket_create(int domain, int type, int protocol);

/* 绑定地址 */
int socket_bind(int fd, const sockaddr_in_t *addr);

/* 监听（TCP） */
int socket_listen(int fd, int backlog);

/* 接受连接（TCP） */
int socket_accept(int fd, sockaddr_in_t *addr);

/* 连接（TCP） */
int socket_connect(int fd, const sockaddr_in_t *addr);

/* 发送数据 */
int socket_send(int fd, const void *buf, uint32_t len, int flags);

/* 接收数据 */
int socket_recv(int fd, void *buf, uint32_t len, int flags);

/* 发送到指定地址（UDP） */
int socket_sendto(int fd, const void *buf, uint32_t len, int flags,
                  const sockaddr_in_t *dest_addr);

/* 从指定地址接收（UDP） */
int socket_recvfrom(int fd, void *buf, uint32_t len, int flags,
                    sockaddr_in_t *src_addr);

/* 关闭Socket */
int socket_close(int fd);

/* 设置Socket选项 */
int socket_setsockopt(int fd, int level, int optname,
                      const void *optval, uint32_t optlen);

/* 设置非阻塞模式 */
int socket_set_nonblocking(int fd, bool non_blocking);

/* 获取Socket错误 */
int socket_get_error(int fd);

/* 获取本地地址 */
int socket_getsockname(int fd, sockaddr_in_t *addr);

/* 获取远端地址 */
int socket_getpeername(int fd, sockaddr_in_t *addr);

#endif /* SOCKET_H */
