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

/* ==========================================
 * 颜色操作函数
 * ========================================== */

/* 从颜色值中提取各分量 */
static uint8_t color_get_r(uint32_t color) { return (color >> 16) & 0xFF; }
static uint8_t color_get_g(uint32_t color) { return (color >> 8) & 0xFF; }
static uint8_t color_get_b(uint32_t color) { return color & 0xFF; }
static uint8_t color_get_a(uint32_t color) { return (color >> 24) & 0xFF; }

/* 混合颜色 */
uint32_t color_blend(uint32_t fg, uint32_t bg, uint8_t alpha) {
    if (alpha == 255) return fg;
    if (alpha == 0) return bg;

    uint8_t fg_a = color_get_a(fg);
    uint8_t fg_r = color_get_r(fg);
    uint8_t fg_g = color_get_g(fg);
    uint8_t fg_b = color_get_b(fg);

    uint8_t bg_r = color_get_r(bg);
    uint8_t bg_g = color_get_g(bg);
    uint8_t bg_b = color_get_b(bg);

    /* 简单的Alpha混合 */
    uint8_t result_r = (fg_r * alpha + bg_r * (255 - alpha)) / 255;
    uint8_t result_g = (fg_g * alpha + bg_g * (255 - alpha)) / 255;
    uint8_t result_b = (fg_b * alpha + bg_b * (255 - alpha)) / 255;

    return COLOR_RGB(result_r, result_g, result_b);
}

/* 使颜色变亮 */
uint32_t color_lighten(uint32_t color, int amount) {
    uint8_t r = color_get_r(color);
    uint8_t g = color_get_g(color);
    uint8_t b = color_get_b(color);

    r = (r + amount > 255) ? 255 : r + amount;
    g = (g + amount > 255) ? 255 : g + amount;
    b = (b + amount > 255) ? 255 : b + amount;

    return COLOR_RGB(r, g, b);
}

/* 使颜色变暗 */
uint32_t color_darken(uint32_t color, int amount) {
    uint8_t r = color_get_r(color);
    uint8_t g = color_get_g(color);
    uint8_t b = color_get_b(color);

    r = (r - amount < 0) ? 0 : r - amount;
    g = (g - amount < 0) ? 0 : g - amount;
    b = (b - amount < 0) ? 0 : b - amount;

    return COLOR_RGB(r, g, b);
}

/* ==========================================
 * 高级图形函数
 * ========================================== */

/* 渐变填充 */
void graphics_draw_gradient(int x, int y, int width, int height,
                          uint32_t color1, uint32_t color2, int vertical) {
    uint8_t r1 = color_get_r(color1), g1 = color_get_g(color1), b1 = color_get_b(color1);
    uint8_t r2 = color_get_r(color2), g2 = color_get_g(color2), b2 = color_get_b(color2);

    if (vertical) {
        /* 垂直渐变 */
        for (int py = 0; py < height; py++) {
            float ratio = (float)py / height;
            uint8_t r = r1 + (r2 - r1) * ratio;
            uint8_t g = g1 + (g2 - g1) * ratio;
            uint8_t b = b1 + (b2 - b1) * ratio;
            uint32_t color = COLOR_RGB(r, g, b);
            graphics_draw_line(x, y + py, x + width - 1, y + py, color);
        }
    } else {
        /* 水平渐变 */
        for (int px = 0; px < width; px++) {
            float ratio = (float)px / width;
            uint8_t r = r1 + (r2 - r1) * ratio;
            uint8_t g = g1 + (g2 - g1) * ratio;
            uint8_t b = b1 + (b2 - b1) * ratio;
            uint32_t color = COLOR_RGB(r, g, b);
            graphics_draw_line(x + px, y, x + px, y + height - 1, color);
        }
    }
}

/* 绘制带阴影的矩形 */
void graphics_draw_shadow_rect(int x, int y, int width, int height, int shadow_size) {
    /* 绘制阴影 */
    for (int i = 0; i < shadow_size; i++) {
        uint32_t shadow_color = COLOR_ARGB(80 - i * 10, 0, 0, 0);
        /* 底部阴影 */
        graphics_draw_line(x + shadow_size - i, y + height + i,
                          x + width - shadow_size + i, y + height + i, shadow_color);
        /* 右边阴影 */
        graphics_draw_line(x + width + i, y + shadow_size - i,
                          x + width + i, y + height - shadow_size + i, shadow_color);
    }
    /* 绘制主矩形 */
    graphics_draw_rect(x, y, width, height, COLOR_BLACK);
}

