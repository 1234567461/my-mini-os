/* ==========================================
 * NE2000网卡驱动头文件 - ne2000.h
 * 功能：
 *   1. NE2000兼容网卡驱动接口
 *   2. 支持QEMU的ne2k_pci和ne2k_isa
 * ========================================== */
#ifndef NE2000_H
#define NE2000_H

#include "network.h"

/* ==========================================
 * NE2000寄存器（基于基地址偏移）
 * NE2000有两页寄存器，通过CMD寄存器切换
 * ========================================== */

/* 命令寄存器（所有页共用） */
#define NE_REG_CMD      0x00

/* 页0寄存器 */
#define NE_REG_CLDAL0   0x01  /* 当前本地DMA地址低字节 */
#define NE_REG_CLDAL1   0x02  /* 当前本地DMA地址高字节 */
#define NE_REG_PSTART   0x01  /* 页起始地址（写） */
#define NE_REG_PSTOP    0x02  /* 页停止地址（写） */
#define NE_REG_BNRY     0x03  /* 边界指针 */
#define NE_REG_TSR      0x04  /* 发送状态寄存器（读） */
#define NE_REG_TPSR     0x04  /* 发送页起始地址（写） */
#define NE_REG_TBCR0    0x05  /* 发送字节计数低字节 */
#define NE_REG_TBCR1    0x06  /* 发送字节计数高字节 */
#define NE_REG_ISR      0x07  /* 中断状态寄存器 */
#define NE_REG_RSAR0    0x08  /* 远程DMA地址低字节 */
#define NE_REG_RSAR1    0x09  /* 远程DMA地址高字节 */
#define NE_REG_RBCR0    0x0A  /* 远程字节计数低字节 */
#define NE_REG_RBCR1    0x0B  /* 远程字节计数高字节 */
#define NE_REG_RCR      0x0C  /* 接收配置寄存器 */
#define NE_REG_TCR      0x0D  /* 发送配置寄存器 */
#define NE_REG_DCR      0x0E  /* 数据配置寄存器 */
#define NE_REG_IMR      0x0F  /* 中断屏蔽寄存器 */

/* 页1寄存器 */
#define NE_REG_PAR0     0x01  /* 物理地址寄存器0-5 */
#define NE_REG_PAR1     0x02
#define NE_REG_PAR2     0x03
#define NE_REG_PAR3     0x04
#define NE_REG_PAR4     0x05
#define NE_REG_PAR5     0x06
#define NE_REG_CURR     0x07  /* 当前页寄存器 */
#define NE_REG_MAR0     0x08  /* 多播地址寄存器0-7 */

/* CMD寄存器位定义 */
#define NE_CMD_STP      0x01  /* 停止命令 */
#define NE_CMD_STA      0x02  /* 启动命令 */
#define NE_CMD_TXP      0x04  /* 发送包 */
#define NE_CMD_RD0      0x00  /* 远程DMA命令位0 */
#define NE_CMD_RD1      0x08  /* 远程DMA命令位1 */
#define NE_CMD_RD2      0x10  /* 远程DMA命令位2 */
#define NE_CMD_PS0      0x00  /* 页选择位0 */
#define NE_CMD_PS1      0x40  /* 页选择位1 */
#define NE_CMD_PAGE0    (NE_CMD_STP | NE_CMD_RD2)   /* 页0，停止，远程DMA */
#define NE_CMD_PAGE1    (NE_CMD_STP | NE_CMD_RD2 | NE_CMD_PS1)  /* 页1 */

/* ISR位定义 */
#define NE_ISR_PRX      0x01  /* 包接收成功 */
#define NE_ISR_PTX      0x02  /* 包发送成功 */
#define NE_ISR_RXE      0x04  /* 接收错误 */
#define NE_ISR_TXE      0x08  /* 发送错误 */
#define NE_ISR_OVW      0x10  /* 溢出 */
#define NE_ISR_CNT      0x20  /* 计数器溢出 */
#define NE_ISR_RDC      0x40  /* 远程DMA完成 */
#define NE_ISR_RST      0x80  /* 复位状态 */

