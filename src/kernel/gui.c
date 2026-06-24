/* ==========================================
 * GUI - 图形用户界面主模块 v0.8.0
 * 功能：GUI初始化、主循环、示例程序
 * ========================================== */

#include "gui.h"
#include "vga.h"
#include "heap.h"
#include "string.h"
#include "pit.h"

/* GUI运行状态 */
static int gui_initialized = 0;
static int gui_running = 0;

/* 桌面窗口 */
static window_t *desktop_win = NULL;
static window_t *test_win1 = NULL;
static window_t *test_win2 = NULL;

/* 控件区域定义 */
typedef struct {
    int x, y, width, height;
    const char *text;
} button_t;

static button_t test_buttons[] = {
    {10, 30, 80, 24, "OK"},
    {100, 30, 80, 24, "Cancel"},
    {190, 30, 80, 24, "Apply"}
};
#define BUTTON_COUNT (sizeof(test_buttons) / sizeof(button_t))

/* ==========================================
 * 示例窗口过程函数
 * ========================================== */
static void test_window_proc(window_t *win, event_t *event) {
    switch (event->type) {
        case EVENT_MOUSE_DOWN:
            /* 检测按钮点击 */
            for (int i = 0; i < BUTTON_COUNT; i++) {
                button_t *btn = &test_buttons[i];
                int abs_x = win->client_area.x + btn->x;
                int abs_y = win->client_area.y + btn->y;

                if (event->mouse_x >= abs_x && event->mouse_x < abs_x + btn->width &&
                    event->mouse_y >= abs_y && event->mouse_y < abs_y + btn->height) {
                    /* 按钮被点击 */
                    vga_printf("Button '%s' clicked!\n", btn->text);
                }
            }
            break;

        case EVENT_PAINT:
            /* 重绘窗口内容 */
            /* 绘制按钮 */
            for (int i = 0; i < BUTTON_COUNT; i++) {
                button_t *btn = &test_buttons[i];
                int abs_x = win->client_area.x + btn->x;
                int abs_y = win->client_area.y + btn->y;
                button_draw(win, abs_x, abs_y, btn->width, btn->height, btn->text);
            }

            /* 绘制示例文本 */
            font_draw_string(win->client_area.x + 10, win->client_area.y + 70,
                            "Welcome to My Mini OS GUI!", COLOR_BLACK, 1);
            font_draw_string(win->client_area.x + 10, win->client_area.y + 90,
                            "Click buttons to test.", COLOR_DARK_GRAY, 1);
            font_draw_string(win->client_area.x + 10, win->client_area.y + 110,
                            "Mouse: Click titlebar to drag.", COLOR_DARK_GRAY, 1);
            break;

        case EVENT_WINDOW_CLOSE:
            vga_printf("Window '%s' closed\n", win->title);
            break;

        default:
            break;
    }
}

/* ==========================================
 * 第二个测试窗口过程
 * ========================================== */
static void test_window2_proc(window_t *win, event_t *event) {
    switch (event->type) {
        case EVENT_PAINT:
            /* 绘制窗口内容 */
            font_draw_string(win->client_area.x + 10, win->client_area.y + 10,
                            "Info Window", COLOR_BLUE, 1);
            font_draw_string(win->client_area.x + 10, win->client_area.y + 40,
                            "This is a second window.", COLOR_BLACK, 1);
            font_draw_string(win->client_area.x + 10, win->client_area.y + 60,
                            "You can move it by dragging", COLOR_DARK_GRAY, 1);
            font_draw_string(win->client_area.x + 10, win->client_area.y + 75,
                            "the title bar.", COLOR_DARK_GRAY, 1);

            /* 绘制一个简单的图形 */
            graphics_draw_circle(
                win->client_area.x + 80,
                win->client_area.y + 120,
                30,
                COLOR_BLUE
            );
            graphics_fill_circle(
                win->client_area.x + 150,
                win->client_area.y + 120,
                25,
                COLOR_GREEN
            );
            break;

        default:
            break;
    }
}

/* ==========================================
 * 绘制按钮
 * ========================================== */
