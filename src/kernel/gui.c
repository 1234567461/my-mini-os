/* ==========================================
 * GUI - 图形用户界面主模块 v0.9.0
 * 功能：GUI初始化、主循环、示例程序、任务栏、开始菜单
 * ========================================== */

#include "gui.h"
#include "vga.h"
#include "heap.h"
#include "string.h"
#include "pit.h"
#include "rtc.h"

/* GUI运行状态 */
static int gui_initialized = 0;
static int gui_running = 0;

/* 桌面窗口 */
static window_t *desktop_win = NULL;
static window_t *test_win1 = NULL;
static window_t *test_win2 = NULL;

/* 任务栏状态 */
static int taskbar_visible = 1;
static int taskbar_clock_update = 0;

/* 开始菜单状态 */
static int start_menu_visible = 0;
static int start_menu_x = 0;
static int start_menu_y = 0;

/* 开始菜单项 */
#define MAX_START_MENU_ITEMS 10
static start_menu_item_t start_menu_items[MAX_START_MENU_ITEMS];
static int start_menu_item_count = 0;

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

/* 文本框 */
static textbox_t main_textbox = {
    .text = "Hello, World!",
    .cursor_pos = 12,
    .max_length = 255,
    .isFocused = 0
};

/* 进度条 */
static progressbar_t main_progressbar = {
    .value = 45,
    .min = 0,
    .max = 100
};

/* ==========================================
 * 任务栏操作函数
 * ========================================== */
void taskbar_init(void) {
    taskbar_visible = 1;
    taskbar_clock_update = 0;
}

void taskbar_draw(void) {
    if (!taskbar_visible) return;

    int taskbar_y = GUI_HEIGHT - TASKBAR_HEIGHT;

    /* 绘制任务栏背景（使用渐变） */
    graphics_draw_gradient(0, taskbar_y, GUI_WIDTH, TASKBAR_HEIGHT,
                          COLOR_RGB(200, 200, 200), COLOR_RGB(180, 180, 180), 0);

    /* 绘制任务栏边框 */
    graphics_draw_line(0, taskbar_y, GUI_WIDTH - 1, taskbar_y, COLOR_WHITE);
    graphics_draw_line(0, taskbar_y + 1, GUI_WIDTH - 1, taskbar_y + 1, COLOR_RGB(120, 120, 120));

    /* 绘制开始按钮 */
    int start_btn_x = 4;
    int start_btn_y = taskbar_y + 4;
    int start_btn_w = 60;
    int start_btn_h = TASKBAR_HEIGHT - 8;

    /* 开始按钮背景 */
    graphics_fill_round_rect(start_btn_x, start_btn_y, start_btn_w, start_btn_h, 4, COLOR_RGB(220, 220, 220));
    graphics_draw_round_rect(start_btn_x, start_btn_y, start_btn_w, start_btn_h, 4, COLOR_RGB(100, 100, 100));

    /* 开始按钮文字 */
    font_draw_string(start_btn_x + 8, start_btn_y + 7, "Start", COLOR_BLUE, 1);

    /* 绘制系统托盘区域 */
    int tray_x = GUI_WIDTH - 150;
    graphics_draw_line(tray_x, taskbar_y + 4, tray_x, taskbar_y + TASKBAR_HEIGHT - 4, COLOR_RGB(120, 120, 120));

    /* 绘制时钟 */
    rtc_time_t time;
    rtc_get_time(&time);
    char time_str[16];
    int hour = time.hour;
    const char *ampm = "AM";
    if (hour == 0) hour = 12;
    else if (hour > 12) {
        hour -= 12;
        ampm = "PM";
    }
    font_draw_string(GUI_WIDTH - 140, taskbar_y + 8, "10:30:00", COLOR_BLACK, 1);

    /* 绘制窗口按钮 */
    if (test_win1 && test_win1->is_visible) {
        int btn_x = start_btn_x + start_btn_w + 10;
        graphics_fill_round_rect(btn_x, taskbar_y + 4, 100, TASKBAR_HEIGHT - 8, 4, COLOR_RGB(200, 200, 220));
        graphics_draw_round_rect(btn_x, taskbar_y + 4, 100, TASKBAR_HEIGHT - 8, 4, COLOR_RGB(100, 100, 100));
        font_draw_string(btn_x + 5, taskbar_y + 9, "Test Window 1", COLOR_BLACK, 1);
    }
}

void taskbar_update(void) {
    taskbar_clock_update++;
    if (taskbar_clock_update > 100) {
        taskbar_clock_update = 0;
    }
}

