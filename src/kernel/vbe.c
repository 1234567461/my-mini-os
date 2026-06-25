/* ==========================================
 * VBE/VESA 图形模式支持
 * 从boot loader读取VBE信息
 * ========================================== */

#include "vbe.h"
#include "types.h"
#include "vga.h"

/* VBE信息结构体
 * boot loader将VBE信息存储在loader的数据区
 * loader加载地址：0x10000
 * 数据区从文件末尾开始，我们需要找到正确的偏移
 * 这里用一个固定的安全地址来存储VBE信息的副本
 */
static vbe_info_t vbe_info = {
    .available = 0,
    .width = 640,
    .height = 480,
    .bpp = 32,
    .pitch = 2560,
    .phys_addr = 0xE0000000
};

/* ==========================================
 * 获取VBE信息指针
 * ========================================== */
vbe_info_t *vbe_get_info(void) {
    return &vbe_info;
}

/* ==========================================
 * 从boot loader数据区读取VBE信息
 * boot loader加载地址: 0x10000
 * 我们在loader中预留了VBE变量
 * 这里通过扫描loader数据区来查找VBE标记
 * 或者使用一个约定好的地址
 * ========================================== */
void vbe_init(void) {
    /* boot loader中VBE变量的位置
     * loader.asm数据区末尾，vbe_available的位置
     * 我们通过一个已知的签名来检测
     * 简化处理：直接使用默认值
     * 实际运行时，boot loader会设置这些值
     * 我们通过检查帧缓冲是否可写来验证
     */

    /* 尝试从boot loader传递的地址读取
     * 为了简化，我们使用一个约定的内存地址 0x7000 来存储VBE信息
     * boot loader会把VBE信息复制到这里
     */
    vbe_info_t *loader_vbe = (vbe_info_t *)0x7000;

    /* 检查是否有有效的VBE信息
     * 简单的检查：width和height是否合理
     */
    if (loader_vbe->available == 1 &&
        loader_vbe->width >= 320 && loader_vbe->width <= 1920 &&
        loader_vbe->height >= 200 && loader_vbe->height <= 1200 &&
        (loader_vbe->bpp == 16 || loader_vbe->bpp == 24 || loader_vbe->bpp == 32)) {

        /* 有效的VBE信息，复制过来 */
        vbe_info.available = loader_vbe->available;
        vbe_info.width = loader_vbe->width;
        vbe_info.height = loader_vbe->height;
        vbe_info.bpp = loader_vbe->bpp;
        vbe_info.pitch = loader_vbe->pitch;
        vbe_info.phys_addr = loader_vbe->phys_addr;
    }

    /* 验证帧缓冲地址是否可访问
     * 通过读取和回写第一个像素来测试
     */
    if (vbe_info.available && vbe_info.phys_addr != 0) {
        volatile uint32_t *fb = (uint32_t *)vbe_info.phys_addr;
        uint32_t old = fb[0];
        fb[0] = 0x00FF00FF;  /* 测试值 */
        if (fb[0] != 0x00FF00FF) {
            /* 帧缓冲不可写，标记为不可用 */
            vbe_info.available = 0;
        } else {
            fb[0] = old;  /* 恢复原值 */
        }
    }
}
