/* ==========================================
 * DHCP客户端实现 - dhcp.c
 * 功能：
 *   1. DHCP发现、请求、确认流程
 *   2. 自动获取IP配置
 *   3. 租约管理
 * ========================================== */

#include "dhcp.h"
#include "network.h"
#include "udp.h"
#include "heap.h"
#include "string.h"
#include "pit.h"
#include "klog.h"
#include "vga.h"

/* DHCP客户端上下文 */
static dhcp_client_t dhcp_ctx;

/* ==========================================
 * 函数：dhcp_add_option
 * 功能：向DHCP消息添加选项
 * ========================================== */
static int dhcp_add_option(uint8_t *options, int offset, uint8_t type,
                           const void *data, uint8_t len)
{
    options[offset++] = type;
    options[offset++] = len;
    if (data != NULL && len > 0) {
        memcpy(options + offset, data, len);
    }
    return offset + len;
}

/* ==========================================
 * 函数：dhcp_build_discover
 * 功能：构建DHCP Discover消息
 * ========================================== */
static int dhcp_build_discover(dhcp_message_t *msg)
{
    memset(msg, 0, sizeof(dhcp_message_t));
    
    msg->op = DHCP_OP_REQUEST;
    msg->htype = DHCP_HTYPE_ETHER;
    msg->hlen = DHCP_HLEN_ETHER;
    msg->hops = 0;
    msg->xid = htonl(dhcp_ctx.xid);
    msg->secs = 0;
    msg->flags = htons(0x8000);  /* 广播标志 */
    msg->ciaddr = 0;
    msg->yiaddr = 0;
    msg->siaddr = 0;
    msg->giaddr = 0;
    
    /* 复制MAC地址 */
    net_interface_t *iface = dhcp_ctx.iface;
    if (iface != NULL) {
        memcpy(msg->chaddr, iface->mac.addr, MAC_ADDR_LEN);
    }
    
    /* 魔术Cookie */
    msg->magic_cookie = htonl(DHCP_MAGIC_COOKIE);
    
    /* 添加选项 */
    int offset = 0;
    uint8_t msg_type = DHCP_DISCOVER;
    offset = dhcp_add_option(msg->options, offset, DHCP_OPT_MSG_TYPE, &msg_type, 1);
    
    /* 参数请求列表 */
    uint8_t param_list[] = {
        DHCP_OPT_SUBNET_MASK,
        DHCP_OPT_ROUTER,
        DHCP_OPT_DNS,
        DHCP_OPT_LEASE_TIME
    };
    offset = dhcp_add_option(msg->options, offset, DHCP_OPT_PARAM_LIST,
                            param_list, sizeof(param_list));
    
    msg->options[offset] = DHCP_OPT_END;
    
    return sizeof(dhcp_message_t);
}

/* ==========================================
 * 函数：dhcp_build_request
 * 功能：构建DHCP Request消息
 * ========================================== */
static int dhcp_build_request(dhcp_message_t *msg, ip_addr_t requested_ip,
                              ip_addr_t server_ip)
{
    memset(msg, 0, sizeof(dhcp_message_t));
    
    msg->op = DHCP_OP_REQUEST;
    msg->htype = DHCP_HTYPE_ETHER;
    msg->hlen = DHCP_HLEN_ETHER;
    msg->hops = 0;
    msg->xid = htonl(dhcp_ctx.xid);
    msg->secs = 0;
    msg->flags = htons(0x8000);
    msg->ciaddr = 0;
    msg->yiaddr = 0;
    msg->siaddr = 0;
    msg->giaddr = 0;
    
    net_interface_t *iface = dhcp_ctx.iface;
    if (iface != NULL) {
        memcpy(msg->chaddr, iface->mac.addr, MAC_ADDR_LEN);
    }
    
    msg->magic_cookie = htonl(DHCP_MAGIC_COOKIE);
    
    int offset = 0;
    uint8_t msg_type = DHCP_REQUEST;
    offset = dhcp_add_option(msg->options, offset, DHCP_OPT_MSG_TYPE, &msg_type, 1);
    
    /* 请求的IP */
    offset = dhcp_add_option(msg->options, offset, DHCP_OPT_REQUESTED_IP,
                            &requested_ip, 4);
    
    /* 服务器ID */
    offset = dhcp_add_option(msg->options, offset, DHCP_OPT_SERVER_ID,
                            &server_ip, 4);
    
    msg->options[offset] = DHCP_OPT_END;
    
    return sizeof(dhcp_message_t);
}