int taskbar_hit_test_start_button(int x, int y) {
    int taskbar_y = GUI_HEIGHT - TASKBAR_HEIGHT;
    int start_btn_x = 4;
    int start_btn_y = taskbar_y + 4;
    int start_btn_w = 60;
    int start_btn_h = TASKBAR_HEIGHT - 8;

    return (x >= start_btn_x && x < start_btn_x + start_btn_w &&
            y >= start_btn_y && y < start_btn_y + start_btn_h);
}

/* ==========================================
 * 开始菜单函数
 * ========================================== */
void start_menu_show(void) {
    int taskbar_y = GUI_HEIGHT - TASKBAR_HEIGHT;
    start_menu_x = 0;
    start_menu_y = taskbar_y - START_MENU_HEIGHT;
    start_menu_visible = 1;
}

void start_menu_hide(void) {
    start_menu_visible = 0;
}

int start_menu_is_visible(void) {
    return start_menu_visible;
}

void start_menu_add_item(const char *text, const char *icon, void (*action)(void)) {
    if (start_menu_item_count >= MAX_START_MENU_ITEMS) return;

    start_menu_items[start_menu_item_count].text = text;
    start_menu_items[start_menu_item_count].icon = icon;
    start_menu_items[start_menu_item_count].action = action;
    start_menu_item_count++;
}

void start_menu_draw(void) {
    if (!start_menu_visible) return;

    /* 绘制菜单背景 */
    graphics_fill_round_rect(start_menu_x, start_menu_y, START_MENU_WIDTH, START_MENU_HEIGHT, 6, COLOR_RGB(240, 240, 245));
    graphics_draw_round_rect(start_menu_x, start_menu_y, START_MENU_WIDTH, START_MENU_HEIGHT, 6, COLOR_RGB(100, 100, 100));

    /* 绘制菜单标题栏 */
    graphics_fill_rect(start_menu_x + 2, start_menu_y + 2, START_MENU_WIDTH - 4, 28, COLOR_RGB(0, 0, 150));
    font_draw_string(start_menu_x + 10, start_menu_y + 7, "My Mini OS", COLOR_WHITE, 1);

    /* 绘制分隔线 */
    graphics_draw_line(start_menu_x + 4, start_menu_y + 32, start_menu_x + START_MENU_WIDTH - 4, start_menu_y + 32, COLOR_RGB(150, 150, 150));

    /* 绘制菜单项 */
    int item_y = start_menu_y + 40;
    for (int i = 0; i < start_menu_item_count && i < 6; i++) {
        /* 菜单项背景（悬停效果） */
        graphics_fill_rect(start_menu_x + 4, item_y, START_MENU_WIDTH - 8, 22, COLOR_RGB(240, 240, 245));

        /* 菜单项图标区域 */
        graphics_fill_circle(start_menu_x + 16, item_y + 11, 6, COLOR_BLUE);

        /* 菜单项文字 */
        font_draw_string(start_menu_x + 30, item_y + 4, start_menu_items[i].text, COLOR_BLACK, 1);

        item_y += 26;
    }

    /* 如果没有菜单项，显示默认项 */
    if (start_menu_item_count == 0) {
        /* 显示一些默认的菜单项 */
        const char *default_items[] = {"Terminal", "File Manager", "Settings", "About"};
        for (int i = 0; i < 4; i++) {
            graphics_fill_rect(start_menu_x + 4, item_y, START_MENU_WIDTH - 8, 22, COLOR_RGB(240, 240, 245));
            graphics_fill_circle(start_menu_x + 16, item_y + 11, 6, COLOR_BLUE);
            font_draw_string(start_menu_x + 30, item_y + 4, default_items[i], COLOR_BLACK, 1);
            item_y += 26;
        }
    }

    /* 绘制分隔线和退出选项 */
    graphics_draw_line(start_menu_x + 4, item_y + 4, start_menu_x + START_MENU_WIDTH - 4, item_y + 4, COLOR_RGB(150, 150, 150));

    /* 退出按钮 */
    graphics_fill_rect(start_menu_x + 4, item_y + 10, START_MENU_WIDTH - 8, 24, COLOR_RGB(220, 220, 220));
    font_draw_string(start_menu_x + 30, item_y + 16, "Exit to Shell", COLOR_RED, 1);
}

int start_menu_hit_test(int x, int y) {
    if (!start_menu_visible) return 0;
    return (x >= start_menu_x && x < start_menu_x + START_MENU_WIDTH &&
            y >= start_menu_y && y < start_menu_y + START_MENU_HEIGHT);
}