void button_draw(window_t *btn, int x, int y, int width, int height, const char *text) {
    /* 按钮背景 */
    graphics_fill_rect(x, y, width, height, COLOR_RGB(220, 220, 220));

    /* 按钮边框（3D效果） */
    /* 顶部左边亮色 */
    graphics_draw_line(x, y, x + width - 1, y, COLOR_WHITE);
    graphics_draw_line(x, y, x, y + height - 1, COLOR_WHITE);

    /* 底部右边暗色 */
    graphics_draw_line(x + 1, y + height - 1, x + width - 1, y + height - 1, COLOR_GRAY);
    graphics_draw_line(x + width - 1, y + 1, x + width - 1, y + height - 1, COLOR_GRAY);

    /* 按钮边框 */
    graphics_draw_rect(x, y, width, height, COLOR_BLACK);

    /* 按钮文字 */
    int text_width = font_measure_string(text, 1);
    int text_x = x + (width - text_width) / 2;
    int text_y = y + (height - 16) / 2;

    font_draw_string(text_x, text_y, text, COLOR_BLACK, 1);
}

/* ==========================================
 * 按钮命中测试
 * ========================================== */
int button_hit_test(window_t *btn, int x, int y) {
    (void)btn;
    return 0;  /* 简化实现 */
}

/* ==========================================
 * 初始化GUI系统
 * ========================================== */
void gui_init(void) {
    if (gui_initialized) return;

    /* 初始化各个子系统 */
    framebuffer_init();
    graphics_init();
    font_init();
    window_init();
    event_init();

    gui_initialized = 1;

    vga_printf("GUI system initialized (640x480x32)\n");

    /* 创建桌面窗口 */
    desktop_win = window_create("Desktop", 0, 0, GUI_WIDTH, GUI_HEIGHT);
    if (desktop_win) {
        window_set_color(desktop_win, COLOR_RGB(0, 128, 192), COLOR_BLACK);
        window_show(desktop_win);
    }

    /* 创建测试窗口1 */
    test_win1 = window_create("Test Window 1", 100, 100, 300, 200);
    if (test_win1) {
        test_win1->proc = test_window_proc;
        window_set_color(test_win1, COLOR_WHITE, COLOR_BLUE);
        window_show(test_win1);
        window_set_focus(test_win1);
    }

    /* 创建测试窗口2 */
    test_win2 = window_create("Info", 450, 150, 180, 180);
    if (test_win2) {
        test_win2->proc = test_window2_proc;
        window_set_color(test_win2, COLOR_RGB(240, 240, 240), COLOR_GREEN);
        window_show(test_win2);
    }

    vga_printf("GUI demo windows created\n");
}

/* ==========================================
 * 更新GUI（每帧调用）
 * ========================================== */
void gui_update(void) {
    if (!gui_initialized) return;

    /* 处理事件 */
    event_poll();

    event_t event;
    while (event_pop(&event)) {
        event_dispatch(&event);
    }

    /* 刷新帧缓冲 */
    framebuffer_flush();
}

/* ==========================================
 * GUI主循环
 * ========================================== */
void gui_main_loop(void) {
    if (!gui_initialized) {
        gui_init();
    }

    gui_running = 1;

    vga_printf("Entering GUI main loop...\n");

    /* 主循环 */
    while (gui_running) {
        gui_update();

        /* 简单的延迟，防止过度占用CPU */
        for (int i = 0; i < 1000; i++) {
            asm volatile ("nop");
        }

        /* 让出CPU给其他进程 */
        task_yield();
    }
}

/* ==========================================
 * 退出GUI
 * ========================================== */
void gui_exit(void) {
    gui_running = 0;
}

/* ==========================================
 * 测试：绘制演示图形
 * ========================================== */
void gui_draw_demo(void) {
    /* 绘制演示图形到屏幕中心 */
    int center_x = GUI_WIDTH / 2;
    int center_y = GUI_HEIGHT / 2;

    /* 画一些测试图形 */
    graphics_draw_rect(50, 50, 200, 100, COLOR_RED);
    graphics_fill_rect(300, 50, 150, 80, COLOR_GREEN);
    graphics_draw_circle(center_x, center_y, 50, COLOR_BLUE);
    graphics_fill_circle(center_x + 100, center_y, 30, COLOR_YELLOW);

    /* 画测试文字 */
    font_draw_string(100, 400, "My Mini OS GUI v0.8.0", COLOR_WHITE, 2);

    framebuffer_flush();
}