/* ==========================================
 * 函数：dhcp_parse_offer
 * 功能：解析DHCP Offer
 * ========================================== */
static int dhcp_parse_offer(const dhcp_message_t *msg, dhcp_lease_t *lease)
{
    if (msg->op != DHCP_OP_REPLY) {
        return -1;
    }
    
    /* 检查事务ID */
    if (ntohl(msg->xid) != dhcp_ctx.xid) {
        return -1;
    }
    
    /* 检查消息类型 */
    const uint8_t *options = msg->options;
    int offset = 0;
    uint8_t msg_type = 0;
    
    while (options[offset] != DHCP_OPT_END && offset < 308) {
        if (options[offset] == DHCP_OPT_PAD) {
            offset++;
            continue;
        }
        
        uint8_t type = options[offset++];
        uint8_t len = options[offset++];
        
        switch (type) {
            case DHCP_OPT_MSG_TYPE:
                msg_type = options[offset];
                break;
            case DHCP_OPT_SUBNET_MASK:
                if (len == 4) {
                    memcpy(&lease->netmask, options + offset, 4);
                }
                break;
            case DHCP_OPT_ROUTER:
                if (len >= 4) {
                    memcpy(&lease->gateway, options + offset, 4);
                }
                break;
            case DHCP_OPT_DNS:
                if (len >= 4) {
                    memcpy(&lease->dns, options + offset, 4);
                }
                break;
            case DHCP_OPT_LEASE_TIME:
                if (len == 4) {
                    uint32_t time;
                    memcpy(&time, options + offset, 4);
                    lease->lease_time = ntohl(time);
                }
                break;
            case DHCP_OPT_SERVER_ID:
                if (len == 4) {
                    memcpy(&lease->server_ip, options + offset, 4);
                }
                break;
        }
        
        offset += len;
    }
    
    if (msg_type != DHCP_OFFER) {
        return -1;
    }
    
    lease->ip = msg->yiaddr;
    
    return 0;
}

/* ==========================================
 * 函数：dhcp_parse_ack
 * 功能：解析DHCP ACK
 * ========================================== */
static int dhcp_parse_ack(const dhcp_message_t *msg, dhcp_lease_t *lease)
{
    if (msg->op != DHCP_OP_REPLY) {
        return -1;
    }
    
    if (ntohl(msg->xid) != dhcp_ctx.xid) {
        return -1;
    }
    
    const uint8_t *options = msg->options;
    int offset = 0;
    uint8_t msg_type = 0;
    
    while (options[offset] != DHCP_OPT_END && offset < 308) {
        if (options[offset] == DHCP_OPT_PAD) {
            offset++;
            continue;
        }
        
        uint8_t type = options[offset++];
        uint8_t len = options[offset++];
        
        switch (type) {
            case DHCP_OPT_MSG_TYPE:
                msg_type = options[offset];
                break;
            case DHCP_OPT_SUBNET_MASK:
                if (len == 4) {
                    memcpy(&lease->netmask, options + offset, 4);
                }
                break;
            case DHCP_OPT_ROUTER:
                if (len >= 4) {
                    memcpy(&lease->gateway, options + offset, 4);
                }
                break;
            case DHCP_OPT_DNS:
                if (len >= 4) {
                    memcpy(&lease->dns, options + offset, 4);
                }
                break;
            case DHCP_OPT_LEASE_TIME:
                if (len == 4) {
                    uint32_t time;
                    memcpy(&time, options + offset, 4);
                    lease->lease_time = ntohl(time);
                }
                break;
            case DHCP_OPT_RENEWAL_TIME:
                if (len == 4) {
                    uint32_t time;
                    memcpy(&time, options + offset, 4);
                    lease->renewal_time = ntohl(time);
                }
                break;
            case DHCP_OPT_REBIND_TIME:
                if (len == 4) {
                    uint32_t time;
                    memcpy(&time, options + offset, 4);
                    lease->rebind_time = ntohl(time);
                }
                break;
        }
        
        offset += len;
    }
    
    if (msg_type != DHCP_ACK) {
        return -1;
    }
    
    lease->ip = msg->yiaddr;
    lease->start_time = pit_get_ticks();
    
    return 0;
}