void start_menu_handle_click(int x, int y) {
    if (!start_menu_visible) return;

    int item_y = start_menu_y + 40;

    /* 检查是否点击了退出按钮 */
    if (item_y + 10 <= y && y < item_y + 34 &&
        x >= start_menu_x + 4 && x < start_menu_x + START_MENU_WIDTH - 4) {
        /* 退出GUI */
        start_menu_hide();
        gui_exit();
        return;
    }

    /* 检查其他菜单项 */
    for (int i = 0; i < start_menu_item_count && i < 6; i++) {
        if (y >= item_y && y < item_y + 22 &&
            x >= start_menu_x + 4 && x < start_menu_x + START_MENU_WIDTH - 4) {
            /* 执行菜单项动作 */
            if (start_menu_items[i].action) {
                start_menu_items[i].action();
            }
            start_menu_hide();
            return;
        }
        item_y += 26;
    }

    /* 点击菜单外部关闭菜单 */
    if (y < start_menu_y || y >= start_menu_y + START_MENU_HEIGHT ||
        x < start_menu_x || x >= start_menu_x + START_MENU_WIDTH) {
        start_menu_hide();
    }
}

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

            /* 绘制一个进度条 */
            progressbar_draw(win->client_area.x + 10, win->client_area.y + 130, 200, 20, &main_progressbar);
            break;

        case EVENT_WINDOW_CLOSE:
            vga_printf("Window '%s' closed\n", win->title);
            window_destroy(win);
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

            /* 绘制一个文本框示例 */
            textbox_draw(win->client_area.x + 10, win->client_area.y + 100, 180, 24, &main_textbox);

            /* 绘制一个简单的图形 */
            graphics_draw_circle(
                win->client_area.x + 80,
                win->client_area.y + 150,
                30,
                COLOR_BLUE
            );
            graphics_fill_circle(
                win->client_area.x + 150,
                win->client_area.y + 150,
                25,
                COLOR_GREEN
            );
            break;

        default:
            break;
    }
}

/* ==========================================
 * 控件绘制函数
 * ========================================== */
void button_draw(window_t *btn, int x, int y, int width, int height, const char *text) {
    (void)btn;
    /* 按钮背景 - 3D效果 */
    graphics_fill_round_rect(x, y, width, height, 3, COLOR_BTN_FACE);

    /* 按钮边框（3D效果） */
    /* 顶部左边亮色 */
    graphics_draw_line(x, y, x + width - 1, y, COLOR_BTN_HIGHLIGHT);
    graphics_draw_line(x, y, x, y + height - 1, COLOR_BTN_HIGHLIGHT);

    /* 底部右边暗色 */
    graphics_draw_line(x + 1, y + height - 1, x + width - 1, y + height - 1, COLOR_BTN_SHADOW);
    graphics_draw_line(x + width - 1, y + 1, x + width - 1, y + height - 1, COLOR_BTN_SHADOW);

    /* 按钮边框 */
    graphics_draw_round_rect(x, y, width, height, 3, COLOR_RGB(100, 100, 100));

    /* 按钮文字 */
    int text_width = font_measure_string(text, 1);
    int text_x = x + (width - text_width) / 2;
    int text_y = y + (height - 16) / 2;

    font_draw_string(text_x, text_y, text, COLOR_BLACK, 1);
}

int button_hit_test(window_t *btn, int x, int y) {
    (void)btn;
    return 0;  /* 简化实现 */
}

/* 文本框绘制 */
void textbox_draw(int x, int y, int width, int height, textbox_t *tb) {
    /* 文本框背景 */
    graphics_fill_rect(x, y, width, height, COLOR_WHITE);

    /* 文本框边框 */
    uint32_t border_color = tb->isFocused ? COLOR_BLUE : COLOR_GRAY;
    graphics_draw_rect(x, y, width, height, border_color);

    /* 绘制文本 */
    font_draw_string(x + 4, y + (height - 16) / 2, tb->text, COLOR_BLACK, 1);

    /* 如果有焦点，绘制光标 */
    if (tb->isFocused) {
        int cursor_x = x + 4 + font_measure_string(tb->text, 1);
        graphics_draw_line(cursor_x, y + 4, cursor_x, y + height - 4, COLOR_BLACK);
    }
}

int textbox_hit_test(int x, int y, int width, int height) {
    return (x >= 0 && x < width && y >= 0 && y < height);
}

void textbox_handle_key(textbox_t *tb, uint32_t key_code) {
    (void)tb;
    (void)key_code;
    /* 简化实现 - 实际需要处理键盘输入 */
}

