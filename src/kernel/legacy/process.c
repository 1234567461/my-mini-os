/* ==========================================
 * 进程管理实现
 * 算法：时间片轮转调度（Round Robin）
 * ========================================== */

#include "process.h"
#include "heap.h"
#include "memory.h"
#include "string.h"
#include "types.h"

/* ==========================================
 * 常量定义
 * ========================================== */
#define KERNEL_STACK_SIZE   0x4000  /* 内核栈大小：16KB */
#define DEFAULT_TIME_SLICE  10      /* 默认时间片：10个滴答 */
#define MAX_PROCESSES       64      /* 最大进程数 */

/* ==========================================
 * 全局变量
 * ========================================== */
static process_t *current_process = NULL;   /* 当前运行的进程 */
static process_t *ready_queue = NULL;       /* 就绪队列（链表头） */
static uint32_t next_pid = 1;               /* 下一个PID */

/* 空闲进程（当没有其他进程时运行） */
static process_t *idle_process = NULL;

/* ==========================================
 * 外部函数（汇编实现）
 * ========================================== */
extern void context_switch(process_t *prev, process_t *next);

/* ==========================================
 * 辅助函数：从就绪队列移除一个进程
 * ========================================== */
static void remove_from_ready_queue(process_t *proc) {
    if (ready_queue == NULL) {
        return;
    }

    /* 如果是链表头 */
    if (ready_queue == proc) {
        ready_queue = proc->next;
        proc->next = NULL;
        return;
    }

    /* 遍历找前一个节点 */
    process_t *prev = ready_queue;
    while (prev->next != NULL && prev->next != proc) {
        prev = prev->next;
    }

    if (prev->next == proc) {
        prev->next = proc->next;
        proc->next = NULL;
    }
}

/* ==========================================
 * 辅助函数：添加到就绪队列末尾
 * ========================================== */
static void add_to_ready_queue(process_t *proc) {
    proc->state = PROCESS_READY;
    proc->next = NULL;

    if (ready_queue == NULL) {
        ready_queue = proc;
        return;
    }

    /* 找到链表末尾 */
    process_t *tail = ready_queue;
    while (tail->next != NULL) {
        tail = tail->next;
    }

    tail->next = proc;
}

/* ==========================================
 * 空闲进程函数
 * 当没有其他进程时，就运行这个
 * ========================================== */
static void idle_thread() {
    while (1) {
        /* 什么都不做，等待中断 */
        asm volatile ("hlt");
    }
}

/* ==========================================
 * 函数：process_init
 * 功能：初始化进程管理
 * ========================================== */
void process_init() {
    next_pid = 1;
    ready_queue = NULL;
    current_process = NULL;

    /* 创建空闲进程 */
    idle_process = process_create("idle", idle_thread, 0);
    if (idle_process != NULL) {
        idle_process->priority = 0;  /* 最低优先级 */
        idle_process->time_slice = 1;
    }
}

/* ==========================================
 * 函数：process_create
 * 功能：创建一个新进程
 * ========================================== */
process_t *process_create(const char *name, void (*entry)(), uint32_t priority) {
    /* 分配PCB */
    process_t *proc = (process_t *)kmalloc(sizeof(process_t));
    if (proc == NULL) {
        return NULL;
    }

    /* 清零 */
    memset(proc, 0, sizeof(process_t));

    /* 设置基本信息 */
    proc->pid = next_pid++;
    strncpy(proc->name, name, 31);
    proc->name[31] = '\0';
    proc->state = PROCESS_READY;
    proc->priority = priority;
    proc->time_slice = DEFAULT_TIME_SLICE;

    /* 分配内核栈 */
    void *stack = kmalloc(KERNEL_STACK_SIZE);
    if (stack == NULL) {
        kfree(proc);
        return NULL;
    }

    /* 栈顶地址（栈从高地址往低地址增长） */
    proc->kernel_stack = (uint32_t)stack + KERNEL_STACK_SIZE;

    /* ==========================================
     * 设置初始栈帧
     * 我们需要构造一个和context_switch保存格式一样的栈
     * 这样第一次调度的时候就能正确启动进程
     *
     * context_switch的栈格式（从高地址到低地址）：
     *   返回地址  <- 进程入口函数
     *   旧ebp
     *   ebx
     *   esi
     *   edi  <- context指针指向这里
     * ========================================== */
    uint32_t *stack_ptr = (uint32_t *)proc->kernel_stack;

    /* 栈从高地址往低地址增长 */

    /* 1. 返回地址：进程入口函数 */
    *(--stack_ptr) = (uint32_t)entry;

    /* 2. 旧ebp（初始为0） */
    *(--stack_ptr) = 0;

    /* 3. ebx, esi, edi（初始为0） */
    *(--stack_ptr) = 0;  /* ebx */
    *(--stack_ptr) = 0;  /* esi */
    *(--stack_ptr) = 0;  /* edi */

    /* 保存栈顶指针（context指向这里） */
    proc->context = (registers_t *)stack_ptr;

    /* 页目录：先和内核共用同一个（简化） */
    proc->page_directory = NULL;

    /* 添加到就绪队列 */
    add_to_ready_queue(proc);

    return proc;
}

