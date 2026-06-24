/* ==========================================
 * GUI - 图形用户界面核心头文件 v0.8.0
 * 功能：
 *   1. 帧缓冲驱动
 *   2. 图形绘制
 *   3. 窗口管理
 *   4. 事件处理
 * ========================================== */

#ifndef GUI_H
#define GUI_H

#include "types.h"

/* GUI常量定义 */
#define GUI_WIDTH    640
#define GUI_HEIGHT   480
#define GUI_BPP      32          /* 32位色深 */
#define GUI_PITCH    (GUI_WIDTH * 4)  /* 每行字节数 */

/* 颜色格式：0xAARRGGBB */
#define COLOR_ARGB(a, r, g, b) ((uint32_t)((a) << 24 | (r) << 16 | (g) << 8 | (b)))
#define COLOR_RGB(r, g, b)     COLOR_ARGB(255, r, g, b)
#define COLOR_BLACK            COLOR_RGB(0, 0, 0)
#define COLOR_WHITE            COLOR_RGB(255, 255, 255)
#define COLOR_RED              COLOR_RGB(255, 0, 0)
#define COLOR_GREEN            COLOR_RGB(0, 255, 0)
#define COLOR_BLUE             COLOR_RGB(0, 0, 255)
#define COLOR_YELLOW           COLOR_RGB(255, 255, 0)
#define COLOR_CYAN             COLOR_RGB(0, 255, 255)
#define COLOR_MAGENTA          COLOR_RGB(255, 0, 255)
#define COLOR_GRAY             COLOR_RGB(128, 128, 128)
#define COLOR_LIGHT_GRAY       COLOR_RGB(192, 192, 192)
#define COLOR_DARK_GRAY        COLOR_RGB(64, 64, 64)

/* 透明度 */
#define ALPHA_TRANSPARENT  0
#define ALPHA_OPAQUE       255

/* 矩形结构 */
typedef struct {
    int x, y;
    int width, height;
} rect_t;

/* 点结构 */
typedef struct {
    int x, y;
} point_t;

/* 颜色结构 */
typedef struct {
    uint8_t r, g, b, a;
} color_t;

/* 窗口样式 */
typedef enum {
    WINDOW_STYLE_DEFAULT,
    WINDOW_STYLE_DIALOG,
    WINDOW_STYLE_TOOLBAR
} window_style_t;

/* 窗口标志 */
#define WINDOW_FLAG_TITLEBAR   0x01
#define WINDOW_FLAG_RESIZABLE  0x02
#define WINDOW_FLAG_MOVABLE    0x04
#define WINDOW_FLAG_CLOSABLE   0x08
#define WINDOW_FLAG_MINIMIZABLE 0x10

/* 事件类型 */
typedef enum {
    EVENT_NONE,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_DOWN,
    EVENT_MOUSE_UP,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_WINDOW_CLOSE,
    EVENT_WINDOW_RESIZE,
    EVENT_WINDOW_MOVE,
    EVENT_PAINT
} event_type_t;

/* 事件结构 */
typedef struct {
    event_type_t type;
    int mouse_x, mouse_y;
    int mouse_button;
    uint32_t key_code;
    void *window;
    void *data;
} event_t;

/* 窗口结构前向声明 */
typedef struct window window_t;

/* 窗口处理函数类型 */
typedef void (*window_proc_t)(window_t *win, event_t *event);

/* 窗口结构 */
struct window {
    char title[64];           /* 窗口标题 */
    rect_t bounds;            /* 窗口边界 */
    rect_t client_area;       /* 客户区 */
    uint32_t bg_color;        /* 背景色 */
    uint32_t border_color;     /* 边框色 */
    uint32_t title_color;     /* 标题栏颜色 */
    uint32_t title_text_color; /* 标题文字颜色 */
    uint8_t flags;            /* 窗口标志 */
    window_style_t style;     /* 窗口样式 */
    window_proc_t proc;       /* 窗口过程函数 */
    void *user_data;         /* 用户数据 */
    window_t *parent;         /* 父窗口 */
    window_t *next;           /* 链表下一窗口 */
    window_t *children;       /* 子窗口链表 */
    int z_order;              /* Z顺序 */
    int is_visible;           /* 是否可见 */
    int is_focused;           /* 是否获得焦点 */
    int is_dragging;          /* 是否在拖动 */
    point_t drag_offset;      /* 拖动偏移 */
};

