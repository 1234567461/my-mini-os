/* ==========================================
 * Window - 窗口管理实现 v0.8.0
 * 功能：窗口创建、销毁、绘制、焦点管理等
 * ========================================== */

#include "gui.h"
#include "heap.h"
#include "string.h"

/* 窗口管理器状态 */
static window_t *window_list = NULL;      /* 所有窗口链表 */
static window_t *focused_window = NULL;   /* 当前焦点窗口 */
static int window_count = 0;
static int next_z_order = 0;

/* 桌面子窗口 */
static window_t *desktop_window = NULL;

/* 默认窗口样式颜色 */
#define DEFAULT_TITLE_HEIGHT  24
#define DEFAULT_BORDER_WIDTH  2
#define DEFAULT_BORDER_COLOR  COLOR_RGB(100, 100, 100)
#define DEFAULT_TITLE_COLOR   COLOR_RGB(0, 0, 139)  /* 深蓝色 */
#define DEFAULT_TITLE_TEXT_COLOR COLOR_WHITE
#define DEFAULT_BG_COLOR      COLOR_RGB(192, 192, 192)  /* 浅灰色 */

/* ==========================================
 * 初始化窗口管理系统
 * ========================================== */
void window_init(void) {
    window_list = NULL;
    focused_window = NULL;
    desktop_window = NULL;
    window_count = 0;
    next_z_order = 0;

    /* 创建桌面窗口 */
    desktop_window = window_create("Desktop", 0, 0, GUI_WIDTH, GUI_HEIGHT);
    if (desktop_window) {
        desktop_window->style = WINDOW_STYLE_DEFAULT;
        window_set_color(desktop_window, COLOR_RGB(0, 128, 192), COLOR_BLACK);
    }
}

/* ==========================================
 * 创建窗口
 * ========================================== */
window_t *window_create(const char *title, int x, int y, int width, int height) {
    window_t *win = (window_t *)kmalloc(sizeof(window_t));
    if (!win) return NULL;

    /* 清零结构体 */
    memset(win, 0, sizeof(window_t));

    /* 设置标题 */
    if (title) {
        strncpy(win->title, title, 63);
        win->title[63] = '\0';
    }

    /* 设置边界 */
    win->bounds.x = x;
    win->bounds.y = y;
    win->bounds.width = width;
    win->bounds.height = height;

    /* 计算客户区（减去标题栏和边框） */
    win->client_area.x = x + DEFAULT_BORDER_WIDTH;
    win->client_area.y = y + DEFAULT_TITLE_HEIGHT;
    win->client_area.width = width - DEFAULT_BORDER_WIDTH * 2;
    win->client_area.height = height - DEFAULT_TITLE_HEIGHT - DEFAULT_BORDER_WIDTH;

    /* 设置颜色 */
    win->bg_color = DEFAULT_BG_COLOR;
    win->border_color = DEFAULT_BORDER_COLOR;
    win->title_color = DEFAULT_TITLE_COLOR;
    win->title_text_color = DEFAULT_TITLE_TEXT_COLOR;

    /* 设置标志 */
    win->flags = WINDOW_FLAG_TITLEBAR | WINDOW_FLAG_MOVABLE | WINDOW_FLAG_CLOSABLE;
    win->style = WINDOW_STYLE_DEFAULT;
    win->proc = NULL;
    win->user_data = NULL;
    win->parent = NULL;
    win->next = NULL;
    win->children = NULL;
    win->z_order = next_z_order++;
    win->is_visible = 1;
    win->is_focused = 0;
    win->is_dragging = 0;

    /* 添加到窗口链表 */
    win->next = window_list;
    window_list = win;
    window_count++;

    return win;
}

/* ==========================================
 * 销毁窗口
 * ========================================== */
void window_destroy(window_t *win) {
    if (!win) return;

    /* 从链表中移除 */
    window_t **prev = &window_list;
    while (*prev && *prev != win) {
        prev = &(*prev)->next;
    }
    if (*prev) {
        *prev = win->next;
    }

    /* 如果是焦点窗口，清除焦点 */
    if (focused_window == win) {
        focused_window = NULL;
    }

    /* 销毁子窗口 */
    window_t *child = win->children;
    while (child) {
        window_t *next = child->next;
        window_destroy(child);
        child = next;
    }

    /* 释放内存 */
    kfree(win);
    window_count--;
}

/* ==========================================
 * 显示窗口
 * ========================================== */
void window_show(window_t *win) {
    if (!win) return;
    win->is_visible = 1;
    window_invalidate(win);
}

/* ==========================================
 * 隐藏窗口
 * ========================================== */
void window_hide(window_t *win) {
    if (!win) return;
    win->is_visible = 0;
    /* 重绘以清除窗口区域 */
    window_invalidate(win);
}

/* ==========================================
 * 移动窗口
 * ========================================== */
void window_move(window_t *win, int x, int y) {
    if (!win) return;

    int old_x = win->bounds.x;
    int old_y = win->bounds.y;

    win->bounds.x = x;
    win->bounds.y = y;

    /* 更新客户区坐标 */
    win->client_area.x = x + DEFAULT_BORDER_WIDTH;
    win->client_area.y = y + DEFAULT_TITLE_HEIGHT;

    /* 如果可见，重绘 */
    if (win->is_visible) {
        window_invalidate(win);
    }
}

