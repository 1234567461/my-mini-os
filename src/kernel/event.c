/* ==========================================
 * Event - 事件处理实现 v0.8.0
 * 功能：鼠标、键盘事件管理和分发
 * ========================================== */

#include "gui.h"
#include "heap.h"
#include "string.h"
#include "mouse.h"

/* 事件队列大小 */
#define EVENT_QUEUE_SIZE 64

/* 事件队列 */
static event_t event_queue[EVENT_QUEUE_SIZE];
static int event_head = 0;
static int event_tail = 0;
static int event_count = 0;

/* 鼠标状态 */
static int mouse_x = 0;
static int mouse_y = 0;
static int mouse_button = 0;
static int mouse_prev_button = 0;

/* 鼠标按键按下时的窗口 */
static void *mouse_down_window = NULL;

/* 回调函数指针 */
static void (*mouse_callback)(int x, int y, int button) = NULL;
static void (*keyboard_callback)(uint32_t key_code, int is_pressed) = NULL;

/* 拖动状态 */
static int is_dragging = 0;
static void *drag_window = NULL;
static int drag_offset_x = 0;
static int drag_offset_y = 0;

/* ==========================================
 * 初始化事件系统
 * ========================================== */
void event_init(void) {
    event_head = 0;
    event_tail = 0;
    event_count = 0;

    mouse_x = GUI_WIDTH / 2;
    mouse_y = GUI_HEIGHT / 2;
    mouse_button = 0;
    mouse_prev_button = 0;
    mouse_down_window = NULL;

    is_dragging = 0;
    drag_window = NULL;

    mouse_callback = NULL;
    keyboard_callback = NULL;
}

/* ==========================================
 * 推送事件到队列
 * ========================================== */
void event_push(event_t *event) {
    if (!event || event_count >= EVENT_QUEUE_SIZE) return;

    /* 复制事件 */
    event_queue[event_tail] = *event;

    /* 更新队列尾指针 */
    event_tail = (event_tail + 1) % EVENT_QUEUE_SIZE;
    event_count++;
}

/* ==========================================
 * 从队列取出事件
 * ========================================== */
int event_pop(event_t *event) {
    if (event_count <= 0) return 0;

    /* 取出事件 */
    *event = event_queue[event_head];

    /* 更新队列头指针 */
    event_head = (event_head + 1) % EVENT_QUEUE_SIZE;
    event_count--;

    return 1;
}

/* ==========================================
 * 轮询事件（从硬件获取）
 * ========================================== */
void event_poll(void) {
    event_t event;
    event.type = EVENT_NONE;

    /* 使用mouse状态获取鼠标数据 */
    extern void mouse_get_state(mouse_state_t* state);
    mouse_state_t mouse_state;
    mouse_get_state(&mouse_state);

    int new_mouse_x = (int)mouse_state.x;
    int new_mouse_y = (int)mouse_state.y;
    int new_mouse_button = mouse_state.buttons;

    /* 检测鼠标移动 */
    if (new_mouse_x != mouse_x || new_mouse_y != mouse_y) {
        mouse_x = new_mouse_x;
        mouse_y = new_mouse_y;

        event.type = EVENT_MOUSE_MOVE;
        event.mouse_x = mouse_x;
        event.mouse_y = mouse_y;
        event.mouse_button = mouse_button;
        event.window = NULL;

        event_push(&event);
    }

    /* 检测鼠标按钮状态变化 */
    if (new_mouse_button != mouse_button) {
        if (new_mouse_button && !mouse_prev_button) {
            /* 按下 */
            event.type = EVENT_MOUSE_DOWN;
            event.mouse_x = mouse_x;
            event.mouse_y = mouse_y;
            event.mouse_button = new_mouse_button;
            event.window = NULL;
            event_push(&event);
        } else if (!new_mouse_button && mouse_prev_button) {
            /* 释放 */
            event.type = EVENT_MOUSE_UP;
            event.mouse_x = mouse_x;
            event.mouse_y = mouse_y;
            event.mouse_button = 0;
            event.window = NULL;
            event_push(&event);
        }

        mouse_prev_button = mouse_button;
        mouse_button = new_mouse_button;
    }
}

/* ==========================================
 * 分发事件
 * ========================================== */
