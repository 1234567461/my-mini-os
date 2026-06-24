/* ==========================================
 * UDP协议实现 - udp.c
 * 功能：
 *   1. UDP数据包封装/解封装
 *   2. UDP端口管理
 *   3. UDP校验和计算
 * ========================================== */

#include "udp.h"
#include "network.h"
#include "ip.h"
#include "heap.h"
#include "string.h"
#include "klog.h"

/* UDP端口监听表 */
static udp_port_entry_t udp_ports[UDP_MAX_PORTS];

/* ==========================================
 * 函数：udp_init
 * 功能：初始化UDP子系统
 * ========================================== */
void udp_init(void)
{
    /* 清空端口表 */
    for (int i = 0; i < UDP_MAX_PORTS; i++) {
        udp_ports[i].active = false;
    }
    
    klog_log("udp", "UDP subsystem initialized");
}

/* ==========================================
 * 函数：udp_checksum
 * 功能：计算UDP校验和
 * ========================================== */
uint16_t udp_checksum(ip_addr_t src_ip, ip_addr_t dst_ip,
                      const void *data, uint32_t len)
{
    uint32_t sum = 0;
    
    /* 计算伪首部 */
    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += dst_ip & 0xFFFF;
    sum += htons(IP_PROTO_UDP);
    sum += htons(len);
    
    /* 计算UDP数据 */
    const uint16_t *p = (const uint16_t *)data;
    uint32_t remaining = len;
    
    while (remaining > 1) {
        sum += *p++;
        remaining -= 2;
    }
    
    if (remaining > 0) {
        sum += *(const uint8_t *)p;
    }
    
    /* 折叠 */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    uint16_t result = ~sum;
    return result == 0 ? 0xFFFF : result;
}

/* ==========================================
 * 函数：udp_bind
 * 功能：注册UDP端口监听
 * ========================================== */
int udp_bind(uint16_t port, udp_callback_t callback)
{
    if (callback == NULL) {
        return -1;
    }
    
    /* 检查端口是否已被绑定 */
    for (int i = 0; i < UDP_MAX_PORTS; i++) {
        if (udp_ports[i].active && udp_ports[i].port == port) {
            return -1;  /* 端口已被占用 */
        }
    }
    
    /* 查找空闲槽位 */
    for (int i = 0; i < UDP_MAX_PORTS; i++) {
        if (!udp_ports[i].active) {
            udp_ports[i].port = port;
            udp_ports[i].callback = callback;
            udp_ports[i].active = true;
            return 0;
        }
    }
    
    return -1;  /* 没有空闲槽位 */
}

/* ==========================================
 * 函数：udp_unbind
 * 功能：解除UDP端口监听
 * ========================================== */
int udp_unbind(uint16_t port)
{
    for (int i = 0; i < UDP_MAX_PORTS; i++) {
        if (udp_ports[i].active && udp_ports[i].port == port) {
            udp_ports[i].active = false;
            udp_ports[i].callback = NULL;
            return 0;
        }
    }
    
    return -1;  /* 端口未绑定 */
}

/* ==========================================
 * 函数：udp_send
 * 功能：发送UDP数据包
 * ========================================== */
int udp_send(net_interface_t *iface, ip_addr_t dst_ip, uint16_t dst_port,
             uint16_t src_port, const void *data, uint32_t len)
{
    if (iface == NULL || data == NULL) {
        return -1;
    }
    
    if (len > UDP_MAX_PAYLOAD) {
        return -1;
    }
    
    /* 分配UDP包缓冲区 */
    uint32_t total_len = UDP_HEADER_SIZE + len;
    uint8_t *packet = (uint8_t *)kmalloc(total_len);
    if (packet == NULL) {
        return -1;
    }
    
    /* 构建UDP头部 */
    udp_header_t *udp = (udp_header_t *)packet;
    udp->src_port = htons(src_port);
    udp->dst_port = htons(dst_port);
    udp->length = htons(total_len);
    udp->checksum = 0;
    
    /* 复制数据 */
    memcpy(packet + UDP_HEADER_SIZE, data, len);
    
    /* 计算校验和 */
    udp->checksum = udp_checksum(iface->ip, dst_ip, packet, total_len);
    
    /* 发送IP包 */
    int ret = ip_send(iface, dst_ip, IP_PROTO_UDP, packet, total_len);
    
    kfree(packet);
    return ret;
}

/* ==========================================
 * 函数：udp_receive
 * 功能：处理收到的UDP包
 * ========================================== */
void udp_receive(net_interface_t *iface, ip_addr_t src_ip, ip_addr_t dst_ip,
                 const void *data, uint32_t len)
{
    if (iface == NULL || data == NULL || len < UDP_HEADER_SIZE) {
        return;
    }
    
    const udp_header_t *udp = (const udp_header_t *)data;
    
    /* 验证长度 */
    uint16_t udp_len = ntohs(udp->length);
    if (udp_len < UDP_HEADER_SIZE || udp_len > len) {
        return;
    }
    
    /* 验证校验和（如果非零） */
    if (udp->checksum != 0) {
        if (udp_checksum(src_ip, dst_ip, data, udp_len) != 0) {
            return;  /* 校验和错误 */
        }
    }
    
    /* 提取端口和数据 */
    uint16_t src_port = ntohs(udp->src_port);
    uint16_t dst_port = ntohs(udp->dst_port);
    const uint8_t *payload = (const uint8_t *)data + UDP_HEADER_SIZE;
    uint32_t payload_len = udp_len - UDP_HEADER_SIZE;
    
    /* 查找匹配的监听端口 */
    for (int i = 0; i < UDP_MAX_PORTS; i++) {
        if (udp_ports[i].active && udp_ports[i].port == dst_port) {
            /* 调用回调函数 */
            udp_ports[i].callback(iface, src_ip, src_port, payload, payload_len);
            return;
        }
    }
    
    /* 没有监听该端口，发送端口不可达 */
    /* 简化版：忽略 */
}