/* ==========================================
 * 调整窗口大小
 * ========================================== */
void window_resize(window_t *win, int width, int height) {
    if (!win) return;
    if (width < 100) width = 100;
    if (height < 100) height = 100;

    win->bounds.width = width;
    win->bounds.height = height;

    /* 更新客户区 */
    win->client_area.width = width - DEFAULT_BORDER_WIDTH * 2;
    win->client_area.height = height - DEFAULT_TITLE_HEIGHT - DEFAULT_BORDER_WIDTH;

    /* 如果可见，重绘 */
    if (win->is_visible) {
        window_invalidate(win);
    }
}

/* ==========================================
 * 设置窗口标题
 * ========================================== */
void window_set_title(window_t *win, const char *title) {
    if (!win || !title) return;
    strncpy(win->title, title, 63);
    win->title[63] = '\0';

    /* 如果可见，重绘标题栏 */
    if (win->is_visible) {
        window_draw_titlebar(win);
    }
}

/* ==========================================
 * 设置窗口颜色
 * ========================================== */
void window_set_color(window_t *win, uint32_t bg_color, uint32_t border_color) {
    if (!win) return;
    win->bg_color = bg_color;
    win->border_color = border_color;
    window_invalidate(win);
}

/* ==========================================
 * 使窗口无效（需要重绘）
 * ========================================== */
void window_invalidate(window_t *win) {
    if (!win || !win->is_visible) return;
    /* 在简单实现中，直接重绘窗口 */
    window_draw(win);
}

/* ==========================================
 * 使窗口特定区域无效
 * ========================================== */
void window_invalidate_rect(window_t *win, rect_t *rect) {
    if (!win || !rect || !win->is_visible) return;
    (void)rect;
    /* 简化实现：重绘整个窗口 */
    window_draw(win);
}

/* ==========================================
 * 设置焦点窗口
 * ========================================== */
void window_set_focus(window_t *win) {
    /* 如果新焦点窗口不可见，不设置焦点 */
    if (win && !win->is_visible) return;

    /* 清除旧焦点窗口的焦点 */
    if (focused_window && focused_window != win) {
        focused_window->is_focused = 0;
        window_draw_border(focused_window);
    }

    /* 设置新焦点窗口 */
    focused_window = win;

    if (win) {
        win->is_focused = 1;
        /* 将窗口放到最前面 */
        window_bring_to_front(win);
        window_draw_border(win);
        window_draw_titlebar(win);
    }
}

/* ==========================================
 * 获取焦点窗口
 * ========================================== */
window_t *window_get_focused(void) {
    return focused_window;
}

/* ==========================================
 * 将窗口放到最前面
 * ========================================== */
void window_bring_to_front(window_t *win) {
    if (!win) return;
    win->z_order = next_z_order++;
}

/* ==========================================
 * 将窗口放到最后面
 * ========================================== */
void window_send_to_back(window_t *win) {
    if (!win) return;
    /* 将z_order设为负数，使其在最底层 */
    win->z_order = -1000;
}

/* ==========================================
 * 绘制窗口
 * ========================================== */
void window_draw(window_t *win) {
    if (!win || !win->is_visible) return;

    /* 绘制边框 */
    window_draw_border(win);

    /* 绘制标题栏 */
    if (win->flags & WINDOW_FLAG_TITLEBAR) {
        window_draw_titlebar(win);
    }

    /* 填充客户区背景 */
    graphics_fill_rect(
        win->client_area.x,
        win->client_area.y,
        win->client_area.width,
        win->client_area.height,
        win->bg_color
    );
}

/* ==========================================
 * 绘制窗口边框
 * ========================================== */
void window_draw_border(window_t *win) {
    if (!win || !win->is_visible) return;

    uint32_t border_color = win->border_color;

    /* 如果是焦点窗口，使用深色边框 */
    if (win->is_focused) {
        border_color = COLOR_RGB(0, 0, 128);  /* 深蓝色 */
    }

    /* 绘制外边框 */
    graphics_draw_rect(
        win->bounds.x,
        win->bounds.y,
        win->bounds.width,
        win->bounds.height,
        border_color
    );

    /* 绘制内边框（3D效果） */
    /* 顶部和左边亮色 */
    graphics_draw_line(
        win->bounds.x + 1,
        win->bounds.y + 1,
        win->bounds.x + win->bounds.width - 2,
        win->bounds.y + 1,
        COLOR_WHITE
    );
    graphics_draw_line(
        win->bounds.x + 1,
        win->bounds.y + 1,
        win->bounds.x + 1,
        win->bounds.y + win->bounds.height - 2,
        COLOR_WHITE
    );

    /* 底部和右边暗色 */
    graphics_draw_line(
        win->bounds.x + 1,
        win->bounds.y + win->bounds.height - 2,
        win->bounds.x + win->bounds.width - 2,
        win->bounds.y + win->bounds.height - 2,
        COLOR_BLACK
    );
    graphics_draw_line(
        win->bounds.x + win->bounds.width - 2,
        win->bounds.y + 1,
        win->bounds.x + win->bounds.width - 2,
        win->bounds.y + win->bounds.height - 2,
        COLOR_BLACK
    );
}

