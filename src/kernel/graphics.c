/* ==========================================
 * Graphics - 图形绘制实现 v0.8.0
 * 功能：基本图形绘制（线、矩形、圆等）
 * ========================================== */

#include "gui.h"

/* 当前剪辑区域 */
static rect_t current_clip = {0, 0, GUI_WIDTH, GUI_HEIGHT};

/* ==========================================
 * 初始化图形系统
 * ========================================== */
void graphics_init(void) {
    /* 初始化剪辑区域为全屏 */
    current_clip.x = 0;
    current_clip.y = 0;
    current_clip.width = GUI_WIDTH;
    current_clip.height = GUI_HEIGHT;
}

/* ==========================================
 * 设置剪辑矩形
 * ========================================== */
void graphics_set_clip_rect(rect_t *rect) {
    if (rect) {
        current_clip = *rect;
    } else {
        current_clip.x = 0;
        current_clip.y = 0;
        current_clip.width = GUI_WIDTH;
        current_clip.height = GUI_HEIGHT;
    }
}

/* ==========================================
 * 获取剪辑矩形
 * ========================================== */
void graphics_get_clip_rect(rect_t *rect) {
    if (rect) {
        *rect = current_clip;
    }
}

/* ==========================================
 * 绘制像素（内部函数，不做边界检查）
 * ========================================== */
static void draw_pixel_unsafe(int x, int y, uint32_t color) {
    framebuffer_set_pixel(x, y, color);
}

/* ==========================================
 * 绘制像素（公开接口，有边界检查）
 * ========================================== */
void graphics_draw_pixel(int x, int y, uint32_t color) {
    /* 剪辑检查 */
    if (x < current_clip.x || x >= current_clip.x + current_clip.width ||
        y < current_clip.y || y >= current_clip.y + current_clip.height) {
        return;
    }
    framebuffer_set_pixel(x, y, color);
}

/* ==========================================
 * 绘制直线（Bresenham算法）
 * ========================================== */
void graphics_draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int abs_dx = dx < 0 ? -dx : dx;
    int abs_dy = dy < 0 ? -dy : dy;
    int sign_x = dx < 0 ? -1 : 1;
    int sign_y = dy < 0 ? -1 : 1;
    int e = abs_dx - abs_dy;

    while (1) {
        graphics_draw_pixel(x1, y1, color);

        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * e;
        if (e2 > -abs_dy) {
            e -= abs_dy;
            x1 += sign_x;
        }
        if (e2 < abs_dx) {
            e += abs_dx;
            y1 += sign_y;
        }
    }
}

/* ==========================================
 * 绘制矩形边框
 * ========================================== */
void graphics_draw_rect(int x, int y, int width, int height, uint32_t color) {
    /* 上边和下边 */
    graphics_draw_line(x, y, x + width - 1, y, color);
    graphics_draw_line(x, y + height - 1, x + width - 1, y + height - 1, color);

    /* 左边和右边 */
    graphics_draw_line(x, y, x, y + height - 1, color);
    graphics_draw_line(x + width - 1, y, x + width - 1, y + height - 1, color);
}

/* ==========================================
 * 填充矩形
 * ========================================== */
void graphics_fill_rect(int x, int y, int width, int height, uint32_t color) {
    /* 裁剪 */
    int start_x = x;
    int start_y = y;
    int end_x = x + width;
    int end_y = y + height;

    if (start_x < current_clip.x) start_x = current_clip.x;
    if (start_y < current_clip.y) start_y = current_clip.y;
    if (end_x > current_clip.x + current_clip.width) end_x = current_clip.x + current_clip.width;
    if (end_y > current_clip.y + current_clip.height) end_y = current_clip.y + current_clip.height;

    if (start_x >= end_x || start_y >= end_y) return;

    framebuffer_fill_rect(start_x, start_y, end_x - start_x, end_y - start_y, color);
}

/* ==========================================
 * 绘制圆（Bresenham算法）
 * ========================================== */
void graphics_draw_circle(int cx, int cy, int radius, uint32_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (x <= y) {
        graphics_draw_pixel(cx + x, cy + y, color);
        graphics_draw_pixel(cx - x, cy + y, color);
        graphics_draw_pixel(cx + x, cy - y, color);
        graphics_draw_pixel(cx - x, cy - y, color);
        graphics_draw_pixel(cx + y, cy + x, color);
        graphics_draw_pixel(cx - y, cy + x, color);
        graphics_draw_pixel(cx + y, cy - x, color);
        graphics_draw_pixel(cx - y, cy - x, color);

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

/* ==========================================
 * 填充圆
 * ========================================== */
void graphics_fill_circle(int cx, int cy, int radius, uint32_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (x <= y) {
        /* 填充垂直扫描线 */
        graphics_draw_line(cx - x, cy + y, cx + x, cy + y, color);
        graphics_draw_line(cx - x, cy - y, cx + x, cy - y, color);
        graphics_draw_line(cx - y, cy + x, cx + y, cy + x, color);
        graphics_draw_line(cx - y, cy - x, cx + y, cy - x, color);

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

/* ==========================================
 * 绘制位图（单色位图，如字体）
 * ========================================== */
void graphics_draw_bitmap(int x, int y, int width, int height,
                          const uint8_t *bitmap, uint32_t fg_color, uint32_t bg_color) {
    for (int py = 0; py < height; py++) {
        for (int px = 0; px < width; px++) {
            int byte_index = py * ((width + 7) / 8) + (px / 8);
            int bit_index = 7 - (px % 8);
            int bit = (bitmap[byte_index] >> bit_index) & 1;

            if (bit) {
                graphics_draw_pixel(x + px, y + py, fg_color);
            } else if (bg_color != 0) {  /* bg_color为0表示透明 */
                graphics_draw_pixel(x + px, y + py, bg_color);
            }
        }
    }
}

/* ==========================================
 * 复制区域
 * ========================================== */
void graphics_copy_area(int src_x, int src_y, int width, int height, int dest_x, int dest_y) {
    /* 简单的裁剪 */
    if (dest_x < 0) {
        src_x -= dest_x;
        width += dest_x;
        dest_x = 0;
    }
    if (dest_y < 0) {
        src_y -= dest_y;
        height += dest_y;
        dest_y = 0;
    }
    if (src_x < 0) {
        dest_x -= src_x;
        width += src_x;
        src_x = 0;
    }
    if (src_y < 0) {
        dest_y -= src_y;
        height += src_y;
        src_y = 0;
    }

    if (width <= 0 || height <= 0) return;

    int max_width = GUI_WIDTH - (src_x > dest_x ? src_x : dest_x);
    int max_height = GUI_HEIGHT - (src_y > dest_y ? src_y : dest_y);

    if (width > max_width) width = max_width;
    if (height > max_height) height = max_height;

    framebuffer_copy(dest_x, dest_y, src_x, src_y, width, height);
}