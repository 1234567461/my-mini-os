/* ==========================================
 * Framebuffer - 帧缓冲驱动 v1.0.0
 * 功能：VBE/VESA图形模式下的帧缓冲操作
 * ========================================== */

#include "gui.h"
#include "types.h"
#include "vga.h"
#include "vbe.h"

/* 帧缓冲信息 */
static uint32_t *framebuffer = (uint32_t *)0xE0000000;  /* 线性帧缓冲基址 */
static int fb_initialized = 0;
static int fb_width = 640;
static int fb_height = 480;
static int fb_bpp = 32;
static int fb_pitch = 2560;

/* 当前剪辑区域 */
static rect_t clip_rect = {0, 0, GUI_WIDTH, GUI_HEIGHT};

/* ==========================================
 * 初始化帧缓冲
 * ========================================== */
void framebuffer_init(void) {
    vbe_info_t *vbe = vbe_get_info();

    if (vbe->available) {
        /* 使用VBE提供的真实帧缓冲地址和参数 */
        framebuffer = (uint32_t *)vbe->phys_addr;
        fb_width = vbe->width;
        fb_height = vbe->height;
        fb_bpp = vbe->bpp;
        fb_pitch = vbe->pitch;

        /* 更新剪辑区域 */
        clip_rect.width = fb_width < GUI_WIDTH ? fb_width : GUI_WIDTH;
        clip_rect.height = fb_height < GUI_HEIGHT ? fb_height : GUI_HEIGHT;
    }

    fb_initialized = 1;

    /* 清空帧缓冲 */
    framebuffer_fill_rect(0, 0, fb_width, fb_height, COLOR_BLACK);
}

/* ==========================================
 * 设置像素
 * ========================================== */
void framebuffer_set_pixel(int x, int y, uint32_t color) {
    if (!fb_initialized) return;

    /* 边界检查 */
    if (x < 0 || x >= fb_width || y < 0 || y >= fb_height) return;

    /* 剪辑区域检查 */
    if (x < clip_rect.x || x >= clip_rect.x + clip_rect.width ||
        y < clip_rect.y || y >= clip_rect.y + clip_rect.height) return;

    uint32_t *pixel = (uint32_t *)((uint8_t *)framebuffer + y * fb_pitch + x * (fb_bpp / 8));
    *pixel = color;
}

/* ==========================================
 * 获取像素
 * ========================================== */
uint32_t framebuffer_get_pixel(int x, int y) {
    if (!fb_initialized) return 0;
    if (x < 0 || x >= fb_width || y < 0 || y >= fb_height) return 0;

    uint32_t *pixel = (uint32_t *)((uint8_t *)framebuffer + y * fb_pitch + x * (fb_bpp / 8));
    return *pixel;
}

/* ==========================================
 * 填充矩形
 * ========================================== */
void framebuffer_fill_rect(int x, int y, int width, int height, uint32_t color) {
    if (!fb_initialized) return;

    /* 边界裁剪 */
    int start_x = x < 0 ? 0 : x;
    int start_y = y < 0 ? 0 : y;
    int end_x = (x + width) > fb_width ? fb_width : (x + width);
    int end_y = (y + height) > fb_height ? fb_height : (y + height);

    /* 填充 */
    for (int py = start_y; py < end_y; py++) {
        uint32_t *row = (uint32_t *)((uint8_t *)framebuffer + py * fb_pitch);
        for (int px = start_x; px < end_x; px++) {
            row[px] = color;
        }
    }
}

/* ==========================================
 * 像素块传输
 * ========================================== */
void framebuffer_blit(int x, int y, int width, int height, uint32_t *data) {
    if (!fb_initialized || data == NULL) return;

    for (int py = 0; py < height; py++) {
        int dest_y = y + py;
        if (dest_y < 0 || dest_y >= fb_height) continue;

        uint32_t *dest_row = (uint32_t *)((uint8_t *)framebuffer + dest_y * fb_pitch);
        uint32_t *src_row = &data[py * width];

        for (int px = 0; px < width; px++) {
            int dest_x = x + px;
            if (dest_x >= 0 && dest_x < fb_width) {
                dest_row[dest_x] = src_row[px];
            }
        }
    }
}

/* ==========================================
 * 区域复制
 * ========================================== */
void framebuffer_copy(int dest_x, int dest_y, int src_x, int src_y, int width, int height) {
    if (!fb_initialized) return;

    /* 从下往上复制，避免覆盖 */
    if (dest_y < src_y) {
        for (int y = 0; y < height; y++) {
            int sy = src_y + y;
            int dy = dest_y + y;
            if (sy < 0 || sy >= fb_height || dy < 0 || dy >= fb_height) continue;

            uint32_t *src_row = (uint32_t *)((uint8_t *)framebuffer + sy * fb_pitch);
            uint32_t *dest_row = (uint32_t *)((uint8_t *)framebuffer + dy * fb_pitch);

            for (int x = 0; x < width; x++) {
                int sx = src_x + x;
                int dx = dest_x + x;
                if (sx >= 0 && sx < fb_width && dx >= 0 && dx < fb_width) {
                    dest_row[dx] = src_row[sx];
                }
            }
        }
    } else {
        for (int y = height - 1; y >= 0; y--) {
            int sy = src_y + y;
            int dy = dest_y + y;
            if (sy < 0 || sy >= fb_height || dy < 0 || dy >= fb_height) continue;

            uint32_t *src_row = (uint32_t *)((uint8_t *)framebuffer + sy * fb_pitch);
            uint32_t *dest_row = (uint32_t *)((uint8_t *)framebuffer + dy * fb_pitch);

            for (int x = 0; x < width; x++) {
                int sx = src_x + x;
                int dx = dest_x + x;
                if (sx >= 0 && sx < fb_width && dx >= 0 && dx < fb_width) {
                    dest_row[dx] = src_row[sx];
                }
            }
        }
    }
}

/* ==========================================
 * 刷新帧缓冲（用于双缓冲）
 * ========================================== */
void framebuffer_flush(void) {
    /* 在真实硬件上，这里可能需要：
     * 1. 等待垂直回扫
     * 2. 复制到硬件帧缓冲
     *
     * 目前是直接模式，无需额外操作
     */
}

/* ==========================================
 * 获取帧缓冲宽度
 * ========================================== */
int framebuffer_get_width(void) {
    return fb_width;
}

/* ==========================================
 * 获取帧缓冲高度
 * ========================================== */
int framebuffer_get_height(void) {
    return fb_height;
}

/* ==========================================
 * 获取帧缓冲行间距（pitch）
 * ========================================== */
int framebuffer_get_pitch(void) {
    return fb_pitch;
}

/* ==========================================
 * 获取帧缓冲色深（bpp）
 * ========================================== */
int framebuffer_get_bpp(void) {
    return fb_bpp;
}