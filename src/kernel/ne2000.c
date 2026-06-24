/* ==========================================
 * NE2000网卡驱动实现 - ne2000.c
 * 功能：
 *   1. NE2000兼容网卡初始化和配置
 *   2. 数据包发送和接收
 *   3. 中断处理
 * ========================================== */

#include "ne2000.h"
#include "types.h"
#include "isr.h"
#include "pic.h"
#include "heap.h"
#include "string.h"
#include "vga.h"
#include "klog.h"

/* NE2000私有数据 */
static ne2000_data_t ne2000_priv;

/* ==========================================
 * 辅助函数：读取NE2000寄存器
 * ========================================== */
static uint8_t ne_read_reg(uint16_t base, uint8_t reg)
{
    return inb(base + reg);
}

/* ==========================================
 * 辅助函数：写入NE2000寄存器
 * ========================================== */
static void ne_write_reg(uint16_t base, uint8_t reg, uint8_t value)
{
    outb(base + reg, value);
}

/* ==========================================
 * 辅助函数：等待远程DMA完成
 * ========================================== */
static int ne_wait_rdc(uint16_t base)
{
    int timeout = 10000;
    while (timeout--) {
        uint8_t isr = ne_read_reg(base, NE_REG_ISR);
        if (isr & NE_ISR_RDC) {
            ne_write_reg(base, NE_REG_ISR, NE_ISR_RDC);
            return 0;
        }
    }
    return -1;
}

/* ==========================================
 * 函数：ne2000_read_prom
 * 功能：从PROM读取数据（用于获取MAC地址）
 * ========================================== */
static void ne2000_read_prom(uint16_t base, uint8_t offset, uint8_t *data, int len)
{
    /* 切换到页0，远程DMA读 */
    ne_write_reg(base, NE_REG_CMD, NE_CMD_RD2 | NE_CMD_STP);
    
    /* 设置远程DMA地址 */
    ne_write_reg(base, NE_REG_RSAR0, offset * 2);
    ne_write_reg(base, NE_REG_RSAR1, 0);
    
    /* 设置读取字节数 */
    ne_write_reg(base, NE_REG_RBCR0, len * 2);
    ne_write_reg(base, NE_REG_RBCR1, 0);
    
    /* 启动远程读 */
    ne_write_reg(base, NE_REG_CMD, NE_CMD_RD0 | NE_CMD_STP);
    
    /* 读取数据（16位模式，每个字读两次） */
    for (int i = 0; i < len; i++) {
        uint16_t word = inw(base + NE_DATA_PORT);
        data[i] = word & 0xFF;  /* 取低字节 */
        if (i + 1 < len) {
            i++;
            data[i] = (word >> 8) & 0xFF;
        }
    }
    
    /* 等待RDC */
    ne_wait_rdc(base);
}

/* ==========================================
 * 函数：ne2000_init
 * 功能：初始化NE2000网卡
 * ========================================== */
