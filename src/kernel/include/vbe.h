/* ==========================================
 * VBE/VESA 图形模式信息
 * 由boot loader填充，内核读取
 * ========================================== */

#ifndef VBE_H
#define VBE_H

#include "types.h"

/* VBE信息结构体（与boot loader共享）
 * 注意：boot loader在0x10000+偏移处存储这些变量
 * 我们在内核中通过绝对地址访问
 */
typedef struct {
    uint8_t  available;       /* VBE是否可用 */
    uint16_t width;           /* 水平分辨率 */
    uint16_t height;          /* 垂直分辨率 */
    uint8_t  bpp;             /* 每像素位数 */
    uint16_t pitch;           /* 每行字节数 */
    uint32_t phys_addr;       /* 线性帧缓冲物理地址 */
} __attribute__((packed)) vbe_info_t;

/* VBE信息的物理地址（boot loader在0x10000+偏移处存储）
 * boot loader加载到0x10000，vbe_available是数据区的第一个变量
 * 我们需要计算偏移量
 */
#define VBE_INFO_ADDR  0x10000 + 0x200  /* 估算的偏移地址 */

/* 获取VBE信息指针 */
vbe_info_t *vbe_get_info(void);

/* 初始化VBE帧缓冲 */
void vbe_init(void);

#endif
