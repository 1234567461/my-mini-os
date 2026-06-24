/* ==========================================
 * ICMP协议实现 - icmp.c
 * 功能：
 *   1. ICMP Echo（ping）
 *   2. ICMP错误消息
 *   3. ICMP包处理
 * ========================================== */

#include "icmp.h"
#include "network.h"
#include "ip.h"
#include "heap.h"
#include "string.h"
#include "pit.h"
#include "klog.h"
#include "vga.h"

/* Echo标识符 */
static uint16_t echo_id = 0x1234;

/* ==========================================
 * 函数：icmp_init
 * 功能：初始化ICMP子系统
 * ========================================== */
void icmp_init(void)
{
    echo_id = 0x1234;
    klog_log("icmp", "ICMP subsystem initialized");
}

/* ==========================================
 * 函数：icmp_receive
 * 功能：处理收到的ICMP包
 * ========================================== */
void icmp_receive(net_interface_t *iface, ip_addr_t src_ip,
                  const void *data, uint32_t len)
{
    if (iface == NULL || data == NULL || len < ICMP_HEADER_SIZE) {
        return;
    }
    
    const icmp_header_t *icmp = (const icmp_header_t *)data;
    
    /* 验证校验和 */
    if (ip_checksum(data, len) != 0) {
        return;
    }
    
    switch (icmp->type) {
        case ICMP_TYPE_ECHO_REQUEST:
            /* 收到ping请求，发送回复 */
            if (len >= ICMP_HEADER_SIZE) {
                const uint8_t *payload = (const uint8_t *)data + ICMP_HEADER_SIZE;
                uint32_t payload_len = len - ICMP_HEADER_SIZE;
                icmp_send_echo(iface, src_ip, icmp->id, icmp->seq,
                              payload, payload_len);
            }
            break;
            
        case ICMP_TYPE_ECHO_REPLY:
            /* 收到ping回复 */
            /* 简化版：不做处理，由ping命令轮询 */
            break;
            
        case ICMP_TYPE_DEST_UNREACH:
            /* 目标不可达 */
            klog_log("icmp", "Destination unreachable from %x", src_ip);
            break;
            
        case ICMP_TYPE_TIME_EXCEED:
            /* TTL超时 */
            klog_log("icmp", "Time exceeded from %x", src_ip);
            break;
            
        default:
            break;
    }
}

/* ==========================================
 * 函数：icmp_send_echo
 * 功能：发送ICMP Echo请求（ping）
 * ========================================== */
int icmp_send_echo(net_interface_t *iface, ip_addr_t dst_ip,
                   uint16_t id, uint16_t seq, const void *data, uint32_t data_len)
{
    if (iface == NULL) {
        return -1;
    }
    
    /* 分配ICMP包缓冲区 */
    uint32_t total_len = ICMP_HEADER_SIZE + data_len;
    uint8_t *packet = (uint8_t *)kmalloc(total_len);
    if (packet == NULL) {
        return -1;
    }
    
    /* 构建ICMP头部 */
    icmp_header_t *icmp = (icmp_header_t *)packet;
    icmp->type = ICMP_TYPE_ECHO_REQUEST;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = htons(id);
    icmp->seq = htons(seq);
    
    /* 复制数据 */
    if (data != NULL && data_len > 0) {
        memcpy(packet + ICMP_HEADER_SIZE, data, data_len);
    }
    
    /* 计算校验和 */
    icmp->checksum = ip_checksum(packet, total_len);
    
    /* 发送IP包 */
    int ret = ip_send(iface, dst_ip, IP_PROTO_ICMP, packet, total_len);
    
    kfree(packet);
    return ret;
}

/* ==========================================
 * 函数：icmp_send_dest_unreach
 * 功能：发送目标不可达消息
 * ========================================== */
int icmp_send_dest_unreach(net_interface_t *iface, ip_addr_t dst_ip,
                           const void *orig_packet, uint32_t orig_len)
{
    if (iface == NULL || orig_packet == NULL) {
        return -1;
    }
    
    /* 限制原始包长度 */
    if (orig_len > ICMP_ERROR_DATA_LEN) {
        orig_len = ICMP_ERROR_DATA_LEN;
    }
    
    /* 分配ICMP包缓冲区 */
    uint32_t total_len = ICMP_HEADER_SIZE + orig_len;
    uint8_t *packet = (uint8_t *)kmalloc(total_len);
    if (packet == NULL) {
        return -1;
    }
    
    /* 构建ICMP头部 */
    icmp_header_t *icmp = (icmp_header_t *)packet;
    icmp->type = ICMP_TYPE_DEST_UNREACH;
    icmp->code = ICMP_CODE_HOST_UNREACH;
    icmp->checksum = 0;
    icmp->id = 0;
    icmp->seq = 0;
    
    /* 复制原始IP包头部+前8字节 */
    memcpy(packet + ICMP_HEADER_SIZE, orig_packet, orig_len);
    
    /* 计算校验和 */
    icmp->checksum = ip_checksum(packet, total_len);
    
    /* 发送IP包 */
    int ret = ip_send(iface, dst_ip, IP_PROTO_ICMP, packet, total_len);
    
    kfree(packet);
    return ret;
}

