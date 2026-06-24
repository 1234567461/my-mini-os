/* ==========================================
 * Socket API实现 - socket.c
 * 功能：
 *   1. BSD Socket风格API
 *   2. TCP/UDP Socket支持
 *   3. 连接管理和数据传输
 * ========================================== */

#include "socket.h"
#include "network.h"
#include "tcp.h"
#include "udp.h"
#include "heap.h"
#include "string.h"
#include "klog.h"

/* Socket表 */
static socket_t socket_table[MAX_SOCKETS];

/* ==========================================
 * 函数：socket_init
 * 功能：初始化Socket子系统
 * ========================================== */
void socket_init(void)
{
    for (int i = 0; i < MAX_SOCKETS; i++) {
        socket_table[i].fd = -1;
        socket_table[i].state = SOCKET_STATE_FREE;
    }
    
    klog_log("socket", "Socket subsystem initialized");
}

/* ==========================================
 * 函数：socket_alloc
 * 功能：分配Socket
 * ========================================== */
static socket_t *socket_alloc(void)
{
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (socket_table[i].state == SOCKET_STATE_FREE) {
            socket_table[i].fd = i;
            socket_table[i].state = SOCKET_STATE_CREATED;
            socket_table[i].error = 0;
            socket_table[i].non_blocking = false;
            socket_table[i].recv_buf_start = 0;
            socket_table[i].recv_buf_end = 0;
            socket_table[i].recv_buf_len = 0;
            return &socket_table[i];
        }
    }
    return NULL;
}

/* ==========================================
 * 函数：socket_create
 * 功能：创建Socket
 * ========================================== */
int socket_create(int domain, int type, int protocol)
{
    if (domain != AF_INET) {
        return SOCKET_ERR_INVALID;
    }
    
    if (type != SOCK_STREAM && type != SOCK_DGRAM) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = socket_alloc();
    if (sock == NULL) {
        return SOCKET_ERR_NOSOCK;
    }
    
    sock->type = type;
    
    if (type == SOCK_STREAM) {
        /* TCP连接 */
        sock->tcp_conn = tcp_create_connection();
        if (sock->tcp_conn == NULL) {
            sock->state = SOCKET_STATE_FREE;
            return SOCKET_ERR_NOMEM;
        }
    }
    
    klog_log("socket", "Created %s socket fd=%d",
             type == SOCK_STREAM ? "TCP" : "UDP", sock->fd);
    
    return sock->fd;
}

/* ==========================================
 * 函数：socket_bind
 * 功能：绑定地址
 * ========================================== */
int socket_bind(int fd, const sockaddr_in_t *addr)
{
    if (fd < 0 || fd >= MAX_SOCKETS || addr == NULL) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    if (sock->state != SOCKET_STATE_CREATED) {
        return SOCKET_ERR_ALREADY;
    }
    
    sock->local_ip = addr->sin_addr;
    sock->local_port = ntohs(addr->sin_port);
    sock->state = SOCKET_STATE_BOUND;
    
    /* 如果是UDP，注册端口监听 */
    if (sock->type == SOCK_DGRAM) {
        /* UDP回调需要转发到Socket */
        udp_bind(sock->local_port, NULL);
    }
    
    return SOCKET_OK;
}

/* ==========================================
 * 函数：socket_listen
 * 功能：监听（TCP）
 * ========================================== */
int socket_listen(int fd, int backlog)
{
    if (fd < 0 || fd >= MAX_SOCKETS) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    if (sock->type != SOCK_STREAM) {
        return SOCKET_ERR_INVALID;
    }
    
    if (sock->state != SOCKET_STATE_BOUND && sock->state != SOCKET_STATE_CREATED) {
        return SOCKET_ERR_ALREADY;
    }
    
    if (sock->tcp_conn != NULL) {
        int ret = tcp_listen(sock->tcp_conn, sock->local_port);
        if (ret != 0) {
            return SOCKET_ERR_INVALID;
        }
    }
    
    sock->state = SOCKET_STATE_LISTENING;
    
    klog_log("socket", "Listening on port %d", sock->local_port);
    
    return SOCKET_OK;
}

/* ==========================================
 * 函数：socket_accept
 * 功能：接受连接（TCP）
 * ========================================== */