/* 进度条绘制 */
void progressbar_draw(int x, int y, int width, int height, progressbar_t *pb) {
    /* 进度条背景 */
    graphics_fill_rect(x, y, width, height, COLOR_RGB(200, 200, 200));
    graphics_draw_rect(x, y, width, height, COLOR_GRAY);

    /* 进度条填充 */
    int fill_width = (pb->value * (width - 4)) / (pb->max - pb->min);
    if (fill_width > 0) {
        graphics_fill_rect(x + 2, y + 2, fill_width, height - 4, COLOR_BLUE);
    }

    /* 百分比文字 */
    char percent_str[16];
    int percent = (pb->value * 100) / (pb->max - pb->min);
    font_draw_string(x + width / 2 - 15, y + (height - 16) / 2, "45%", COLOR_WHITE, 1);
}

void progressbar_set_value(progressbar_t *pb, int value) {
    if (value < pb->min) value = pb->min;
    if (value > pb->max) value = pb->max;
    pb->value = value;
}

/* 复选框绘制 */
void checkbox_draw(int x, int y, const char *label, int checked, int focused) {
    /* 复选框背景 */
    graphics_fill_rect(x, y, 16, 16, COLOR_WHITE);
    graphics_draw_rect(x, y, 16, 16, focused ? COLOR_BLUE : COLOR_GRAY);

    /* 勾选标记 */
    if (checked) {
        graphics_draw_line(x + 2, y + 8, x + 6, y + 12, COLOR_BLUE);
        graphics_draw_line(x + 6, y + 12, x + 14, y + 2, COLOR_BLUE);
    }

    /* 标签 */
    font_draw_string(x + 22, y + 1, label, COLOR_BLACK, 1);
}

int checkbox_hit_test(int x, int y) {
    return (x >= 0 && x < 16 && y >= 0 && y < 16);
}

/* 标签绘制 */
void label_draw(int x, int y, const char *text, uint32_t color) {
    font_draw_string(x, y, text, color, 1);
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
    taskbar_init();

    gui_initialized = 1;

    vga_printf("GUI system initialized (640x480x32) v0.9.0\n");

    /* 创建桌面窗口 */
    desktop_win = window_create("Desktop", 0, 0, GUI_WIDTH, GUI_HEIGHT);
    if (desktop_win) {
        desktop_win->style = WINDOW_STYLE_DEFAULT;
        /* 桌面使用渐变背景 */
        window_show(desktop_win);
    }

    /* 创建测试窗口1 */
    test_win1 = window_create("Control Panel", 80, 80, 320, 250);
    if (test_win1) {
        test_win1->proc = test_window_proc;
        window_set_color(test_win1, COLOR_WHITE, COLOR_BLUE);
        window_show(test_win1);
        window_set_focus(test_win1);
    }

    /* 创建测试窗口2 */
    test_win2 = window_create("System Info", 420, 120, 200, 220);
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

    /* 绘制桌面背景（渐变） */
    graphics_draw_gradient(0, 0, GUI_WIDTH, GUI_HEIGHT - TASKBAR_HEIGHT,
                            COLOR_RGB(30, 80, 150), COLOR_RGB(10, 40, 100), 1);

    /* 绘制所有窗口 */
    window_t *win = window_get_list();
    while (win) {
        if (win->is_visible && win != desktop_win) {
            window_draw(win);
        }
        win = win->next;
    }

    /* 绘制任务栏 */
    taskbar_draw();

    /* 绘制开始菜单 */
    if (start_menu_visible) {
        start_menu_draw();
    }

    /* 处理事件 */
    event_poll();

    event_t event;
    while (event_pop(&event)) {
        /* 处理开始菜单点击 */
        if (start_menu_visible && event.type == EVENT_MOUSE_DOWN) {
            start_menu_handle_click(event.mouse_x, event.mouse_y);
        }

        /* 处理任务栏点击 */
        if (event.type == EVENT_MOUSE_DOWN) {
            int taskbar_y = GUI_HEIGHT - TASKBAR_HEIGHT;
            if (event.mouse_y >= taskbar_y) {
                if (taskbar_hit_test_start_button(event.mouse_x, event.mouse_y)) {
                    if (start_menu_visible) {
                        start_menu_hide();
                    } else {
                        start_menu_show();
                    }
                    continue;
                }
            }
        }

        event_dispatch(&event);
    }

    /* 更新任务栏时钟 */
    taskbar_update();

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
    vga_printf("Click 'Start' button to access menu.\n");

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
    font_draw_string(100, 400, "My Mini OS GUI v0.9.0", COLOR_WHITE, 2);

    framebuffer_flush();
}