int ne2000_init(net_interface_t *iface)
{
    uint16_t base = NE_IO_BASE;
    
    klog_logf("ne2k", "Initializing NE2000 at I/O 0x%x", base);
    
    /* 初始化私有数据 */
    memset(&ne2000_priv, 0, sizeof(ne2000_priv));
    ne2000_priv.io_base = base;
    ne2000_priv.irq = NE_IRQ;
    
    /* 复位网卡 */
    outb(base + NE_RESET_PORT, inb(base + NE_RESET_PORT));
    
    /* 等待复位完成 */
    int timeout = 1000;
    while (timeout--) {
        uint8_t isr = ne_read_reg(base, NE_REG_ISR);
        if (isr & NE_ISR_RST) {
            break;
        }
    }
    
    if (timeout <= 0) {
        klog_log("ne2k", "Reset timeout");
        return -1;
    }
    
    /* 停止网卡，进入页0 */
    ne_write_reg(base, NE_REG_CMD, NE_CMD_STP | NE_CMD_RD2);
    
    /* 设置为16位模式 */
    ne_write_reg(base, NE_REG_DCR, 0x49);  /* 16位DMA，字节序交换 */
    
    /* 清除所有中断标志 */
    ne_write_reg(base, NE_REG_ISR, 0xFF);
    
    /* 屏蔽所有中断 */
    ne_write_reg(base, NE_REG_IMR, 0x00);
    
    /* 设置接收配置：接受广播和本站 */
    ne_write_reg(base, NE_REG_RCR, 0x04);  /* AB */
    
    /* 设置发送配置 */
    ne_write_reg(base, NE_REG_TCR, 0x00);
    
    /* 读取MAC地址（从PROM） */
    uint8_t prom[32];
    ne2000_read_prom(base, 0, prom, 12);
    
    /* NE2000的PROM中MAC地址在偶数字节位置 */
    for (int i = 0; i < 6; i++) {
        iface->mac.addr[i] = prom[i * 2];
    }
    
    /* 设置物理地址（页1） */
    ne_write_reg(base, NE_REG_CMD, NE_CMD_STP | NE_CMD_RD2 | NE_CMD_PS1);
    for (int i = 0; i < 6; i++) {
        ne_write_reg(base, NE_REG_PAR0 + i, iface->mac.addr[i]);
    }
    
    /* 设置多播地址（全0，不接收多播） */
    for (int i = 0; i < 8; i++) {
        ne_write_reg(base, NE_REG_MAR0 + i, 0x00);
    }
    
    /* 设置接收缓冲区边界 */
    ne_write_reg(base, NE_REG_CMD, NE_CMD_STP | NE_CMD_RD2);
    ne_write_reg(base, NE_REG_PSTART, NE_RX_START);
    ne_write_reg(base, NE_REG_BNRY, NE_RX_START - 1);
    ne_write_reg(base, NE_REG_PSTOP, NE_RX_STOP);
    
    /* 切换到页1，读取当前页指针 */
    ne_write_reg(base, NE_REG_CMD, NE_CMD_STP | NE_CMD_RD2 | NE_CMD_PS1);
    ne2000_priv.curr_page = ne_read_reg(base, NE_REG_CURR);
    if (ne2000_priv.curr_page < NE_RX_START || ne2000_priv.curr_page >= NE_RX_STOP) {
        ne2000_priv.curr_page = NE_RX_START;
    }
    
    /* 启动网卡 */
    ne_write_reg(base, NE_REG_CMD, NE_CMD_RD2 | NE_CMD_STA);
    
    /* 清除中断标志 */
    ne_write_reg(base, NE_REG_ISR, 0xFF);
    
    /* 启用中断：接收、发送、溢出 */
    ne_write_reg(base, NE_REG_IMR, NE_ISR_PRX | NE_ISR_PTX | NE_ISR_RXE | NE_ISR_OVW);
    
    /* 设置接口信息 */
    iface->name[0] = 'e';
    iface->name[1] = 't';
    iface->name[2] = 'h';
    iface->name[3] = '0';
    iface->name[4] = '\0';
    iface->state = NET_IF_UP;
    iface->send = ne2000_send;
    iface->recv = ne2000_recv;
    iface->irq_handler = ne2000_irq_handler;
    iface->driver_data = &ne2000_priv;
    
    ne2000_priv.initialized = true;
    
    klog_logf("ne2k", "NE2000 initialized: MAC=%02x:%02x:%02x:%02x:%02x:%02x",
             mac_format(&iface->mac));
    
    /* 注册IRQ处理程序 */
    isr_register_handler(NE_IRQ + 32, (isr_handler_t)ne2000_irq_handler);
    pic_irq_unmask(NE_IRQ);
    
    return 0;
}

/* ==========================================
 * 函数：ne2000_send
 * 功能：发送数据包
 * ========================================== */
