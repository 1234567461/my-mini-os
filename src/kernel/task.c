/* ==========================================
 * 进程管理实现
 * 调度算法：时间片轮转（Round Robin）
 * ========================================== */
#include "task.h"
#include "heap.h"
#include "string.h"
#include "string.h"
#include "pit.h"
#include "types.h"

/* 最大进程数 */
#define MAX_TASKS       64

/* 默认栈大小（4KB） */
#define DEFAULT_STACK_SIZE  4096

/* 默认时间片（10个tick，即100ms @ 100Hz） */
#define DEFAULT_TIME_SLICE  10

/* 进程链表 */
static task_t *task_list = NULL;

/* 当前运行的进程 */
static task_t *current_task = NULL;

/* 下一个PID */
static uint32_t next_pid = 1;

/* 进程数量 */
static uint32_t task_count = 0;

/* 空闲进程（idle） */
static task_t *idle_task = NULL;

/* ==========================================
 * 空闲进程入口
 * ========================================== */
static void idle_task_entry() {
    while (1) {
        hlt();  /* 停机等待中断 */
    }
}

/* ==========================================
 * 进程退出包装
 * ========================================== */
static void task_exit_wrapper() {
    task_exit();
}

/* ==========================================
 * 初始化进程调度器
 * ========================================== */
void task_init() {
    /* 创建idle进程 */
    idle_task = (task_t *)kmalloc(sizeof(task_t));
    if (idle_task == NULL) {
        return;
    }

    memset(idle_task, 0, sizeof(task_t));
    idle_task->pid = 0;
    strcpy(idle_task->name, "idle");
    idle_task->state = TASK_READY;
    idle_task->priority = 0;
    idle_task->time_slice = 1;
    idle_task->remaining_time = 1;

    /* 分配栈 */
    idle_task->stack_size = DEFAULT_STACK_SIZE;
    idle_task->stack = (uint32_t *)kmalloc(DEFAULT_STACK_SIZE);
    if (idle_task->stack == NULL) {
        kfree(idle_task);
        return;
    }

    /* 设置栈顶（栈从高地址向低地址增长） */
    uint32_t *stack_top = (uint32_t *)((uint32_t)idle_task->stack + DEFAULT_STACK_SIZE);

    /* 初始化上下文 */
    idle_task->context.eip = (uint32_t)idle_task_entry;
    idle_task->context.cs = 0x08;  /* 代码段选择子 */
    idle_task->context.eflags = 0x202;  /* 开中断 */
    idle_task->context.esp = (uint32_t)stack_top;
    idle_task->context.ss = 0x10;  /* 数据段选择子 */

    /* 加入链表 */
    idle_task->next = NULL;
    task_list = idle_task;
    current_task = idle_task;
    task_count = 1;
}

/* ==========================================
 * 创建新进程
 * 返回：0成功，-1失败
 * ========================================== */
int task_create(const char *name, void (*entry)(), uint32_t priority) {
    if (task_count >= MAX_TASKS) {
        return -1;
    }

    /* 分配PCB */
    task_t *task = (task_t *)kmalloc(sizeof(task_t));
    if (task == NULL) {
        return -1;
    }

    memset(task, 0, sizeof(task_t));

    /* 设置基本信息 */
    task->pid = next_pid++;
    strncpy(task->name, name, 31);
    task->name[31] = '\0';
    task->state = TASK_READY;
    task->priority = priority;
    task->time_slice = DEFAULT_TIME_SLICE + priority * 5;  /* 优先级高的时间片长 */
    task->remaining_time = task->time_slice;

    /* 分配栈 */
    task->stack_size = DEFAULT_STACK_SIZE;
    task->stack = (uint32_t *)kmalloc(DEFAULT_STACK_SIZE);
    if (task->stack == NULL) {
        kfree(task);
        return -1;
    }

    /* 设置栈顶（栈从高地址向低地址增长） */
    uint32_t *stack_top = (uint32_t *)((uint32_t)task->stack + DEFAULT_STACK_SIZE);

    /* 模拟函数调用栈：
     * 栈顶是返回地址（task_exit_wrapper），
     * 然后是进程入口函数地址
     */
    *(--stack_top) = (uint32_t)task_exit_wrapper;  /* 返回地址 */

    /* 初始化上下文 */
    task->context.eip = (uint32_t)entry;
    task->context.cs = 0x08;  /* 代码段选择子 */
    task->context.eflags = 0x202;  /* 开中断 */
    task->context.esp = (uint32_t)stack_top;
    task->context.ss = 0x10;  /* 数据段选择子 */

    /* 初始化通用寄存器 */
    task->context.eax = 0;
    task->context.ebx = 0;
    task->context.ecx = 0;
    task->context.edx = 0;
    task->context.esi = 0;
    task->context.edi = 0;
    task->context.ebp = 0;

    /* 加入链表（加到末尾） */
    task->next = NULL;
    if (task_list == NULL) {
        task_list = task;
    } else {
        task_t *last = task_list;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = task;
    }

    task_count++;
    return task->pid;
}