/* ==========================================
 * 函数：dhcp_udp_callback
 * 功能：DHCP UDP回调
 * ========================================== */
static void dhcp_udp_callback(net_interface_t *iface, ip_addr_t src_ip,
                              uint16_t src_port, const void *data, uint32_t len)
{
    if (len < sizeof(dhcp_message_t)) {
        return;
    }
    
    const dhcp_message_t *msg = (const dhcp_message_t *)data;
    
    switch (dhcp_ctx.state) {
        case DHCP_STATE_SELECTING:
            /* 收到Offer */
            if (dhcp_parse_offer(msg, &dhcp_ctx.lease) == 0) {
                dhcp_ctx.state = DHCP_STATE_REQUESTING;
                
                /* 发送Request */
                dhcp_message_t request;
                dhcp_build_request(&request, dhcp_ctx.lease.ip,
                                  dhcp_ctx.lease.server_ip);
                
                udp_send(iface, IP_BROADCAST, DHCP_SERVER_PORT,
                        DHCP_CLIENT_PORT, &request, sizeof(request));
            }
            break;
            
        case DHCP_STATE_REQUESTING:
            /* 收到ACK */
            if (dhcp_parse_ack(msg, &dhcp_ctx.lease) == 0) {
                dhcp_ctx.state = DHCP_STATE_BOUND;
                
                /* 更新网络接口配置 */
                if (iface != NULL) {
                    iface->ip = dhcp_ctx.lease.ip;
                    iface->netmask = dhcp_ctx.lease.netmask;
                    iface->gateway = dhcp_ctx.lease.gateway;
                    iface->dns = dhcp_ctx.lease.dns;
                }
                
                char ip_buf[16];
                ip_format(dhcp_ctx.lease.ip, ip_buf);
                klog_log("dhcp", "Got IP: %s", ip_buf);
            }
            break;
            
        default:
            break;
    }
}

/* ==========================================
 * 函数：dhcp_init
 * 功能：初始化DHCP客户端
 * ========================================== */
void dhcp_init(net_interface_t *iface)
{
    memset(&dhcp_ctx, 0, sizeof(dhcp_ctx));
    dhcp_ctx.iface = iface;
    dhcp_ctx.state = DHCP_STATE_INIT;
    dhcp_ctx.xid = pit_get_ticks();  /* 用时间戳作为事务ID */
    
    /* 注册UDP监听 */
    udp_bind(DHCP_CLIENT_PORT, dhcp_udp_callback);
    
    klog_log("dhcp", "DHCP client initialized");
}

/* ==========================================
 * 函数：dhcp_start
 * 功能：开始DHCP获取IP
 * ========================================== */
int dhcp_start(void)
{
    if (dhcp_ctx.iface == NULL) {
        return -1;
    }
    
    /* 生成新的事务ID */
    dhcp_ctx.xid++;
    
    /* 发送Discover */
    dhcp_message_t discover;
    dhcp_build_discover(&discover);
    
    net_interface_t *iface = dhcp_ctx.iface;
    int ret = udp_send(iface, IP_BROADCAST, DHCP_SERVER_PORT,
                      DHCP_CLIENT_PORT, &discover, sizeof(discover));
    
    if (ret == 0) {
        dhcp_ctx.state = DHCP_STATE_SELECTING;
        dhcp_ctx.timeout = pit_get_ticks() + DHCP_TIMEOUT_MS / 10;
        
        char ip_buf[16];
        ip_format(iface->ip, ip_buf);
        klog_log("dhcp", "Sending DHCP Discover...");
    }
    
    return ret;
}

/* ==========================================
 * 函数：dhcp_release
 * 功能：释放租约
 * ========================================== */
