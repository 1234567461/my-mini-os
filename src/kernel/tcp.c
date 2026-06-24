/* ==========================================
 * TCP协议实现 - tcp.c
 * 功能：
 *   1. TCP连接管理（三次握手/四次挥手）
 *   2. TCP数据可靠传输
 *   3. TCP状态机
 * ========================================== */

#include "tcp.h"
#include "network.h"
#include "ip.h"
#include "heap.h"
#include "string.h"
#include "pit.h"
#include "klog.h"

/* TCP连接池 */
static tcp_connection_t tcp_connections[TCP_MAX_CONNECTIONS];

/* ==========================================
 * 函数：tcp_init
 * 功能：初始化TCP子系统
 * ========================================== */
void tcp_init(void)
{
    /* 清空连接池 */
    for (int i = 0; i < TCP_MAX_CONNECTIONS; i++) {
        tcp_connections[i].active = false;
        tcp_connections[i].state = TCP_CLOSED;
    }
    
    klog_log("tcp", "TCP subsystem initialized");
}

/* ==========================================
 * 函数：tcp_checksum
 * 功能：计算TCP校验和
 * ========================================== */
uint16_t tcp_checksum(ip_addr_t src_ip, ip_addr_t dst_ip,
                      const void *data, uint32_t len)
{
    uint32_t sum = 0;
    
    /* 伪首部 */
    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += dst_ip & 0xFFFF;
    sum += htons(IP_PROTO_TCP);
    sum += htons(len);
    
    /* TCP数据 */
    const uint16_t *p = (const uint16_t *)data;
    uint32_t remaining = len;
    
    while (remaining > 1) {
        sum += *p++;
        remaining -= 2;
    }
    
    if (remaining > 0) {
        sum += *(const uint8_t *)p;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    uint16_t result = ~sum;
    return result == 0 ? 0xFFFF : result;
}

/* ==========================================
 * 函数：tcp_create_connection
 * 功能：创建TCP连接
 * ========================================== */
tcp_connection_t *tcp_create_connection(void)
{
    for (int i = 0; i < TCP_MAX_CONNECTIONS; i++) {
        if (!tcp_connections[i].active) {
            tcp_connection_t *conn = &tcp_connections[i];
            memset(conn, 0, sizeof(tcp_connection_t));
            conn->active = true;
            conn->state = TCP_CLOSED;
            conn->mss = TCP_MAX_PAYLOAD;
            conn->recv_window = TCP_RECV_BUF_SIZE;
            conn->send_window = TCP_MAX_PAYLOAD;
            conn->rto = 1000;  /* 初始RTO: 1秒 */
            return conn;
        }
    }
    return NULL;
}

/* ==========================================
 * 函数：tcp_destroy_connection
 * 功能：销毁TCP连接
 * ========================================== */
void tcp_destroy_connection(tcp_connection_t *conn)
{
    if (conn != NULL) {
        conn->active = false;
        conn->state = TCP_CLOSED;
    }
}

/* ==========================================
 * 函数：tcp_state_name
 * 功能：获取状态名称
 * ========================================== */
const char *tcp_state_name(tcp_state_t state)
{
    switch (state) {
        case TCP_CLOSED:      return "CLOSED";
        case TCP_LISTEN:      return "LISTEN";
        case TCP_SYN_SENT:    return "SYN_SENT";
        case TCP_SYN_RECEIVED:return "SYN_RECEIVED";
        case TCP_ESTABLISHED: return "ESTABLISHED";
        case TCP_FIN_WAIT_1:  return "FIN_WAIT_1";
        case TCP_FIN_WAIT_2:  return "FIN_WAIT_2";
        case TCP_CLOSE_WAIT:  return "CLOSE_WAIT";
        case TCP_CLOSING:     return "CLOSING";
        case TCP_LAST_ACK:    return "LAST_ACK";
        case TCP_TIME_WAIT:   return "TIME_WAIT";
        default:              return "UNKNOWN";
    }
}

/* ==========================================
 * 函数：tcp_send_segment
 * 功能：发送TCP段
 * ========================================== */
static int tcp_send_segment(net_interface_t *iface, tcp_connection_t *conn,
                           uint8_t flags, const void *data, uint32_t data_len)
{
    if (iface == NULL || conn == NULL) {
        return -1;
    }
    
    /* 分配TCP段缓冲区 */
    uint32_t total_len = TCP_HEADER_SIZE + data_len;
    uint8_t *segment = (uint8_t *)kmalloc(total_len);
    if (segment == NULL) {
        return -1;
    }
    
    /* 构建TCP头部 */
    tcp_header_t *tcp = (tcp_header_t *)segment;
    tcp->src_port = htons(conn->local_port);
    tcp->dst_port = htons(conn->remote_port);
    tcp->seq_num = htonl(conn->send_next);
    tcp->ack_num = htonl(conn->recv_next);
    tcp->data_offset = (TCP_HEADER_SIZE / 4) << 4;
    tcp->flags = flags;
    tcp->window = htons(conn->recv_window);
    tcp->checksum = 0;
    tcp->urgent_ptr = 0;
    
    /* 复制数据 */
    if (data != NULL && data_len > 0) {
        memcpy(segment + TCP_HEADER_SIZE, data, data_len);
    }
    
    /* 计算校验和 */
    tcp->checksum = tcp_checksum(conn->local_ip, conn->remote_ip, segment, total_len);
    
    /* 发送IP包 */
    int ret = ip_send(iface, conn->remote_ip, IP_PROTO_TCP, segment, total_len);
    
    kfree(segment);
    
    if (ret == 0) {
        /* 更新序列号 */
        conn->send_next += data_len;
        if (flags & TCP_FLAG_SYN) conn->send_next++;
        if (flags & TCP_FLAG_FIN) conn->send_next++;
        conn->last_send_time = pit_get_ticks();
    }
    
    return ret;
}

/* ==========================================
 * 函数：tcp_connect
 * 功能：主动打开连接（客户端）
 * ========================================== */
int tcp_connect(tcp_connection_t *conn, ip_addr_t remote_ip, uint16_t remote_port)
{
    if (conn == NULL) {
        return -1;
    }
    
    net_interface_t *iface = net_get_default_interface();
    if (iface == NULL) {
        return -1;
    }
    
    conn->remote_ip = remote_ip;
    conn->remote_port = remote_port;
    conn->local_ip = iface->ip;
    
    /* 分配本地端口 */
    static uint16_t next_port = 49152;
    conn->local_port = next_port++;
    if (next_port == 0) next_port = 49152;
    
    /* 初始化序列号 */
    conn->send_next = pit_get_ticks();  /* 简单用时间戳作为ISN */
    conn->send_unack = conn->send_next;
    conn->recv_next = 0;
    
    /* 发送SYN */
    conn->state = TCP_SYN_SENT;
    int ret = tcp_send_segment(iface, conn, TCP_FLAG_SYN, NULL, 0);
    
    if (ret != 0) {
        conn->state = TCP_CLOSED;
        return -1;
    }
    
    /* 简化版：等待SYN-ACK */
    /* 实际应该异步处理 */
    uint32_t timeout = pit_get_ticks() + 500;  /* 5秒超时 */
    while (conn->state == TCP_SYN_SENT && pit_get_ticks() < timeout) {
        /* 等待处理 */
    }
    
    if (conn->state != TCP_ESTABLISHED) {
        conn->state = TCP_CLOSED;
        return -1;
    }
    
    return 0;
}

/* ==========================================
 * 函数：tcp_listen
 * 功能：被动打开连接（服务器）
 * ========================================== */
int tcp_listen(tcp_connection_t *conn, uint16_t local_port)
{
    if (conn == NULL) {
        return -1;
    }
    
    net_interface_t *iface = net_get_default_interface();
    if (iface == NULL) {
        return -1;
    }
    
    conn->local_ip = iface->ip;
    conn->local_port = local_port;
    conn->state = TCP_LISTEN;
    
    return 0;
}

/* ==========================================
 * 函数：tcp_send
 * 功能：发送数据
 * ========================================== */
int tcp_send(tcp_connection_t *conn, const void *data, uint32_t len)
{
    if (conn == NULL || data == NULL) {
        return -1;
    }
    
    if (conn->state != TCP_ESTABLISHED) {
        return -1;
    }
    
    net_interface_t *iface = net_get_default_interface();
    if (iface == NULL) {
        return -1;
    }
    
    /* 分段发送 */
    const uint8_t *p = (const uint8_t *)data;
    uint32_t remaining = len;
    
    while (remaining > 0) {
        uint32_t seg_len = remaining;
        if (seg_len > conn->mss) {
            seg_len = conn->mss;
        }
        if (seg_len > conn->send_window) {
            seg_len = conn->send_window;
        }
        if (seg_len == 0) {
            break;
        }
        
        int ret = tcp_send_segment(iface, conn, TCP_FLAG_ACK | TCP_FLAG_PSH,
                                   p, seg_len);
        if (ret != 0) {
            return -1;
        }
        
        p += seg_len;
        remaining -= seg_len;
    }
    
    return len - remaining;
}

/* ==========================================
 * 函数：tcp_recv
 * 功能：接收数据
 * ========================================== */
int tcp_recv(tcp_connection_t *conn, void *buf, uint32_t len)
{
    if (conn == NULL || buf == NULL) {
        return -1;
    }
    
    if (conn->recv_buf_len == 0) {
        return 0;
    }
    
    uint32_t copy_len = len;
    if (copy_len > conn->recv_buf_len) {
        copy_len = conn->recv_buf_len;
    }
    
    memcpy(buf, conn->recv_buf, copy_len);
    
    /* 移动缓冲区 */
    if (copy_len < conn->recv_buf_len) {
        memcpy(conn->recv_buf, conn->recv_buf + copy_len,
               conn->recv_buf_len - copy_len);
    }
    conn->recv_buf_len -= copy_len;
    conn->recv_window = TCP_RECV_BUF_SIZE - conn->recv_buf_len;
    
    return copy_len;
}

/* ==========================================
 * 函数：tcp_close
 * 功能：关闭连接
 * ========================================== */
int tcp_close(tcp_connection_t *conn)
{
    if (conn == NULL) {
        return -1;
    }
    
    if (conn->state == TCP_ESTABLISHED) {
        net_interface_t *iface = net_get_default_interface();
        if (iface != NULL) {
            /* 发送FIN */
            conn->state = TCP_FIN_WAIT_1;
            tcp_send_segment(iface, conn, TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
        }
    } else {
        conn->state = TCP_CLOSED;
    }
    
    return 0;
}

/* ==========================================
 * 函数：tcp_receive
 * 功能：处理收到的TCP包
 * ========================================== */
void tcp_receive(net_interface_t *iface, ip_addr_t src_ip, ip_addr_t dst_ip,
                 const void *data, uint32_t len)
{
    if (iface == NULL || data == NULL || len < TCP_HEADER_SIZE) {
        return;
    }
    
    const tcp_header_t *tcp = (const tcp_header_t *)data;
    
    /* 验证校验和 */
    if (tcp_checksum(src_ip, dst_ip, data, len) != 0) {
        return;
    }
    
    /* 提取头部长度 */
    uint8_t header_len = (tcp->data_offset >> 4) * 4;
    if (header_len < TCP_HEADER_SIZE || header_len > len) {
        return;
    }
    
    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dst_port = ntohs(tcp->dst_port);
    uint32_t seq = ntohl(tcp->seq_num);
    uint32_t ack = ntohl(tcp->ack_num);
    uint8_t flags = tcp->flags;
    
    const uint8_t *payload = (const uint8_t *)data + header_len;
    uint32_t payload_len = len - header_len;
    
    /* 查找匹配的连接 */
    tcp_connection_t *conn = NULL;
    for (int i = 0; i < TCP_MAX_CONNECTIONS; i++) {
        if (!tcp_connections[i].active) continue;
        
        tcp_connection_t *c = &tcp_connections[i];
        if (c->local_port == dst_port &&
            c->remote_port == src_port &&
            c->remote_ip == src_ip) {
            conn = c;
            break;
        }
    }
    
    /* 监听状态的连接 */
    if (conn == NULL) {
        for (int i = 0; i < TCP_MAX_CONNECTIONS; i++) {
            if (tcp_connections[i].active &&
                tcp_connections[i].state == TCP_LISTEN &&
                tcp_connections[i].local_port == dst_port) {
                conn = &tcp_connections[i];
                break;
            }
        }
    }
    
    if (conn == NULL) {
        /* 没有匹配的连接，发送RST */
        return;
    }
    
    /* TCP状态机 */
    switch (conn->state) {
        case TCP_LISTEN:
            if (flags & TCP_FLAG_SYN) {
                /* 收到SYN，发送SYN-ACK */
                conn->remote_ip = src_ip;
                conn->remote_port = src_port;
                conn->recv_next = seq + 1;
                conn->send_next = pit_get_ticks();
                conn->state = TCP_SYN_RECEIVED;
                tcp_send_segment(iface, conn, TCP_FLAG_SYN | TCP_FLAG_ACK, NULL, 0);
            }
            break;
            
        case TCP_SYN_SENT:
            if ((flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) == (TCP_FLAG_SYN | TCP_FLAG_ACK)) {
                /* 收到SYN-ACK，发送ACK */
                conn->recv_next = seq + 1;
                conn->send_unack = ack;
                conn->state = TCP_ESTABLISHED;
                tcp_send_segment(iface, conn, TCP_FLAG_ACK, NULL, 0);
            }
            break;
            
        case TCP_SYN_RECEIVED:
            if (flags & TCP_FLAG_ACK) {
                /* 收到ACK，连接建立 */
                conn->send_unack = ack;
                conn->state = TCP_ESTABLISHED;
            }
            break;
            
        case TCP_ESTABLISHED:
            if (flags & TCP_FLAG_ACK) {
                conn->send_unack = ack;
            }
            
            if (payload_len > 0) {
                /* 接收数据 */
                if (seq == conn->recv_next) {
                    /* 按序到达 */
                    uint32_t space = TCP_RECV_BUF_SIZE - conn->recv_buf_len;
                    uint32_t copy_len = payload_len;
                    if (copy_len > space) copy_len = space;
                    
                    memcpy(conn->recv_buf + conn->recv_buf_len, payload, copy_len);
                    conn->recv_buf_len += copy_len;
                    conn->recv_next += copy_len;
                    
                    /* 发送ACK */
                    tcp_send_segment(iface, conn, TCP_FLAG_ACK, NULL, 0);
                }
            }
            
            if (flags & TCP_FLAG_FIN) {
                conn->recv_next++;
                conn->state = TCP_CLOSE_WAIT;
                tcp_send_segment(iface, conn, TCP_FLAG_ACK, NULL, 0);
            }
            break;
            
        case TCP_FIN_WAIT_1:
            if (flags & TCP_FLAG_ACK) {
                conn->state = TCP_FIN_WAIT_2;
            }
            if (flags & TCP_FLAG_FIN) {
                conn->recv_next++;
                conn->state = TCP_TIME_WAIT;
                tcp_send_segment(iface, conn, TCP_FLAG_ACK, NULL, 0);
            }
            break;
            
        case TCP_FIN_WAIT_2:
            if (flags & TCP_FLAG_FIN) {
                conn->recv_next++;
                conn->state = TCP_TIME_WAIT;
                tcp_send_segment(iface, conn, TCP_FLAG_ACK, NULL, 0);
            }
            break;
            
        case TCP_CLOSE_WAIT:
            /* 等待应用层关闭 */
            break;
            
        case TCP_LAST_ACK:
            if (flags & TCP_FLAG_ACK) {
                conn->state = TCP_CLOSED;
                conn->active = false;
            }
            break;
            
        default:
            break;
    }
}

/* ==========================================
 * 函数：tcp_timer_tick
 * 功能：TCP定时器处理
 * ========================================== */
void tcp_timer_tick(void)
{
    net_interface_t *iface = net_get_default_interface();
    if (iface == NULL) return;
    
    for (int i = 0; i < TCP_MAX_CONNECTIONS; i++) {
        tcp_connection_t *conn = &tcp_connections[i];
        if (!conn->active) continue;
        
        /* 检查重传超时 */
        if (conn->state == TCP_SYN_SENT ||
            conn->state == TCP_SYN_RECEIVED ||
            conn->state == TCP_ESTABLISHED) {
            
            if (conn->send_unack != conn->send_next) {
                /* 有未确认的数据 */
                uint32_t now = pit_get_ticks();
                if (now - conn->last_send_time > conn->rto) {
                    /* 超时，重传 */
                    conn->retransmit_count++;
                    if (conn->retransmit_count > 5) {
                        /* 重传次数过多，关闭连接 */
                        conn->state = TCP_CLOSED;
                        conn->active = false;
                    } else {
                        /* 指数退避 */
                        conn->rto *= 2;
                    }
                }
            }
        }
    }
}