int ne2000_send(net_interface_t *iface, const void *data, uint32_t len)
{
    ne2000_data_t *priv = (ne2000_data_t *)iface->driver_data;
    uint16_t base = priv->io_base;
    
    if (priv->tx_busy) {
        return -1;
    }
    
    if (len > ETH_MAX_FRAME) {
        return -1;
    }
    
    priv->tx_busy = true;
    
    /* 等待发送完成（最多1秒） */
    int timeout = 100000;
    while (timeout--) {
        uint8_t cmd = ne_read_reg(base, NE_REG_CMD);
        if (!(cmd & NE_CMD_TXP)) {
            break;
        }
    }
    
    /* 通过远程DMA写入发送缓冲区 */
    ne_write_reg(base, NE_REG_CMD, NE_CMD_RD2 | NE_CMD_STP);
    ne_write_reg(base, NE_REG_RSAR0, 0);
    ne_write_reg(base, NE_REG_RSAR1, NE_TX_START);
    ne_write_reg(base, NE_REG_RBCR0, len & 0xFF);
    ne_write_reg(base, NE_REG_RBCR1, (len >> 8) & 0xFF);
    ne_write_reg(base, NE_REG_CMD, NE_CMD_RD1 | NE_CMD_STP);
    
    /* 写入数据（16位模式） */
    const uint8_t *p = (const uint8_t *)data;
    for (uint32_t i = 0; i < len; i += 2) {
        uint16_t word;
        if (i + 1 < len) {
            word = p[i] | (p[i + 1] << 8);
        } else {
            word = p[i];
        }
        outw(base + NE_DATA_PORT, word);
    }
    
    /* 等待RDC */
    ne_wait_rdc(base);
    
    /* 设置发送参数 */
    ne_write_reg(base, NE_REG_TPSR, NE_TX_START);
    ne_write_reg(base, NE_REG_TBCR0, len & 0xFF);
    ne_write_reg(base, NE_REG_TBCR1, (len >> 8) & 0xFF);
    
    /* 启动发送 */
    ne_write_reg(base, NE_REG_CMD, NE_CMD_RD2 | NE_CMD_STA | NE_CMD_TXP);
    
    /* 等待发送完成 */
    timeout = 100000;
    while (timeout--) {
        uint8_t isr = ne_read_reg(base, NE_REG_ISR);
        if (isr & NE_ISR_PTX) {
            ne_write_reg(base, NE_REG_ISR, NE_ISR_PTX);
            priv->tx_busy = false;
            priv->tx_count++;
            return 0;
        }
        if (isr & NE_ISR_TXE) {
            ne_write_reg(base, NE_REG_ISR, NE_ISR_TXE);
            priv->tx_busy = false;
            priv->tx_errors++;
            return -1;
        }
    }
    
    priv->tx_busy = false;
    return -1;
}

/* ==========================================
 * 函数：ne2000_recv
 * 功能：接收数据包（轮询模式）
 * ========================================== */
