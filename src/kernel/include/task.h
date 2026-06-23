/* ==========================================
 * 进程管理 - task.h v0.4.0
 * 功能增强：
 *   1. 每个进程独立的页目录（地址空间隔离）
 *   2. 用户栈和用户堆
 *   3. 内核/用户空间分离
 * ========================================== */
#ifndef TASK_H
#define TASK_H

#include "types.h"
#include "paging.h"

/* 进程状态 */
typedef enum {
    TASK_RUNNING,    /* 运行中 */
    TASK_READY,      /* 就绪 */
    TASK_BLOCKED,    /* 阻塞 */
    TASK_SLEEPING,   /* 睡眠 */
    TASK_ZOMBIE      /* 僵尸 */
} task_state_t;

/* 寄存器上下文（32位保护模式） */
typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t user_esp;
    uint32_t ss;
} task_context_t;

/* 用户内存空间信息 */
typedef struct {
    uint32_t code_start;    /* 代码段起始 */
    uint32_t code_size;     /* 代码段大小 */
    uint32_t heap_start;    /* 用户堆起始 */
    uint32_t heap_brk;      /* 用户堆当前边界 */
    uint32_t stack_start;   /* 用户栈起始（高地址） */
    uint32_t stack_size;    /* 用户栈大小 */
} user_space_t;

/* 进程控制块（PCB） */
typedef struct task_struct {
    uint32_t pid;                    /* 进程ID */
    char name[32];                   /* 进程名 */
    task_state_t state;              /* 进程状态 */
    uint32_t priority;               /* 优先级 */
    uint32_t time_slice;             /* 时间片 */
    uint32_t remaining_time;         /* 剩余时间片 */
    
    uint32_t *kernel_stack;          /* 内核栈底指针 */
    uint32_t kernel_stack_size;      /* 内核栈大小 */
    
    page_directory_t *page_dir;      /* 进程页目录（地址空间） */
    user_space_t user_space;         /* 用户空间信息 */
    
    task_context_t context;          /* 寄存器上下文 */
    struct task_struct *next;        /* 链表指针 */
} task_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化进程调度器 */
void task_init();

/* 创建新进程 */
int task_create(const char *name, void (*entry)(), uint32_t priority);

/* 创建用户进程（带独立地址空间） */
int task_create_user(const char *name, void (*entry)(), uint32_t priority);

/* 进程调度（时间片轮转） */
void schedule();

/* 主动放弃CPU */
void task_yield();

/* 当前进程睡眠 */
void task_sleep(uint32_t ticks);

/* 唤醒进程 */
void task_wakeup(task_t *task);

/* 获取当前进程 */
task_t *get_current_task();

/* 获取进程列表 */
task_t *get_task_list();

/* 获取进程数量 */
uint32_t get_task_count();

/* 退出当前进程 */
void task_exit();

/* 切换进程地址空间 */
void switch_task_address_space(task_t *task);

/* 根据 PID 查找进程 */
task_t *task_find_by_pid(uint32_t pid);

/* 设置进程优先级 */
int task_set_priority(uint32_t pid, uint32_t priority);

/* 获取进程优先级 */
int task_get_priority(uint32_t pid, uint32_t *priority);

/* 时钟中断处理（用于进程调度） */
void task_tick();

/* 进程状态转字符串 */
const char *task_state_str(task_state_t state);

#endif /* TASK_H */