/* ==========================================
 * 函数：process_destroy
 * 功能：销毁进程
 * ========================================== */
void process_destroy(process_t *proc) {
    if (proc == NULL) {
        return;
    }

    /* 从就绪队列移除 */
    remove_from_ready_queue(proc);

    /* 释放栈 */
    if (proc->kernel_stack != 0) {
        void *stack_bottom = (void *)(proc->kernel_stack - KERNEL_STACK_SIZE);
        kfree(stack_bottom);
    }

    /* 释放PCB */
    kfree(proc);
}

/* ==========================================
 * 函数：schedule
 * 功能：调度器，选择下一个要运行的进程
 * ========================================== */
void schedule() {
    process_t *prev = current_process;

    /* 如果当前进程还在运行，把它放回就绪队列 */
    if (prev != NULL && prev->state == PROCESS_RUNNING) {
        prev->state = PROCESS_READY;
        add_to_ready_queue(prev);
    }

    /* 选择下一个进程 */
    process_t *next = ready_queue;

    /* 如果就绪队列为空，就运行空闲进程 */
    if (next == NULL) {
        next = idle_process;
    }

    /* 从就绪队列移除 */
    if (next != idle_process) {
        remove_from_ready_queue(next);
    }

    /* 设置为运行状态 */
    next->state = PROCESS_RUNNING;
    next->time_slice = DEFAULT_TIME_SLICE;  /* 重置时间片 */
    current_process = next;

    /* 如果是同一个进程，不用切换 */
    if (prev == next) {
        return;
    }

    /* 上下文切换 */
    if (prev != NULL) {
        context_switch(prev, next);
    } else {
        /* 第一次调度：prev为NULL，不保存当前上下文 */
        context_switch(NULL, next);
    }
}

/* ==========================================
 * 函数：process_yield
 * 功能：主动让出CPU
 * ========================================== */
void process_yield() {
    if (current_process == NULL) {
        return;
    }

    /* 重置时间片，然后调度 */
    current_process->time_slice = 0;
    schedule();
}

/* ==========================================
 * 函数：process_current
 * 功能：获取当前运行的进程
 * ========================================== */
process_t *process_get_current() {
    return current_process;
}

/* ==========================================
 * 函数：process_ready_count
 * 功能：获取就绪进程数
 * ========================================== */
uint32_t process_get_ready_count() {
    uint32_t count = 0;
    process_t *curr = ready_queue;

    while (curr != NULL) {
        count++;
        curr = curr->next;
    }

    return count;
}

/* ==========================================
 * 函数：process_exit
 * 功能：进程退出
 * ========================================== */
void process_exit(int code) {
    (void)code;  /* 暂时忽略退出码 */

    if (current_process == NULL) {
        return;
    }

    /* 设置为终止状态 */
    current_process->state = PROCESS_TERMINATED;

    /* 调度到下一个进程 */
    schedule();

    /* 不应该到这里 */
    while (1) {
        asm volatile ("hlt");
    }
}

/* ==========================================
 * 时钟中断处理：驱动调度器
 * ========================================== */
void process_tick() {
    if (current_process == NULL) {
        return;
    }

    /* 减少当前进程的时间片 */
    if (current_process->time_slice > 0) {
        current_process->time_slice--;
    }

    /* 时间片用完了，调度 */
    if (current_process->time_slice == 0) {
        schedule();
    }
}
