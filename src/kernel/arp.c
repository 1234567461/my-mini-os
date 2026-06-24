/* ==========================================
 * ARP协议实现 - arp.c
 * 功能：
 *   1. ARP缓存管理
 *   2. ARP请求/响应处理
 *   3. IP到MAC地址解析
 * ========================================== */

#include "arp.h"
#include "network.h"
#include "heap.h"
#include "string.h"
#include "pit.h"
#include "klog.h"
#include "vga.h"

/* ARP缓存表 */
static arp_entry_t arp_cache[ARP_CACHE_SIZE];

/* ==========================================
 * 函数：arp_init
 * 功能：初始化ARP子系统
 * ========================================== */
void arp_init(void)
{
    /* 清空ARP缓存 */
    memset(arp_cache, 0, sizeof(arp_cache));
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].state = ARP_ENTRY_FREE;
    }
    
    klog_log("arp", "ARP subsystem initialized");
}

/* ==========================================
 * 函数：arp_cache_find
 * 功能：在缓存中查找IP对应的条目
 * ========================================== */
static arp_entry_t *arp_cache_find(ip_addr_t ip)
{
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].state != ARP_ENTRY_FREE && arp_cache[i].ip == ip) {
            return &arp_cache[i];
        }
    }
    return NULL;
}

/* ==========================================
 * 函数：arp_cache_find_free
 * 功能：查找空闲的缓存条目
 * ========================================== */
static arp_entry_t *arp_cache_find_free(void)
{
    /* 先找完全空闲的 */
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].state == ARP_ENTRY_FREE) {
            return &arp_cache[i];
        }
    }
    
    /* 没有空闲的，找最老的 */
    arp_entry_t *oldest = &arp_cache[0];
    for (int i = 1; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].timestamp < oldest->timestamp) {
            oldest = &arp_cache[i];
        }
    }
    return oldest;
}

/* ==========================================
 * 函数：arp_cache_update
 * 功能：添加或更新ARP缓存条目
 * ========================================== */
void arp_cache_update(ip_addr_t ip, const mac_addr_t *mac)
{
    arp_entry_t *entry = arp_cache_find(ip);
    
    if (entry != NULL) {
        /* 更新现有条目 */
        memcpy(&entry->mac, mac, sizeof(mac_addr_t));
        entry->state = ARP_ENTRY_VALID;
        entry->timestamp = pit_get_ticks();
    } else {
        /* 创建新条目 */
        entry = arp_cache_find_free();
        if (entry != NULL) {
            entry->ip = ip;
            memcpy(&entry->mac, mac, sizeof(mac_addr_t));
            entry->state = ARP_ENTRY_VALID;
            entry->timestamp = pit_get_ticks();
            entry->retry_count = 0;
        }
    }
}

/* ==========================================
 * 函数：arp_cache_remove
 * 功能：删除ARP缓存条目
 * ========================================== */
void arp_cache_remove(ip_addr_t ip)
{
    arp_entry_t *entry = arp_cache_find(ip);
    if (entry != NULL) {
        entry->state = ARP_ENTRY_FREE;
    }
}

/* ==========================================
 * 函数：arp_cache_clear
 * 功能：清空ARP缓存
 * ========================================== */
void arp_cache_clear(void)
{
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].state = ARP_ENTRY_FREE;
    }
}

/* ==========================================
 * 函数：arp_send_request
 * 功能：发送ARP请求
 * ========================================== */
int arp_send_request(net_interface_t *iface, ip_addr_t target_ip)
{
    if (iface == NULL) {
        return -1;
    }
    
    /* 构建ARP请求包 */
    arp_packet_t arp;
    arp.hw_type = htons(ARP_HW_ETHER);
    arp.proto_type = htons(ETH_TYPE_IP);
    arp.hw_len = MAC_ADDR_LEN;
    arp.proto_len = 4;
    arp.opcode = htons(ARP_OP_REQUEST);
    
    memcpy(&arp.sender_mac, &iface->mac, MAC_ADDR_LEN);
    arp.sender_ip = iface->ip;
    
    memset(&arp.target_mac, 0, MAC_ADDR_LEN);
    arp.target_ip = target_ip;
    
    /* 发送到广播地址 */
    return net_send_frame(iface, &MAC_BROADCAST, ETH_TYPE_ARP, &arp, sizeof(arp));
}

/* ==========================================
 * 函数：arp_send_reply
 * 功能：发送ARP响应
 * ========================================== */