/* ==========================================
 * 进程调度（时间片轮转）
 * ========================================== */
void schedule() {
    if (task_list == NULL) {
        return;
    }

    /* 保存当前进程状态 */
    if (current_task != NULL && current_task->state == TASK_RUNNING) {
        current_task->state = TASK_READY;
    }

    /* 查找下一个就绪进程 */
    task_t *next = NULL;
    task_t *start = current_task ? current_task->next : task_list;

    /* 从当前位置开始查找 */
    task_t *p = start;
    while (p != NULL) {
        if (p->state == TASK_READY) {
            next = p;
            break;
        }
        p = p->next;
    }

    /* 如果没找到，从头开始找 */
    if (next == NULL) {
        p = task_list;
        while (p != start) {
            if (p->state == TASK_READY) {
                next = p;
                break;
            }
            p = p->next;
        }
    }

    /* 如果还是没找到，运行idle进程 */
    if (next == NULL) {
        next = idle_task;
    }

    /* 切换进程 */
    if (next != current_task) {
        task_t *prev = current_task;
        current_task = next;
        current_task->state = TASK_RUNNING;
        current_task->remaining_time = current_task->time_slice;

        /* 这里应该进行上下文切换（汇编实现）
         * 简化版：我们在时钟中断中处理
         */
    }
}

/* ==========================================
 * 主动放弃CPU
 * ========================================== */
void task_yield() {
    if (current_task != NULL) {
        current_task->remaining_time = 0;
        schedule();
    }
}

/* ==========================================
 * 当前进程睡眠
 * ========================================== */
void task_sleep(uint32_t ticks) {
    if (current_task == NULL || current_task == idle_task) {
        return;
    }

    /* 设置唤醒时间（简化版：直接用ticks计数） */
    current_task->state = TASK_SLEEPING;
    current_task->remaining_time = ticks;  /* 用remaining_time存睡眠ticks */

    schedule();
}

/* ==========================================
 * 唤醒进程
 * ========================================== */
void task_wakeup(task_t *task) {
    if (task != NULL && task->state == TASK_SLEEPING) {
        task->state = TASK_READY;
        task->remaining_time = task->time_slice;
    }
}

/* ==========================================
 * 获取当前进程
 * ========================================== */
task_t *get_current_task() {
    return current_task;
}

/* ==========================================
 * 获取进程列表
 * ========================================== */
task_t *get_task_list() {
    return task_list;
}

/* ==========================================
 * 获取进程数量
 * ========================================== */
uint32_t get_task_count() {
    return task_count;
}

/* ==========================================
 * 退出当前进程
 * ========================================== */
void task_exit() {
    if (current_task == NULL || current_task == idle_task) {
        return;
    }

    /* 设置为僵尸状态 */
    current_task->state = TASK_ZOMBIE;

    /* 释放栈 */
    if (current_task->stack != NULL) {
        kfree(current_task->stack);
        current_task->stack = NULL;
    }

    task_count--;

    schedule();

    /* 不应该到这里 */
    while (1) {
        hlt();
    }
}

/* ==========================================
 * 时钟中断处理（用于进程调度）
 * 在pit中断中调用
 * ========================================== */
void task_tick() {
    if (current_task == NULL) {
        return;
    }

    /* 处理睡眠进程 */
    task_t *p = task_list;
    while (p != NULL) {
        if (p->state == TASK_SLEEPING) {
            if (p->remaining_time > 0) {
                p->remaining_time--;
                if (p->remaining_time == 0) {
                    task_wakeup(p);
                }
            }
        }
        p = p->next;
    }

    /* 当前进程时间片减1 */
    if (current_task->state == TASK_RUNNING) {
        if (current_task->remaining_time > 0) {
            current_task->remaining_time--;
        }

        /* 时间片用完，调度 */
        if (current_task->remaining_time == 0) {
            schedule();
        }
    }
}
