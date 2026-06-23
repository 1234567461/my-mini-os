/* ==========================================
 * 进程管理 - process.h
 * 功能：
 *   1. 进程控制块（PCB）定义
 *   2. 进程创建和销毁
 *   3. 调度器（时间片轮转）
 *   4. 上下文切换
 * ========================================== */

#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

/* ==========================================
 * 进程状态
 * ========================================== */
typedef enum {
    PROCESS_READY,      /* 就绪，等待运行 */
    PROCESS_RUNNING,    /* 正在运行 */
    PROCESS_BLOCKED,    /* 阻塞（等待事件） */
    PROCESS_TERMINATED  /* 已终止 */
} process_state_t;

/* ==========================================
 * 寄存器上下文
 * 注意：这个结构和context_switch.asm里的保存顺序要一致
 * ========================================== */
typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t eip;  /* 返回地址 */
} registers_t;

/* ==========================================
 * 进程控制块（PCB）
 * ========================================== */
typedef struct process {
    uint32_t pid;                  /* 进程ID */
    char name[32];                 /* 进程名 */
    process_state_t state;         /* 进程状态 */
    uint32_t priority;             /* 优先级 */
    uint32_t time_slice;           /* 剩余时间片 */
    uint32_t *page_directory;      /* 页目录（物理地址） */
    uint32_t kernel_stack;         /* 内核栈顶地址 */
    uint32_t user_stack;           /* 用户栈顶地址（暂未使用） */
    registers_t *context;          /* 寄存器上下文指针（栈顶） */
    struct process *next;          /* 链表指针 */
} process_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化进程管理系统 */
void process_init();

/* 创建一个新进程 */
process_t *process_create(const char *name, void (*entry)(), uint32_t priority);

/* 销毁一个进程 */
void process_destroy(process_t *proc);

/* 调度器：选择下一个要运行的进程 */
void schedule();

/* 主动让出CPU */
void process_yield();

/* 获取当前运行的进程 */
process_t *process_get_current();

/* 获取就绪进程数 */
uint32_t process_get_ready_count();

/* 进程退出 */
void process_exit(int code);

/* 时钟tick处理（驱动调度器） */
void process_tick();

/* ==========================================
 * 汇编实现的函数
 * ========================================== */

/* 上下文切换（在context_switch.asm中实现） */
extern void context_switch(process_t *prev, process_t *next);

#endif /* PROCESS_H */