int socket_accept(int fd, sockaddr_in_t *addr)
{
    if (fd < 0 || fd >= MAX_SOCKETS) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    if (sock->state != SOCKET_STATE_LISTENING) {
        return SOCKET_ERR_INVALID;
    }
    
    /* 简化版：等待连接建立 */
    if (sock->tcp_conn != NULL && sock->tcp_conn->state == TCP_ESTABLISHED) {
        socket_t *new_sock = socket_alloc();
        if (new_sock == NULL) {
            return SOCKET_ERR_NOSOCK;
        }
        
        new_sock->type = SOCK_STREAM;
        new_sock->state = SOCKET_STATE_CONNECTED;
        new_sock->local_ip = sock->tcp_conn->local_ip;
        new_sock->local_port = sock->tcp_conn->local_port;
        new_sock->remote_ip = sock->tcp_conn->remote_ip;
        new_sock->remote_port = sock->tcp_conn->remote_port;
        new_sock->tcp_conn = sock->tcp_conn;
        
        if (addr != NULL) {
            addr->sin_family = AF_INET;
            addr->sin_port = htons(new_sock->remote_port);
            addr->sin_addr = new_sock->remote_ip;
        }
        
        /* 原Socket重新监听 */
        sock->tcp_conn = tcp_create_connection();
        if (sock->tcp_conn != NULL) {
            tcp_listen(sock->tcp_conn, sock->local_port);
        }
        
        klog_log("socket", "Accepted connection from fd=%d", new_sock->fd);
        
        return new_sock->fd;
    }
    
    if (sock->non_blocking) {
        return SOCKET_ERR_WOULDBLOCK;
    }
    
    return SOCKET_ERR_TIMEDOUT;
}

/* ==========================================
 * 函数：socket_connect
 * 功能：连接（TCP）
 * ========================================== */
int socket_connect(int fd, const sockaddr_in_t *addr)
{
    if (fd < 0 || fd >= MAX_SOCKETS || addr == NULL) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    if (sock->type != SOCK_STREAM) {
        return SOCKET_ERR_INVALID;
    }
    
    if (sock->state == SOCKET_STATE_CONNECTED) {
        return SOCKET_ERR_ALREADY;
    }
    
    sock->remote_ip = addr->sin_addr;
    sock->remote_port = ntohs(addr->sin_port);
    
    /* 如果没有本地端口，自动分配 */
    if (sock->local_port == 0) {
        net_interface_t *iface = net_get_default_interface();
        if (iface != NULL) {
            sock->local_ip = iface->ip;
        }
    }
    
    if (sock->tcp_conn != NULL) {
        int ret = tcp_connect(sock->tcp_conn, sock->remote_ip, sock->remote_port);
        if (ret != 0) {
            sock->error = SOCKET_ERR_CONNREFUSED;
            return ret;
        }
    }
    
    sock->state = SOCKET_STATE_CONNECTED;
    
    klog_log("socket", "Connected to remote fd=%d", sock->fd);
    
    return SOCKET_OK;
}

/* ==========================================
 * 函数：socket_send
 * 功能：发送数据
 * ========================================== */
int socket_send(int fd, const void *buf, uint32_t len, int flags)
{
    if (fd < 0 || fd >= MAX_SOCKETS || buf == NULL) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    if (sock->state != SOCKET_STATE_CONNECTED) {
        return SOCKET_ERR_NOTCONN;
    }
    
    if (sock->type == SOCK_STREAM) {
        if (sock->tcp_conn != NULL) {
            return tcp_send(sock->tcp_conn, buf, len);
        }
    }
    
    return SOCKET_ERR_INVALID;
}

/* ==========================================
 * 函数：socket_recv
 * 功能：接收数据
 * ========================================== */
int socket_recv(int fd, void *buf, uint32_t len, int flags)
{
    if (fd < 0 || fd >= MAX_SOCKETS || buf == NULL) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    if (sock->state != SOCKET_STATE_CONNECTED) {
        return SOCKET_ERR_NOTCONN;
    }
    
    if (sock->type == SOCK_STREAM) {
        if (sock->tcp_conn != NULL) {
            return tcp_recv(sock->tcp_conn, buf, len);
        }
    }
    
    return SOCKET_ERR_INVALID;
}

/* ==========================================
 * 函数：socket_sendto
 * 功能：发送到指定地址（UDP）
 * ========================================== */
int socket_sendto(int fd, const void *buf, uint32_t len, int flags,
                  const sockaddr_in_t *dest_addr)
{
    if (fd < 0 || fd >= MAX_SOCKETS || buf == NULL || dest_addr == NULL) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    if (sock->type != SOCK_DGRAM) {
        return SOCKET_ERR_INVALID;
    }
    
    net_interface_t *iface = net_get_default_interface();
    if (iface == NULL) {
        return SOCKET_ERR_INVALID;
    }
    
    return udp_send(iface, dest_addr->sin_addr, ntohs(dest_addr->sin_port),
                   sock->local_port, buf, len);
}