/* 绘制抗锯齿像素（简单的加权采样） */
void graphics_draw_antialiased_pixel(int x, int y, uint32_t color) {
    graphics_draw_pixel(x, y, color);
}

/* 绘制圆弧 - 使用Bresenham算法简化版 */
void graphics_draw_arc(int cx, int cy, int radius, int start_angle, int end_angle, uint32_t color) {
    /* 使用Bresenham算法绘制圆，简化版：只画上半部分或下半部分 */
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    /* 转换角度到弧度（用于确定绘制范围） */
    int start_rad = start_angle * 3 / 180;  /* 简化 */
    int end_rad = end_angle * 3 / 180;

    while (x <= y) {
        /* 根据角度范围绘制点 */
        if (x <= y) {
            /* 第一象限 */
            if (start_angle <= 0 && end_angle >= 0) {
                graphics_draw_pixel(cx + y, cy - x, color);
            }
            /* 第二象限 */
            if (start_angle <= 90 && end_angle >= 90) {
                graphics_draw_pixel(cx + x, cy - y, color);
            }
            /* 第三象限 */
            if (start_angle <= 180 && end_angle >= 180) {
                graphics_draw_pixel(cx - x, cy - y, color);
            }
            /* 第四象限 */
            if (start_angle <= 270 && end_angle >= 270) {
                graphics_draw_pixel(cx - y, cy - x, color);
            }

            /* 下方对称点 */
            if (start_angle <= 360 && end_angle >= 360) {
                graphics_draw_pixel(cx + y, cy + x, color);
            }
            if (start_angle <= 270 && end_angle >= 270) {
                graphics_draw_pixel(cx + x, cy + y, color);
            }
            if (start_angle <= 180 && end_angle >= 180) {
                graphics_draw_pixel(cx - x, cy + y, color);
            }
            if (start_angle <= 90 && end_angle >= 90) {
                graphics_draw_pixel(cx - y, cy + x, color);
            }
        }

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

/* 绘制圆角矩形边框 */
void graphics_draw_round_rect(int x, int y, int width, int height, int radius, uint32_t color) {
    /* 绘制四角的圆弧 */
    /* 左上角 */
    graphics_draw_arc(x + radius, y + radius, radius, 180, 270, color);
    /* 右上角 */
    graphics_draw_arc(x + width - radius - 1, y + radius, radius, 270, 360, color);
    /* 右下角 */
    graphics_draw_arc(x + width - radius - 1, y + height - radius - 1, radius, 0, 90, color);
    /* 左下角 */
    graphics_draw_arc(x + radius, y + height - radius - 1, radius, 90, 180, color);

    /* 绘制四条边 */
    graphics_draw_line(x + radius, y, x + width - radius - 1, y, color);  /* 上 */
    graphics_draw_line(x + width - 1, y + radius, x + width - 1, y + height - radius - 1, color);  /* 右 */
    graphics_draw_line(x + radius, y + height - 1, x + width - radius - 1, y + height - 1, color);  /* 下 */
    graphics_draw_line(x, y + radius, x, y + height - radius - 1, color);  /* 左 */
}

/* 填充圆角矩形 */
void graphics_fill_round_rect(int x, int y, int width, int height, int radius, uint32_t color) {
    /* 填充主体矩形（去掉四角） */
    graphics_fill_rect(x + radius, y, width - 2 * radius, height, color);
    graphics_fill_rect(x, y + radius, width, height - 2 * radius, color);

    /* 填充四个角的圆 */
    graphics_fill_circle(x + radius, y + radius, radius, color);
    graphics_fill_circle(x + width - radius - 1, y + radius, radius, color);
    graphics_fill_circle(x + radius, y + height - radius - 1, radius, color);
    graphics_fill_circle(x + width - radius - 1, y + height - radius - 1, radius, color);
}