int arp_send_reply(net_interface_t *iface, ip_addr_t target_ip, const mac_addr_t *target_mac)
{
    if (iface == NULL || target_mac == NULL) {
        return -1;
    }
    
    /* 构建ARP响应包 */
    arp_packet_t arp;
    arp.hw_type = htons(ARP_HW_ETHER);
    arp.proto_type = htons(ETH_TYPE_IP);
    arp.hw_len = MAC_ADDR_LEN;
    arp.proto_len = 4;
    arp.opcode = htons(ARP_OP_REPLY);
    
    memcpy(&arp.sender_mac, &iface->mac, MAC_ADDR_LEN);
    arp.sender_ip = iface->ip;
    memcpy(&arp.target_mac, target_mac, MAC_ADDR_LEN);
    arp.target_ip = target_ip;
    
    /* 发送到目标MAC */
    return net_send_frame(iface, target_mac, ETH_TYPE_ARP, &arp, sizeof(arp));
}

/* ==========================================
 * 函数：arp_receive
 * 功能：处理收到的ARP包
 * ========================================== */
void arp_receive(net_interface_t *iface, const arp_packet_t *arp, uint32_t len)
{
    if (iface == NULL || arp == NULL || len < sizeof(arp_packet_t)) {
        return;
    }
    
    uint16_t opcode = ntohs(arp->opcode);
    
    /* 更新缓存：记录发送者的IP-MAC映射 */
    arp_cache_update(arp->sender_ip, &arp->sender_mac);
    
    /* 处理ARP请求 */
    if (opcode == ARP_OP_REQUEST) {
        /* 如果请求的是我们的IP，发送响应 */
        if (arp->target_ip == iface->ip) {
            arp_send_reply(iface, arp->sender_ip, &arp->sender_mac);
        }
    }
    
    /* ARP响应已经在arp_cache_update中处理了 */
}

/* ==========================================
 * 函数：arp_resolve
 * 功能：解析IP地址到MAC地址
 * 参数：
 *   iface - 网络接口
 *   ip - 目标IP地址
 *   mac - 输出MAC地址
 * 返回：0=成功，-1=失败
 * ========================================== */
int arp_resolve(net_interface_t *iface, ip_addr_t ip, mac_addr_t *mac)
{
    if (iface == NULL || mac == NULL) {
        return -1;
    }
    
    /* 检查是否为目标广播地址 */
    if (ip == IP_BROADCAST) {
        memcpy(mac, &MAC_BROADCAST, sizeof(mac_addr_t));
        return 0;
    }
    
    /* 检查是否在本地子网 */
    ip_addr_t target_net = ip & iface->netmask;
    ip_addr_t local_net = iface->ip & iface->netmask;
    
    ip_addr_t resolve_ip = ip;
    if (target_net != local_net) {
        /* 不在本地子网，解析网关 */
        resolve_ip = iface->gateway;
    }
    
    /* 查找缓存 */
    arp_entry_t *entry = arp_cache_find(resolve_ip);
    if (entry != NULL && entry->state == ARP_ENTRY_VALID) {
        memcpy(mac, &entry->mac, sizeof(mac_addr_t));
        return 0;
    }
    
    /* 缓存未命中，发送ARP请求 */
    if (entry == NULL) {
        /* 创建待解析条目 */
        entry = arp_cache_find_free();
        if (entry != NULL) {
            entry->ip = resolve_ip;
            entry->state = ARP_ENTRY_PENDING;
            entry->timestamp = pit_get_ticks();
            entry->retry_count = 0;
        }
    }
    
    /* 发送ARP请求 */
    arp_send_request(iface, resolve_ip);
    
    if (entry != NULL) {
        entry->retry_count++;
    }
    
    /* 简化版：不等待响应，直接返回失败
     * 实际应该等待响应或异步处理 */
    return -1;
}

/* ==========================================
 * 函数：arp_cache_show
 * 功能：显示ARP缓存表
 * ========================================== */
void arp_cache_show(void)
{
    char ip_buf[16];
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("ARP Cache:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    int count = 0;
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].state != ARP_ENTRY_FREE) {
            ip_format(arp_cache[i].ip, ip_buf);
            vga_printf("  %s -> %02x:%02x:%02x:%02x:%02x:%02x [%s]\n",
                      ip_buf,
                      mac_format(&arp_cache[i].mac),
                      arp_cache[i].state == ARP_ENTRY_VALID ? "valid" : "pending");
            count++;
        }
    }
    
    if (count == 0) {
        vga_puts("  (empty)\n");
    }
}

/* ==========================================
 * 函数：arp_timeout_check
 * 功能：检查ARP缓存超时
 * ========================================== */
void arp_timeout_check(void)
{
    uint32_t now = pit_get_ticks();
    uint32_t timeout_ticks = ARP_TIMEOUT * 100;  /* 假设100Hz */
    
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].state != ARP_ENTRY_FREE) {
            if (now - arp_cache[i].timestamp > timeout_ticks) {
                arp_cache[i].state = ARP_ENTRY_FREE;
            }
        }
    }
}