/* ==========================================
 * 函数：socket_recvfrom
 * 功能：从指定地址接收（UDP）
 * ========================================== */
int socket_recvfrom(int fd, void *buf, uint32_t len, int flags,
                    sockaddr_in_t *src_addr)
{
    if (fd < 0 || fd >= MAX_SOCKETS || buf == NULL) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    if (sock->type != SOCK_DGRAM) {
        return SOCKET_ERR_INVALID;
    }
    
    /* 简化版：从接收缓冲区读取 */
    if (sock->recv_buf_len == 0) {
        if (sock->non_blocking) {
            return SOCKET_ERR_WOULDBLOCK;
        }
        return 0;
    }
    
    uint32_t copy_len = len;
    if (copy_len > sock->recv_buf_len) {
        copy_len = sock->recv_buf_len;
    }
    
    memcpy(buf, sock->recv_buf + sock->recv_buf_start, copy_len);
    sock->recv_buf_start += copy_len;
    sock->recv_buf_len -= copy_len;
    
    if (sock->recv_buf_len == 0) {
        sock->recv_buf_start = 0;
    }
    
    if (src_addr != NULL) {
        src_addr->sin_family = AF_INET;
        src_addr->sin_port = htons(sock->remote_port);
        src_addr->sin_addr = sock->remote_ip;
    }
    
    return copy_len;
}

/* ==========================================
 * 函数：socket_close
 * 功能：关闭Socket
 * ========================================== */
int socket_close(int fd)
{
    if (fd < 0 || fd >= MAX_SOCKETS) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    if (sock->type == SOCK_STREAM && sock->tcp_conn != NULL) {
        tcp_close(sock->tcp_conn);
        tcp_destroy_connection(sock->tcp_conn);
        sock->tcp_conn = NULL;
    }
    
    if (sock->type == SOCK_DGRAM && sock->local_port != 0) {
        udp_unbind(sock->local_port);
    }
    
    sock->state = SOCKET_STATE_FREE;
    sock->fd = -1;
    
    klog_log("socket", "Closed socket fd=%d", fd);
    
    return SOCKET_OK;
}

/* ==========================================
 * 函数：socket_setsockopt
 * 功能：设置Socket选项
 * ========================================== */
int socket_setsockopt(int fd, int level, int optname,
                      const void *optval, uint32_t optlen)
{
    if (fd < 0 || fd >= MAX_SOCKETS || optval == NULL) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    /* 简化版：只处理部分选项 */
    if (level == SOL_SOCKET) {
        switch (optname) {
            case SO_REUSEADDR:
                /* 允许重用地址 */
                break;
            case SO_KEEPALIVE:
                /* 保持连接 */
                break;
            default:
                break;
        }
    }
    
    return SOCKET_OK;
}

/* ==========================================
 * 函数：socket_set_nonblocking
 * 功能：设置非阻塞模式
 * ========================================== */
int socket_set_nonblocking(int fd, bool non_blocking)
{
    if (fd < 0 || fd >= MAX_SOCKETS) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    sock->non_blocking = non_blocking;
    return SOCKET_OK;
}

/* ==========================================
 * 函数：socket_get_error
 * 功能：获取Socket错误
 * ========================================== */
int socket_get_error(int fd)
{
    if (fd < 0 || fd >= MAX_SOCKETS) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    return sock->error;
}

/* ==========================================
 * 函数：socket_getsockname
 * 功能：获取本地地址
 * ========================================== */
int socket_getsockname(int fd, sockaddr_in_t *addr)
{
    if (fd < 0 || fd >= MAX_SOCKETS || addr == NULL) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    addr->sin_family = AF_INET;
    addr->sin_port = htons(sock->local_port);
    addr->sin_addr = sock->local_ip;
    
    return SOCKET_OK;
}

/* ==========================================
 * 函数：socket_getpeername
 * 功能：获取远端地址
 * ========================================== */
int socket_getpeername(int fd, sockaddr_in_t *addr)
{
    if (fd < 0 || fd >= MAX_SOCKETS || addr == NULL) {
        return SOCKET_ERR_INVALID;
    }
    
    socket_t *sock = &socket_table[fd];
    if (sock->state == SOCKET_STATE_FREE) {
        return SOCKET_ERR_NOSOCK;
    }
    
    if (sock->state != SOCKET_STATE_CONNECTED) {
        return SOCKET_ERR_NOTCONN;
    }
    
    addr->sin_family = AF_INET;
    addr->sin_port = htons(sock->remote_port);
    addr->sin_addr = sock->remote_ip;
    
    return SOCKET_OK;
}