int ne2000_recv(net_interface_t *iface, void *buf, uint32_t len)
{
    ne2000_data_t *priv = (ne2000_data_t *)iface->driver_data;
    uint16_t base = priv->io_base;
    
    /* 读取边界指针 */
    uint8_t bnry = ne_read_reg(base, NE_REG_BNRY);
    
    /* 检查是否有新数据 */
    if (bnry == priv->curr_page - 1 || 
        (priv->curr_page == NE_RX_START && bnry == NE_RX_STOP - 1)) {
        return 0;  /* 没有新数据 */
    }
    
    /* 读取包头（4字节） */
    ne_write_reg(base, NE_REG_CMD, NE_CMD_RD2 | NE_CMD_STP);
    ne_write_reg(base, NE_REG_RSAR0, 0);
    ne_write_reg(base, NE_REG_RSAR1, priv->curr_page);
    ne_write_reg(base, NE_REG_RBCR0, 4);
    ne_write_reg(base, NE_REG_RBCR1, 0);
    ne_write_reg(base, NE_REG_CMD, NE_CMD_RD0 | NE_CMD_STP);
    
    uint8_t hdr[4];
    for (int i = 0; i < 2; i++) {
        uint16_t word = inw(base + NE_DATA_PORT);
        hdr[i * 2] = word & 0xFF;
        hdr[i * 2 + 1] = (word >> 8) & 0xFF;
    }
    
    uint8_t status = hdr[0];
    uint16_t pkt_len = hdr[2] | (hdr[3] << 8);
    
    /* 验证包头 */
    if (!(status & NE_RX_STATUS_PRX) || pkt_len > ETH_MAX_FRAME + 4) {
        /* 包错误，跳过 */
        priv->curr_page++;
        if (priv->curr_page >= NE_RX_STOP) {
            priv->curr_page = NE_RX_START;
        }
        ne_write_reg(base, NE_REG_BNRY, priv->curr_page - 1);
        return -1;
    }
    
    pkt_len -= 4;  /* 减去包头 */
    if (pkt_len > len) {
        pkt_len = len;
    }
    
    /* 读取包数据 */
    ne_write_reg(base, NE_REG_CMD, NE_CMD_RD2 | NE_CMD_STP);
    ne_write_reg(base, NE_REG_RSAR0, 4);
    ne_write_reg(base, NE_REG_RSAR1, priv->curr_page);
    ne_write_reg(base, NE_REG_RBCR0, pkt_len & 0xFF);
    ne_write_reg(base, NE_REG_RBCR1, (pkt_len >> 8) & 0xFF);
    ne_write_reg(base, NE_REG_CMD, NE_CMD_RD0 | NE_CMD_STP);
    
    uint8_t *p = (uint8_t *)buf;
    for (uint32_t i = 0; i < pkt_len; i += 2) {
        uint16_t word = inw(base + NE_DATA_PORT);
        p[i] = word & 0xFF;
        if (i + 1 < pkt_len) {
            p[i + 1] = (word >> 8) & 0xFF;
        }
    }
    
    /* 等待RDC */
    ne_wait_rdc(base);
    
    /* 更新页指针 */
    uint8_t next_page = hdr[1] + 1;
    if (next_page >= NE_RX_STOP) {
        next_page = NE_RX_START;
    }
    priv->curr_page = next_page;
    
    /* 更新边界指针 */
    ne_write_reg(base, NE_REG_BNRY, priv->curr_page - 1);
    
    priv->rx_count++;
    return pkt_len;
}

/* ==========================================
 * 函数：ne2000_irq_handler
 * 功能：NE2000中断处理
 * ========================================== */
void ne2000_irq_handler(net_interface_t *iface)
{
    ne2000_data_t *priv = (ne2000_data_t *)iface->driver_data;
    uint16_t base = priv->io_base;
    
    uint8_t isr = ne_read_reg(base, NE_REG_ISR);
    
    /* 处理接收中断 */
    if (isr & NE_ISR_PRX) {
        ne_write_reg(base, NE_REG_ISR, NE_ISR_PRX);
        
        /* 轮询接收所有包 */
        uint8_t buf[ETH_MAX_FRAME];
        int len;
        while ((len = ne2000_recv(iface, buf, sizeof(buf))) > 0) {
            net_receive_frame(iface, buf, len);
        }
    }
    
    /* 处理发送完成中断 */
    if (isr & NE_ISR_PTX) {
        ne_write_reg(base, NE_REG_ISR, NE_ISR_PTX);
        priv->tx_busy = false;
    }
    
    /* 处理接收错误 */
    if (isr & NE_ISR_RXE) {
        ne_write_reg(base, NE_REG_ISR, NE_ISR_RXE);
        priv->rx_errors++;
    }
    
    /* 处理溢出 */
    if (isr & NE_ISR_OVW) {
        ne_write_reg(base, NE_REG_ISR, NE_ISR_OVW);
        /* 复位接收逻辑 */
        ne_write_reg(base, NE_REG_CMD, NE_CMD_RD2 | NE_CMD_STP);
        ne_write_reg(base, NE_REG_CMD, NE_CMD_RD2 | NE_CMD_STA);
    }
    
    /* 发送EOI */
    pic_send_eoi(NE_IRQ);
}

/* ==========================================
 * 函数：ne2000_get_status
 * 功能：获取NE2000状态信息
 * ========================================== */
void ne2000_get_status(ne2000_data_t *status)
{
    if (status != NULL) {
        memcpy(status, &ne2000_priv, sizeof(ne2000_data_t));
    }
}
