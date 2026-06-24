/* ==========================================
 * 网络基础实现 - network.c
 * 功能：
 *   1. 网络子系统初始化
 *   2. 网络接口管理
 *   3. 以太网帧收发
 *   4. IP地址工具函数
 * ========================================== */

#include "network.h"
#include "heap.h"
#include "string.h"
#include "vga.h"
#include "klog.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"
#include "socket.h"
#include "ne2000.h"

/* 默认网络接口 */
static net_interface_t *default_iface = NULL;

/* 广播MAC地址 */
const mac_addr_t MAC_BROADCAST = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

/* ==========================================
 * 函数：network_init
 * 功能：初始化网络子系统
 * ========================================== */
void network_init(void)
{
    klog_log("net", "Initializing network subsystem");
    
    /* 初始化各协议层 */
    arp_init();
    ip_init();
    icmp_init();
    udp_init();
    tcp_init();
    socket_init();
    
    klog_log("net", "Network protocol stack initialized");
}

/* ==========================================
 * 函数：net_register_interface
 * 功能：注册网络接口
 * ========================================== */
int net_register_interface(net_interface_t *iface)
{
    if (iface == NULL) {
        return -1;
    }
    
    /* 如果还没有默认接口，设置为默认 */
    if (default_iface == NULL) {
        default_iface = iface;
        klog_log("net", "Set default interface: %s", iface->name);
    }
    
    return 0;
}

/* ==========================================
 * 函数：net_get_default_interface
 * 功能：获取默认网络接口
 * ========================================== */
net_interface_t *net_get_default_interface(void)
{
    return default_iface;
}

/* ==========================================
 * 函数：net_send_frame
 * 功能：发送以太网帧
 * 参数：
 *   iface - 网络接口
 *   dest - 目标MAC地址
 *   type - 帧类型（如ETH_TYPE_IP）
 *   payload - 载荷数据
 *   len - 载荷长度
 * ========================================== */
int net_send_frame(net_interface_t *iface, const mac_addr_t *dest,
                   uint16_t type, const void *payload, uint32_t len)
{
    if (iface == NULL || dest == NULL || payload == NULL) {
        return -1;
    }
    
    if (len > ETH_MTU) {
        return -1;
    }
    
    /* 分配缓冲区 */
    uint8_t *frame = (uint8_t *)kmalloc(ETH_HEADER_SIZE + len);
    if (frame == NULL) {
        return -1;
    }
    
    /* 构建以太网帧头 */
    eth_header_t *eth = (eth_header_t *)frame;
    memcpy(&eth->dest, dest, MAC_ADDR_LEN);
    memcpy(&eth->src, &iface->mac, MAC_ADDR_LEN);
    eth->type = htons(type);
    
    /* 复制载荷 */
    memcpy(frame + ETH_HEADER_SIZE, payload, len);
    
    /* 通过驱动发送 */
    int ret = iface->send(iface, frame, ETH_HEADER_SIZE + len);
    
    kfree(frame);
    
    if (ret == 0) {
        iface->tx_packets++;
        iface->tx_bytes += ETH_HEADER_SIZE + len;
    } else {
        iface->tx_errors++;
    }
    
    return ret;
}

/* ==========================================
 * 函数：net_receive_frame
 * 功能：接收并分发以太网帧
 * 参数：
 *   iface - 网络接口
 *   data - 帧数据
 *   len - 帧长度
 * ========================================== */
void net_receive_frame(net_interface_t *iface, const void *data, uint32_t len)
{
    if (iface == NULL || data == NULL || len < ETH_HEADER_SIZE) {
        return;
    }
    
    const eth_header_t *eth = (const eth_header_t *)data;
    uint16_t type = ntohs(eth->type);
    const uint8_t *payload = (const uint8_t *)data + ETH_HEADER_SIZE;
    uint32_t payload_len = len - ETH_HEADER_SIZE;
    
    /* 更新统计 */
    iface->rx_packets++;
    iface->rx_bytes += len;
    
    /* 根据帧类型分发 */
    switch (type) {
        case ETH_TYPE_ARP:
            arp_receive(iface, (const arp_packet_t *)payload, payload_len);
            break;
            
        case ETH_TYPE_IP:
            ip_receive(iface, payload, payload_len);
            break;
            
        default:
            /* 未知类型，忽略 */
            break;
    }
}

/* ==========================================
 * 函数：ip_parse
 * 功能：解析IP地址字符串为二进制格式
 * 参数：
 *   str - IP地址字符串，如"192.168.1.1"
 * 返回：IP地址（网络字节序）
 * ========================================== */
ip_addr_t ip_parse(const char *str)
{
    if (str == NULL) {
        return 0;
    }
    
    uint32_t ip = 0;
    int parts[4] = {0};
    int part_idx = 0;
    
    for (int i = 0; str[i] != '\0' && part_idx < 4; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            parts[part_idx] = parts[part_idx] * 10 + (str[i] - '0');
        } else if (str[i] == '.') {
            if (parts[part_idx] > 255) {
                return 0;  /* 无效IP */
            }
            part_idx++;
        } else {
            return 0;  /* 无效字符 */
        }
    }
    
    if (part_idx != 3 || parts[3] > 255) {
        return 0;
    }
    
    ip = IP_ADDR(parts[0], parts[1], parts[2], parts[3]);
    return htonl(ip);
}

/* ==========================================
 * 函数：ip_format
 * 功能：将IP地址格式化为字符串
 * 参数：
 *   ip - IP地址（网络字节序）
 *   buf - 输出缓冲区（至少16字节）
 * ========================================== */
void ip_format(ip_addr_t ip, char *buf)
{
    if (buf == NULL) {
        return;
    }
    
    ip_addr_t host_ip = ntohl(ip);
    int a = (host_ip >> 24) & 0xFF;
    int b = (host_ip >> 16) & 0xFF;
    int c = (host_ip >> 8) & 0xFF;
    int d = host_ip & 0xFF;
    
    /* 简单实现：手动转换数字为字符串 */
    char *p = buf;
    
    /* 转换第一个字节 */
    if (a >= 100) { *p++ = '0' + a / 100; a %= 100; }
    if (a >= 10) { *p++ = '0' + a / 10; a %= 10; }
    *p++ = '0' + a;
    *p++ = '.';
    
    /* 转换第二个字节 */
    if (b >= 100) { *p++ = '0' + b / 100; b %= 100; }
    if (b >= 10) { *p++ = '0' + b / 10; b %= 10; }
    *p++ = '0' + b;
    *p++ = '.';
    
    /* 转换第三个字节 */
    if (c >= 100) { *p++ = '0' + c / 100; c %= 100; }
    if (c >= 10) { *p++ = '0' + c / 10; c %= 10; }
    *p++ = '0' + c;
    *p++ = '.';
    
    /* 转换第四个字节 */
    if (d >= 100) { *p++ = '0' + d / 100; d %= 100; }
    if (d >= 10) { *p++ = '0' + d / 10; d %= 10; }
    *p++ = '0' + d;
    *p = '\0';
}