int dhcp_release(void)
{
    if (dhcp_ctx.state != DHCP_STATE_BOUND) {
        return -1;
    }
    
    /* 发送Release消息 */
    dhcp_message_t msg;
    memset(&msg, 0, sizeof(msg));
    
    msg.op = DHCP_OP_REQUEST;
    msg.htype = DHCP_HTYPE_ETHER;
    msg.hlen = DHCP_HLEN_ETHER;
    msg.xid = htonl(dhcp_ctx.xid);
    msg.ciaddr = dhcp_ctx.lease.ip;
    
    net_interface_t *iface = dhcp_ctx.iface;
    if (iface != NULL) {
        memcpy(msg.chaddr, iface->mac.addr, MAC_ADDR_LEN);
    }
    
    msg.magic_cookie = htonl(DHCP_MAGIC_COOKIE);
    
    int offset = 0;
    uint8_t msg_type = DHCP_RELEASE;
    offset = dhcp_add_option(msg.options, offset, DHCP_OPT_MSG_TYPE, &msg_type, 1);
    offset = dhcp_add_option(msg.options, offset, DHCP_OPT_SERVER_ID,
                            &dhcp_ctx.lease.server_ip, 4);
    msg.options[offset] = DHCP_OPT_END;
    
    udp_send(iface, dhcp_ctx.lease.server_ip, DHCP_SERVER_PORT,
            DHCP_CLIENT_PORT, &msg, sizeof(msg));
    
    dhcp_ctx.state = DHCP_STATE_INIT;
    
    /* 清除接口配置 */
    if (iface != NULL) {
        iface->ip = 0;
        iface->netmask = 0;
        iface->gateway = 0;
        iface->dns = 0;
    }
    
    klog_log("dhcp", "Released IP address");
    
    return 0;
}

/* ==========================================
 * 函数：dhcp_renew
 * 功能：续约租约
 * ========================================== */
int dhcp_renew(void)
{
    if (dhcp_ctx.state != DHCP_STATE_BOUND) {
        return -1;
    }
    
    dhcp_ctx.state = DHCP_STATE_RENEWING;
    return dhcp_start();
}

/* ==========================================
 * 函数：dhcp_get_state
 * 功能：获取DHCP状态
 * ========================================== */
dhcp_state_t dhcp_get_state(void)
{
    return dhcp_ctx.state;
}

/* ==========================================
 * 函数：dhcp_get_lease
 * 功能：获取租约信息
 * ========================================== */
dhcp_lease_t *dhcp_get_lease(void)
{
    return &dhcp_ctx.lease;
}

/* ==========================================
 * 函数：dhcp_timer_tick
 * 功能：DHCP定时器处理
 * ========================================== */
void dhcp_timer_tick(void)
{
    if (dhcp_ctx.state == DHCP_STATE_BOUND) {
        /* 检查是否需要续约 */
        uint32_t now = pit_get_ticks();
        uint32_t elapsed = (now - dhcp_ctx.lease.start_time) / 100;  /* 秒 */
        
        if (elapsed >= dhcp_ctx.lease.renewal_time) {
            dhcp_renew();
        }
    }
}

/* ==========================================
 * 函数：dhcp_show_status
 * 功能：显示DHCP状态
 * ========================================== */
void dhcp_show_status(void)
{
    char ip_buf[16];
    
    const char *state_names[] = {
        "INIT", "SELECTING", "REQUESTING",
        "BOUND", "RENEWING", "REBINDING"
    };
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("DHCP Status: ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_printf("%s\n", state_names[dhcp_ctx.state]);
    
    if (dhcp_ctx.state == DHCP_STATE_BOUND) {
        ip_format(dhcp_ctx.lease.ip, ip_buf);
        vga_printf("  IP Address:    %s\n", ip_buf);
        
        ip_format(dhcp_ctx.lease.netmask, ip_buf);
        vga_printf("  Subnet Mask:   %s\n", ip_buf);
        
        ip_format(dhcp_ctx.lease.gateway, ip_buf);
        vga_printf("  Gateway:       %s\n", ip_buf);
        
        ip_format(dhcp_ctx.lease.dns, ip_buf);
        vga_printf("  DNS Server:    %s\n", ip_buf);
        
        vga_printf("  Lease Time:    %d seconds\n", dhcp_ctx.lease.lease_time);
    }
}