/* ==========================================
 * 帧缓冲函数
 * ========================================== */
void framebuffer_init(void);
void framebuffer_set_pixel(int x, int y, uint32_t color);
uint32_t framebuffer_get_pixel(int x, int y);
void framebuffer_fill_rect(int x, int y, int width, int height, uint32_t color);
void framebuffer_blit(int x, int y, int width, int height, uint32_t *data);
void framebuffer_copy(int dest_x, int dest_y, int src_x, int src_y, int width, int height);
void framebuffer_flush(void);

/* ==========================================
 * 图形绘制函数
 * ========================================== */
void graphics_init(void);
void graphics_draw_pixel(int x, int y, uint32_t color);
void graphics_draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void graphics_draw_rect(int x, int y, int width, int height, uint32_t color);
void graphics_fill_rect(int x, int y, int width, int height, uint32_t color);
void graphics_draw_circle(int cx, int cy, int radius, uint32_t color);
void graphics_fill_circle(int cx, int cy, int radius, uint32_t color);
void graphics_draw_bitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint32_t fg_color, uint32_t bg_color);
void graphics_copy_area(int src_x, int src_y, int width, int height, int dest_x, int dest_y);
void graphics_set_clip_rect(rect_t *rect);
void graphics_get_clip_rect(rect_t *rect);

/* ==========================================
 * 字体渲染函数
 * ========================================== */
void font_init(void);
int font_draw_char(int x, int y, char c, uint32_t color, int size);
int font_draw_string(int x, int y, const char *str, uint32_t color, int size);
int font_measure_string(const char *str, int size);
int font_char_width(int size);
int font_char_height(int size);

/* ==========================================
 * 窗口管理函数
 * ========================================== */
void window_init(void);
window_t *window_create(const char *title, int x, int y, int width, int height);
void window_destroy(window_t *win);
void window_show(window_t *win);
void window_hide(window_t *win);
void window_move(window_t *win, int x, int y);
void window_resize(window_t *win, int width, int height);
void window_set_title(window_t *win, const char *title);
void window_set_color(window_t *win, uint32_t bg_color, uint32_t border_color);
void window_invalidate(window_t *win);
void window_invalidate_rect(window_t *win, rect_t *rect);
void window_set_focus(window_t *win);
window_t *window_get_focused(void);
void window_bring_to_front(window_t *win);
void window_send_to_back(window_t *win);

/* 获取窗口在指定坐标处的最顶层窗口 */
window_t *window_get_at_point(int x, int y);

/* 检测点是否在标题栏上 */
int window_hit_test_titlebar(window_t *win, int x, int y);

/* 检测点是否在关闭按钮上 */
int window_hit_test_close_button(window_t *win, int x, int y);

/* 窗口绘制函数（由GUI系统调用） */
void window_draw(window_t *win);
void window_draw_titlebar(window_t *win);
void window_draw_border(window_t *win);

/* ==========================================
 * 事件处理函数
 * ========================================== */
void event_init(void);
void event_poll(void);
void event_push(event_t *event);
int event_pop(event_t *event);
void event_dispatch(event_t *event);
void event_set_mouse_callback(void (*callback)(int x, int y, int button));
void event_set_keyboard_callback(void (*callback)(uint32_t key_code, int is_pressed));

/* ==========================================
 * GUI主循环
 * ========================================== */
void gui_init(void);
void gui_update(void);
void gui_main_loop(void);

/* ==========================================
 * 控件函数
 * ========================================== */
void button_draw(window_t *btn, int x, int y, int width, int height, const char *text);
int button_hit_test(window_t *btn, int x, int y);

/* ==========================================
 * 实用函数
 * ========================================== */
int rect_contains_point(rect_t *rect, int x, int y);
int rects_intersect(rect_t *a, rect_t *b);
void rect_union(rect_t *a, rect_t *b, rect_t *result);

#endif /* GUI_H */