/* ==========================================
 * 绘制窗口标题栏
 * ========================================== */
void window_draw_titlebar(window_t *win) {
    if (!win || !win->is_visible) return;
    if (!(win->flags & WINDOW_FLAG_TITLEBAR)) return;

    int x = win->bounds.x;
    int y = win->bounds.y;
    int width = win->bounds.width;
    int height = DEFAULT_TITLE_HEIGHT;

    /* 填充标题栏背景 */
    uint32_t title_bg = win->title_color;
    if (win->is_focused) {
        title_bg = win->title_color;
    } else {
        /* 非焦点窗口使用较暗的标题栏 */
        title_bg = COLOR_RGB(128, 128, 128);
    }

    graphics_fill_rect(x + 1, y + 1, width - 2, height - 1, title_bg);

    /* 绘制标题文字 */
    int text_x = x + 6;
    int text_y = y + (height - 16) / 2;  /* 垂直居中 */

    font_draw_string(text_x, text_y, win->title, win->title_text_color, 1);

    /* 绘制关闭按钮（如果可关闭） */
    if (win->flags & WINDOW_FLAG_CLOSABLE) {
        int btn_x = x + width - 20;
        int btn_y = y + 4;
        int btn_size = 16;

        /* 关闭按钮背景 */
        graphics_fill_rect(btn_x, btn_y, btn_size, btn_size, COLOR_RGB(192, 192, 192));

        /* 关闭按钮边框 */
        graphics_draw_rect(btn_x, btn_y, btn_size, btn_size, COLOR_BLACK);

        /* X 符号 */
        graphics_draw_line(btn_x + 4, btn_y + 4, btn_x + btn_size - 4, btn_y + btn_size - 4, COLOR_RED);
        graphics_draw_line(btn_x + 4, btn_y + btn_size - 4, btn_x + btn_size - 4, btn_y + 4, COLOR_RED);
    }
}

/* ==========================================
 * 实用函数：检测点是否在矩形内
 * ========================================== */
int rect_contains_point(rect_t *rect, int x, int y) {
    if (!rect) return 0;
    return (x >= rect->x && x < rect->x + rect->width &&
            y >= rect->y && y < rect->y + rect->height);
}

/* ==========================================
 * 实用函数：检测两矩形是否相交
 * ========================================== */
int rects_intersect(rect_t *a, rect_t *b) {
    if (!a || !b) return 0;
    return !(a->x + a->width <= b->x ||
             b->x + b->width <= a->x ||
             a->y + a->height <= b->y ||
             b->y + b->height <= a->y);
}

/* ==========================================
 * 实用函数：计算两矩形的并集
 * ========================================== */
void rect_union(rect_t *a, rect_t *b, rect_t *result) {
    if (!a || !b || !result) return;

    int min_x = a->x < b->x ? a->x : b->x;
    int min_y = a->y < b->y ? a->y : b->y;
    int max_x = (a->x + a->width) > (b->x + b->width) ? (a->x + a->width) : (b->x + b->width);
    int max_y = (a->y + a->height) > (b->y + b->height) ? (a->y + a->height) : (b->y + b->height);

    result->x = min_x;
    result->y = min_y;
    result->width = max_x - min_x;
    result->height = max_y - min_y;
}

/* ==========================================
 * 检测点是否在标题栏上
 * ========================================== */
int window_hit_test_titlebar(window_t *win, int x, int y) {
    if (!win || !win->is_visible) return 0;
    if (!(win->flags & WINDOW_FLAG_TITLEBAR)) return 0;

    rect_t titlebar = {
        win->bounds.x,
        win->bounds.y,
        win->bounds.width,
        DEFAULT_TITLE_HEIGHT
    };

    return rect_contains_point(&titlebar, x, y);
}

/* ==========================================
 * 检测点是否在关闭按钮上
 * ========================================== */
int window_hit_test_close_button(window_t *win, int x, int y) {
    if (!win || !win->is_visible) return 0;
    if (!(win->flags & WINDOW_FLAG_CLOSABLE)) return 0;

    int btn_x = win->bounds.x + win->bounds.width - 20;
    int btn_y = win->bounds.y + 4;
    int btn_size = 16;

    rect_t close_btn = {btn_x, btn_y, btn_size, btn_size};
    return rect_contains_point(&close_btn, x, y);
}

/* ==========================================
 * 获取窗口在指定坐标处的最顶层窗口
 * ========================================== */
window_t *window_get_at_point(int x, int y) {
    window_t *top = NULL;
    int top_z = -9999;

    window_t *win = window_list;
    while (win) {
        if (win->is_visible && rect_contains_point(&win->bounds, x, y)) {
            if (win->z_order > top_z) {
                top = win;
                top_z = win->z_order;
            }
        }
        win = win->next;
    }

    return top;
}

/* ==========================================
 * 获取窗口链表
 * ========================================== */
window_t *window_get_list(void) {
    return window_list;
}