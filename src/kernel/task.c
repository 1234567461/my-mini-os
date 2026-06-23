/* ==========================================
 * 进程管理实现 v0.6.0
 * 功能增强：
 *   1. 每个进程独立的页目录（地址空间隔离）
 *   2. 用户栈和用户堆
 *   3. 内核/用户空间分离
 *   4. 基于优先级的调度算法
 * 调度算法：优先级调度 + 同优先级轮转
 * ========================================== */
#include "task.h"
#include "heap.h"
#include "string.h"
#include "pit.h"
#include "types.h"
#include "paging.h"
#include "memory.h"

/* 最大进程数 */
#define MAX_TASKS       64

/* 默认内核栈大小（4KB） */
#define DEFAULT_KERNEL_STACK_SIZE  4096

/* 默认用户栈大小（4KB） */
#define DEFAULT_USER_STACK_SIZE    4096

/* 默认用户堆大小（4KB） */
#define DEFAULT_USER_HEAP_SIZE     4096

/* 默认时间片（10个tick，即100ms @ 100Hz） */
#define DEFAULT_TIME_SLICE  10

/* 用户空间起始地址（用于分配用户栈和堆） */
#define USER_SPACE_BASE     USER_SPACE_START

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
 * 切换进程地址空间
 * ========================================== */
