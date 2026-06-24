/* ==========================================
 * IP协议实现 - ip.c
 * 功能：
 *   1. IP数据包封装/解封装
 *   2. IP校验和计算
 *   3. IP路由决策
 * ========================================== */

#include "ip.h"
#include "network.h"
#include "arp.h"
#include "icmp.h"
#include "tcp.h"
#include "udp.h"
#include "heap.h"
#include "string.h"
#include "klog.h"

/* IP包标识符计数器 */
static uint16_t ip_id_counter = 0;

/* ==========================================
 * 函数：ip_init
 * 功能：初始化IP子系统
 * ========================================== */
void ip_init(void)
{
    ip_id_counter = 0;
    klog_log("ip", "IP subsystem initialized");
}

/* ==========================================
 * 函数：ip_next_id
 * 功能：获取下一个IP包标识符
 * ========================================== */
uint16_t ip_next_id(void)
{
    return ip_id_counter++;
}

/* ==========================================
 * 函数：ip_checksum
 * 功能：计算IP校验和
 * 参数：
 *   data - 数据
 *   len - 数据长度
 * 返回：校验和
 * ========================================== */
uint16_t ip_checksum(const void *data, uint32_t len)
{
    const uint16_t *p = (const uint16_t *)data;
    uint32_t sum = 0;
    
    /* 累加16位字 */
    while (len > 1) {
        sum += *p++;
        len -= 2;
    }
    
    /* 处理奇数字节 */
    if (len > 0) {
        sum += *(const uint8_t *)p;
    }
    
    /* 折叠32位和为16位 */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

/* ==========================================
 * 函数：ip_pseudo_checksum
 * 功能：计算伪首部（用于TCP/UDP校验和）
 * ========================================== */
uint16_t ip_pseudo_checksum(ip_addr_t src, ip_addr_t dst,
                            uint8_t protocol, uint16_t length)
{
    uint32_t sum = 0;
    
    /* 源IP */
    sum += (src >> 16) & 0xFFFF;
    sum += src & 0xFFFF;
    
    /* 目标IP */
    sum += (dst >> 16) & 0xFFFF;
    sum += dst & 0xFFFF;
    
    /* 协议和长度 */
    sum += htons(protocol);
    sum += htons(length);
    
    /* 折叠 */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return sum;
}

/* ==========================================
 * 函数：ip_is_local
 * 功能：判断IP是否在本地子网
 * ========================================== */
bool ip_is_local(ip_addr_t ip)
{
    net_interface_t *iface = net_get_default_interface();
    if (iface == NULL) {
        return false;
    }
    
    return (ip & iface->netmask) == (iface->ip & iface->netmask);
}

/* ==========================================
 * 函数：ip_is_mine
 * 功能：判断是否为本机IP
 * ========================================== */
bool ip_is_mine(ip_addr_t ip)
{
    net_interface_t *iface = net_get_default_interface();
    if (iface == NULL) {
        return false;
    }
    
    return ip == iface->ip || ip == IP_BROADCAST;
}

/* ==========================================
 * 函数：ip_send
 * 功能：发送IP数据包
 * 参数：
 *   iface - 网络接口
 *   dst_ip - 目标IP
 *   protocol - 上层协议（TCP/UDP/ICMP）
 *   payload - 载荷数据
 *   payload_len - 载荷长度
 * ========================================== */
int ip_send(net_interface_t *iface, ip_addr_t dst_ip, uint8_t protocol,
            const void *payload, uint32_t payload_len)
{
    if (iface == NULL || payload == NULL) {
        return -1;
    }
    
    /* 分配IP包缓冲区 */
    uint32_t total_len = IP_HEADER_LEN + payload_len;
    uint8_t *packet = (uint8_t *)kmalloc(total_len);
    if (packet == NULL) {
        return -1;
    }
    
    /* 构建IP头部 */
    ip_header_t *ip = (ip_header_t *)packet;
    ip->ver_ihl = (IP_VERSION << 4) | 5;  /* 版本4，头部长度20字节 */
    ip->tos = 0;
    ip->total_len = htons(total_len);
    ip->id = htons(ip_next_id());
    ip->flags_frag = htons(IP_FLAG_DF);  /* 不分片 */
    ip->ttl = IP_DEFAULT_TTL;
    ip->protocol = protocol;
    ip->checksum = 0;
    ip->src_ip = iface->ip;
    ip->dst_ip = dst_ip;
    
    /* 计算IP头部校验和 */
    ip->checksum = ip_checksum(ip, IP_HEADER_LEN);
    
    /* 复制载荷 */
    memcpy(packet + IP_HEADER_LEN, payload, payload_len);
    
    /* 解析目标MAC地址 */
    mac_addr_t dest_mac;
    if (arp_resolve(iface, dst_ip, &dest_mac) != 0) {
        /* ARP解析失败，尝试使用网关 */
        if (!ip_is_local(dst_ip) && iface->gateway != 0) {
            if (arp_resolve(iface, iface->gateway, &dest_mac) != 0) {
                kfree(packet);
                return -1;
            }
        } else {
            kfree(packet);
            return -1;
        }
    }
    
    /* 发送以太网帧 */
    int ret = net_send_frame(iface, &dest_mac, ETH_TYPE_IP, packet, total_len);
    
    kfree(packet);
    return ret;
}

/* ==========================================
 * 函数：ip_receive
 * 功能：处理收到的IP包
 * 参数：
 *   iface - 网络接口
 *   data - IP包数据
 *   len - 数据长度
 * ========================================== */
void ip_receive(net_interface_t *iface, const void *data, uint32_t len)
{
    if (iface == NULL || data == NULL || len < IP_HEADER_LEN) {
        return;
    }
    
    const ip_header_t *ip = (const ip_header_t *)data;
    
    /* 验证IP版本 */
    uint8_t version = (ip->ver_ihl >> 4) & 0x0F;
    if (version != IP_VERSION) {
        return;
    }
    
    /* 验证头部长度 */
    uint8_t ihl = ip->ver_ihl & 0x0F;
    uint32_t header_len = ihl * 4;
    if (header_len < IP_HEADER_LEN || header_len > len) {
        return;
    }
    
    /* 验证总长度 */
    uint16_t total_len = ntohs(ip->total_len);
    if (total_len > len) {
        return;
    }
    
    /* 验证校验和 */
    if (ip_checksum(ip, header_len) != 0) {
        return;
    }
    
    /* 检查是否为本机IP */
    if (!ip_is_mine(ip->dst_ip)) {
        /* 不是发给我们的，忽略（简化版：不支持路由） */
        return;
    }
    
    /* 根据协议分发 */
    const uint8_t *payload = (const uint8_t *)data + header_len;
    uint32_t payload_len = total_len - header_len;
    
    switch (ip->protocol) {
        case IP_PROTO_ICMP:
            icmp_receive(iface, ip->src_ip, payload, payload_len);
            break;
            
        case IP_PROTO_TCP:
            tcp_receive(iface, ip->src_ip, ip->dst_ip, payload, payload_len);
            break;
            
        case IP_PROTO_UDP:
            udp_receive(iface, ip->src_ip, ip->dst_ip, payload, payload_len);
            break;
            
        default:
            /* 未知协议，发送目标不可达 */
            icmp_send_dest_unreach(iface, ip->src_ip, data, total_len);
            break;
    }
}