void event_dispatch(event_t *event) {
    window_t *win;
    rect_t titlebar_rect;

    switch (event->type) {
        case EVENT_MOUSE_MOVE:
            /* 如果正在拖动窗口 */
            if (is_dragging && drag_window) {
                window_t *drag_win = (window_t *)drag_window;
                int new_x = event->mouse_x - drag_offset_x;
                int new_y = event->mouse_y - drag_offset_y;
                window_move(drag_win, new_x, new_y);
            } else {
                /* 查找鼠标下的窗口 */
                win = window_get_at_point(event->mouse_x, event->mouse_y);
                if (win && win->proc) {
                    event->window = win;
                    win->proc(win, event);
                }
            }

            /* 调用鼠标回调 */
            if (mouse_callback) {
                mouse_callback(event->mouse_x, event->mouse_y, event->mouse_button);
            }
            break;

        case EVENT_MOUSE_DOWN:
            /* 查找点击的窗口 */
            win = window_get_at_point(event->mouse_x, event->mouse_y);
            if (win) {
                /* 检查是否点击关闭按钮 */
                if (window_hit_test_close_button(win, event->mouse_x, event->mouse_y)) {
                    /* 发送关闭事件 */
                    event_t close_event;
                    close_event.type = EVENT_WINDOW_CLOSE;
                    close_event.window = win;
                    event->window = win;
                    if (win->proc) {
                        win->proc(win, &close_event);
                    }
                    break;
                }

                /* 检查是否点击标题栏 */
                if (window_hit_test_titlebar(win, event->mouse_x, event->mouse_y)) {
                    if (win->flags & WINDOW_FLAG_MOVABLE) {
                        /* 开始拖动 */
                        is_dragging = 1;
                        drag_window = win;
                        drag_offset_x = event->mouse_x - win->bounds.x;
                        drag_offset_y = event->mouse_y - win->bounds.y;
                    }
                }

                /* 设置焦点 */
                window_set_focus(win);

                /* 发送鼠标按下事件到窗口 */
                event->window = win;
                if (win->proc) {
                    win->proc(win, event);
                }

                mouse_down_window = win;
            }
            break;

        case EVENT_MOUSE_UP:
            /* 结束拖动 */
            if (is_dragging) {
                is_dragging = 0;
                drag_window = NULL;
            }

            /* 发送鼠标释放事件 */
            win = (window_t *)mouse_down_window;
            if (win) {
                event->window = win;
                if (win->proc) {
                    win->proc(win, event);
                }
                mouse_down_window = NULL;
            }
            break;

        case EVENT_KEY_DOWN:
        case EVENT_KEY_UP:
            /* 发送键盘事件到焦点窗口 */
            win = window_get_focused();
            if (win && win->proc) {
                event->window = win;
                win->proc(win, event);
            }

            /* 调用键盘回调 */
            if (keyboard_callback) {
                keyboard_callback(event->key_code, event->type == EVENT_KEY_DOWN);
            }
            break;

        case EVENT_WINDOW_CLOSE:
            /* 关闭窗口 */
            win = (window_t *)event->window;
            if (win) {
                window_destroy(win);
            }
            break;

        case EVENT_WINDOW_RESIZE:
        case EVENT_WINDOW_MOVE:
            win = (window_t *)event->window;
            if (win && win->proc) {
                win->proc(win, event);
            }
            break;

        default:
            break;
    }
}

/* ==========================================
 * 设置鼠标回调
 * ========================================== */
void event_set_mouse_callback(void (*callback)(int x, int y, int button)) {
    mouse_callback = callback;
}

/* ==========================================
 * 设置键盘回调
 * ========================================== */
void event_set_keyboard_callback(void (*callback)(uint32_t key_code, int is_pressed)) {
    keyboard_callback = callback;
}

/* ==========================================
 * 模拟按键事件（供测试用）
 * ========================================== */
void event_simulate_key(uint32_t key_code, int is_pressed) {
    event_t event;
    event.type = is_pressed ? EVENT_KEY_DOWN : EVENT_KEY_UP;
    event.key_code = key_code;
    event.window = NULL;

    event_push(&event);
}

/* ==========================================
 * 模拟鼠标点击（供测试用）
 * ========================================== */
void event_simulate_mouse_click(int x, int y, int button) {
    event_t event_down, event_up;

    /* 鼠标按下 */
    event_down.type = EVENT_MOUSE_DOWN;
    event_down.mouse_x = x;
    event_down.mouse_y = y;
    event_down.mouse_button = button;
    event_down.window = NULL;
    event_push(&event_down);

    /* 鼠标释放 */
    event_up.type = EVENT_MOUSE_UP;
    event_up.mouse_x = x;
    event_up.mouse_y = y;
    event_up.mouse_button = 0;
    event_up.window = NULL;
    event_push(&event_up);
}