void switch_task_address_space(task_t *task) {
    if (task == NULL || task->page_dir == NULL) {
        return;
    }
    switch_page_directory(task->page_dir);
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

    /* 分配内核栈 */
    idle_task->kernel_stack_size = DEFAULT_KERNEL_STACK_SIZE;
    idle_task->kernel_stack = (uint32_t *)kmalloc(DEFAULT_KERNEL_STACK_SIZE);
    if (idle_task->kernel_stack == NULL) {
        kfree(idle_task);
        return;
    }

    /* idle进程使用内核页目录 */
    idle_task->page_dir = get_kernel_page_dir();

    /* 设置栈顶（栈从高地址向低地址增长） */
    uint32_t *stack_top = (uint32_t *)((uint32_t)idle_task->kernel_stack + DEFAULT_KERNEL_STACK_SIZE);

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
 * 创建新进程（内核进程，共享内核地址空间）
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

    /* 分配内核栈 */
    task->kernel_stack_size = DEFAULT_KERNEL_STACK_SIZE;
    task->kernel_stack = (uint32_t *)kmalloc(DEFAULT_KERNEL_STACK_SIZE);
    if (task->kernel_stack == NULL) {
        kfree(task);
        return -1;
    }

    /* 内核进程共享内核页目录 */
    task->page_dir = get_kernel_page_dir();

    /* 设置栈顶（栈从高地址向低地址增长） */
    uint32_t *stack_top = (uint32_t *)((uint32_t)task->kernel_stack + DEFAULT_KERNEL_STACK_SIZE);

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
 * 创建用户进程（带独立地址空间）
 * 返回：0成功，-1失败
 * ========================================== */
int task_create_user(const char *name, void (*entry)(), uint32_t priority) {
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
    task->time_slice = DEFAULT_TIME_SLICE + priority * 5;
    task->remaining_time = task->time_slice;

    /* 分配内核栈 */
    task->kernel_stack_size = DEFAULT_KERNEL_STACK_SIZE;
    task->kernel_stack = (uint32_t *)kmalloc(DEFAULT_KERNEL_STACK_SIZE);
    if (task->kernel_stack == NULL) {
        kfree(task);
        return -1;
    }

    /* 创建独立的页目录（复制内核空间映射） */
    task->page_dir = create_page_directory();
    if (task->page_dir == NULL) {
        kfree(task->kernel_stack);
        kfree(task);
        return -1;
    }

    /* 设置用户空间信息 */
    task->user_space.code_start = USER_SPACE_BASE;
    task->user_space.code_size = 0;  /* 暂时为0，后续加载用户程序时设置 */
    task->user_space.heap_start = USER_SPACE_BASE + 0x100000;  /* 1MB偏移处开始堆 */
    task->user_space.heap_brk = task->user_space.heap_start;
    task->user_space.stack_start = USER_SPACE_END - 0x1000;  /* 用户空间顶部向下4KB */
    task->user_space.stack_size = DEFAULT_USER_STACK_SIZE;

    /* 分配用户栈物理页并映射 */
    void *user_stack_phys = pmm_alloc_page();
    if (user_stack_phys == NULL) {
        destroy_page_directory(task->page_dir);
        kfree(task->kernel_stack);
        kfree(task);
        return -1;
    }

    /* 映射用户栈到用户空间 */
    uint32_t user_stack_virt = task->user_space.stack_start - DEFAULT_USER_STACK_SIZE;
    map_page_to_dir(task->page_dir, user_stack_virt, (uint32_t)user_stack_phys, USER_PAGE_FLAGS);

    /* 分配用户堆初始页并映射 */
    void *user_heap_phys = pmm_alloc_page();
    if (user_heap_phys == NULL) {
        pmm_free_page(user_stack_phys);
        destroy_page_directory(task->page_dir);
        kfree(task->kernel_stack);
        kfree(task);
        return -1;
    }

    /* 映射用户堆到用户空间 */
    map_page_to_dir(task->page_dir, task->user_space.heap_start, (uint32_t)user_heap_phys, USER_PAGE_FLAGS);
    task->user_space.heap_brk = task->user_space.heap_start + PAGE_SIZE;

    /* 设置内核栈顶（栈从高地址向低地址增长） */
    uint32_t *stack_top = (uint32_t *)((uint32_t)task->kernel_stack + DEFAULT_KERNEL_STACK_SIZE);

    /* 模拟函数调用栈 */
    *(--stack_top) = (uint32_t)task_exit_wrapper;  /* 返回地址 */

    /* 初始化上下文
     * 注意：目前用户进程仍在内核态运行，
     * 真正的用户态切换需要TSS和iret，后续版本实现
     */
    task->context.eip = (uint32_t)entry;
    task->context.cs = 0x08;  /* 代码段选择子（内核态） */
    task->context.eflags = 0x202;  /* 开中断 */
    task->context.esp = (uint32_t)stack_top;
    task->context.ss = 0x10;  /* 数据段选择子（内核态） */

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

    /* ==========================================
     * 优先级调度算法：
     * 1. 找到最高优先级的就绪进程
     * 2. 相同优先级的进程按轮转方式调度
     * 3. 优先级数值越大，优先级越高
     * ========================================== */
    
    task_t *next = NULL;
    int highest_priority = -1;
    
    /* 第一遍：找到最高优先级 */
    task_t *p = task_list;
    while (p != NULL) {
        if (p->state == TASK_READY && p->priority > highest_priority) {
            highest_priority = p->priority;
        }
        p = p->next;
    }
    
    /* 如果没有找到就绪进程，运行 idle 进程 */
    if (highest_priority == -1) {
        next = idle_task;
    } else {
        /* 第二遍：在最高优先级的进程中，选择下一个（轮转） */
        task_t *start = current_task ? current_task->next : task_list;
        
        /* 从当前位置开始查找 */
        p = start;
        while (p != NULL) {
            if (p->state == TASK_READY && p->priority == highest_priority) {
                next = p;
                break;
            }
            p = p->next;
        }
        
        /* 如果没找到，从头开始找 */
        if (next == NULL) {
            p = task_list;
            while (p != start) {
                if (p->state == TASK_READY && p->priority == highest_priority) {
                    next = p;
                    break;
                }
                p = p->next;
            }
        }
        
        /* 如果还是没找到（理论上不应该发生），运行 idle 进程 */
        if (next == NULL) {
            next = idle_task;
        }
    }

    /* 切换进程 */
    if (next != current_task) {
        task_t *prev = current_task;
        current_task = next;
        current_task->state = TASK_RUNNING;
        current_task->remaining_time = current_task->time_slice;

        /* 切换地址空间（如果页目录不同） */
        if (prev == NULL || prev->page_dir != current_task->page_dir) {
            switch_task_address_space(current_task);
        }

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

    /* 释放内核栈 */
    if (current_task->kernel_stack != NULL) {
        kfree(current_task->kernel_stack);
        current_task->kernel_stack = NULL;
    }

    /* 销毁独立的页目录（如果不是内核页目录） */
    if (current_task->page_dir != NULL && 
        current_task->page_dir != get_kernel_page_dir()) {
        /* 注意：这里先切换回内核页目录，再销毁 */
        switch_page_directory(get_kernel_page_dir());
        destroy_page_directory(current_task->page_dir);
        current_task->page_dir = NULL;
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