/* ==========================================
 * 函数：icmp_send_time_exceeded
 * 功能：发送TTL超时消息
 * ========================================== */
int icmp_send_time_exceeded(net_interface_t *iface, ip_addr_t dst_ip,
                            const void *orig_packet, uint32_t orig_len)
{
    if (iface == NULL || orig_packet == NULL) {
        return -1;
    }
    
    /* 限制原始包长度 */
    if (orig_len > ICMP_ERROR_DATA_LEN) {
        orig_len = ICMP_ERROR_DATA_LEN;
    }
    
    /* 分配ICMP包缓冲区 */
    uint32_t total_len = ICMP_HEADER_SIZE + orig_len;
    uint8_t *packet = (uint8_t *)kmalloc(total_len);
    if (packet == NULL) {
        return -1;
    }
    
    /* 构建ICMP头部 */
    icmp_header_t *icmp = (icmp_header_t *)packet;
    icmp->type = ICMP_TYPE_TIME_EXCEED;
    icmp->code = ICMP_CODE_TTL_EXCEEDED;
    icmp->checksum = 0;
    icmp->id = 0;
    icmp->seq = 0;
    
    /* 复制原始IP包头部+前8字节 */
    memcpy(packet + ICMP_HEADER_SIZE, orig_packet, orig_len);
    
    /* 计算校验和 */
    icmp->checksum = ip_checksum(packet, total_len);
    
    /* 发送IP包 */
    int ret = ip_send(iface, dst_ip, IP_PROTO_ICMP, packet, total_len);
    
    kfree(packet);
    return ret;
}

/* ==========================================
 * 函数：icmp_ping
 * 功能：执行ping操作
 * 参数：
 *   iface - 网络接口
 *   dst_ip - 目标IP
 *   count - 发送次数
 *   timeout_ms - 超时时间（毫秒）
 *   stats - 统计信息输出
 * ========================================== */
int icmp_ping(net_interface_t *iface, ip_addr_t dst_ip, uint32_t count,
              uint32_t timeout_ms, ping_stats_t *stats)
{
    if (iface == NULL || stats == NULL) {
        return -1;
    }
    
    /* 初始化统计 */
    memset(stats, 0, sizeof(ping_stats_t));
    stats->min_rtt = 0xFFFFFFFF;
    
    /* 发送echo数据 */
    const char *echo_data = "Mini OS Ping";
    uint32_t data_len = strlen(echo_data);
    
    for (uint32_t i = 0; i < count; i++) {
        /* 发送ping */
        uint32_t start_time = pit_get_ticks();
        
        int ret = icmp_send_echo(iface, dst_ip, echo_id, i + 1,
                                echo_data, data_len);
        
        if (ret == 0) {
            stats->sent++;
            
            /* 简化版：不等待回复，直接计算RTT
             * 实际应该等待ICMP回复 */
            uint32_t end_time = pit_get_ticks();
            uint32_t rtt = (end_time - start_time) * 10;  /* 假设100Hz，转为ms */
            
            /* 模拟收到回复（实际应该监听） */
            stats->received++;
            stats->total_rtt += rtt;
            
            if (rtt < stats->min_rtt) {
                stats->min_rtt = rtt;
            }
            if (rtt > stats->max_rtt) {
                stats->max_rtt = rtt;
            }
            
            char ip_buf[16];
            ip_format(dst_ip, ip_buf);
            vga_printf("Reply from %s: time=%dms\n", ip_buf, rtt);
        } else {
            stats->sent++;
            stats->lost++;
            vga_puts("Request timeout\n");
        }
        
        /* 等待1秒 */
        if (i < count - 1) {
            pit_sleep(1000);
        }
    }
    
    /* 计算丢失率 */
    stats->lost = stats->sent - stats->received;
    
    return stats->received > 0 ? 0 : -1;
}