/* RCR位定义 */
#define NE_RCR_SEP      0x01  /* 保存错误包 */
#define NE_RCR_AR       0x02  /* 接受广播 */
#define NE_RCR_AB       0x04  /* 接受广播（别名） */
#define NE_RCR_AM       0x08  /* 接受多播 */
#define NE_RCR_PRO      0x10  /* 混杂模式 */
#define NE_RCR_MON      0x20  /* 监控模式 */
#define NE_RCR_INTT     0x00  /* 中断立即触发 */

/* TCR位定义 */
#define NE_TCR_CRC      0x01  /* 不生成CRC */
#define NE_TCR_LB0      0x00  /* 正常模式 */

/* DCR位定义 */
#define NE_DCR_WTS      0x00  /* 16位传输 */
#define NE_DCR_BOS      0x01  /* 字节序交换 */
#define NE_DCR_LS       0x00  /* 非回环模式 */
#define NE_DCR_ARM      0x00  /* 非自动初始化远程DMA */
#define NE_DCR_FIFO     0x00  /* FIFO阈值 */

/* ==========================================
 * NE2000 I/O端口基地址
 * ========================================== */
#define NE_IO_BASE      0x300  /* NE2000默认I/O基地址 */
#define NE_IO_SIZE      0x20   /* I/O端口范围 */

/* 数据端口 = 基地址 + 0x10 */
#define NE_DATA_PORT    (NE_IO_BASE + 0x10)
#define NE_RESET_PORT   (NE_IO_BASE + 0x1F)

/* ==========================================
 * NE2000 IRQ
 * ========================================== */
#define NE_IRQ          11   /* NE2000使用的IRQ（对应IRQ11） */

/* ==========================================
 * 内存缓冲区配置
 * 每个页面256字节，NE2000使用256字节的页作为传输单位
 * ========================================== */
#define NE_PAGE_SIZE    256
#define NE_TX_PAGES     6    /* 发送缓冲区页数（6页=1536字节，够一个以太网帧） */
#define NE_RX_PAGES     20   /* 接收缓冲区页数 */

#define NE_TX_START     0x40  /* 发送缓冲区起始页 */
#define NE_RX_START     (NE_TX_START + NE_TX_PAGES)  /* 接收缓冲区起始页 */
#define NE_RX_STOP      (NE_RX_START + NE_RX_PAGES)  /* 接收缓冲区结束页 */

/* ==========================================
 * 接收包状态
 * ========================================== */
#define NE_RX_STATUS_PRX    0x01  /* 接收完成 */
#define NE_RX_STATUS_CRC    0x02  /* CRC错误 */
#define NE_RX_STATUS_FAE    0x04  /* 帧对齐错误 */
#define NE_RX_STATUS_FIFO   0x20  /* FIFO溢出 */

/* ==========================================
 * NE2000驱动私有数据
 * ========================================== */
typedef struct {
    uint16_t io_base;        /* I/O基地址 */
    uint8_t irq;             /* 使用的IRQ */
    bool initialized;        /* 是否已初始化 */
    
    /* 接收状态 */
    uint8_t curr_page;       /* 当前接收页指针 */
    
    /* 发送状态 */
    bool tx_busy;            /* 发送忙标志 */
    
    /* 统计信息 */
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t tx_errors;
    uint32_t rx_errors;
} ne2000_data_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化NE2000网卡 */
int ne2000_init(net_interface_t *iface);

/* NE2000中断处理 */
void ne2000_irq_handler(net_interface_t *iface);

/* 发送数据包 */
int ne2000_send(net_interface_t *iface, const void *data, uint32_t len);

/* 接收数据包 */
int ne2000_recv(net_interface_t *iface, void *buf, uint32_t len);

/* 获取NE2000状态信息 */
void ne2000_get_status(ne2000_data_t *status);

#endif /* NE2000_H */
