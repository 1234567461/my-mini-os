/* ==========================================
 * 系统调用实现 - syscall.c v0.7.0
 * 功能：
 *   1. 系统调用中断处理（int 0x80）
 *   2. 系统调用分发
 *   3. 基础系统调用实现
 *   4. 支持用户态切换
 * ========================================== */

#include "syscall.h"
#include "isr.h"
#include "idt.h"
#include "task.h"
#include "vga.h"
#include "vfs.h"
#include "keyboard.h"
#include "string.h"
#include "types.h"
#include "elf.h"
#include "heap.h"
#include "gdt.h"
#include "paging.h"

/* 系统调用函数指针类型 */
typedef int (*syscall_func_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

/* 系统调用表 */
static syscall_func_t syscall_table[MAX_SYSCALLS];

/* 外部变量：从汇编代码引用 */
extern uint32_t tss_esp0;
uint32_t current_task_kernel_stack = 0;

/* ==========================================
 * 函数：sys_exit
 * 功能：退出当前进程
 * ========================================== */
static int sys_exit(uint32_t status, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;

    task_exit(status);
    return 0;  /* 不会到达这里 */
}

/* ==========================================
 * 函数：sys_write
 * 功能：写入文件/设备
 * 参数：
 *   fd - 文件描述符（0=stdin, 1=stdout, 2=stderr, 其他=文件）
 *   buf - 缓冲区
 *   count - 写入字节数
 * 返回：实际写入的字节数
 * ========================================== */
static int sys_write(uint32_t fd, uint32_t buf, uint32_t count, uint32_t arg4, uint32_t arg5)
{
    (void)arg4;
    (void)arg5;
    
    const char *buffer = (const char *)buf;
    
    /* 标准输出和标准错误输出到 VGA */
    if (fd == 1 || fd == 2) {
        for (size_t i = 0; i < count; i++) {
            vga_putc(buffer[i]);
        }
        return count;
    }
    
    /* 普通文件写入 */
    task_t *current = get_current_task();
    if (current != NULL && fd >= 3 && fd < MAX_FDS) {
        vfs_file_t *file = current->files[fd];
        if (file != NULL) {
            return vfs_write(file, buffer, count);
        }
    }
    
    return -1;
}

/* ==========================================
 * 函数：sys_read
 * 功能：读取文件/设备
 * 参数：
 *   fd - 文件描述符（0=stdin, 其他=文件）
 *   buf - 缓冲区
 *   count - 读取字节数
 * 返回：实际读取的字节数
 * ========================================== */
static int sys_read(uint32_t fd, uint32_t buf, uint32_t count, uint32_t arg4, uint32_t arg5)
{
    (void)arg4;
    (void)arg5;
    
    char *buffer = (char *)buf;
    
    /* 标准输入从键盘读取 */
    if (fd == 0) {
        if (count == 0) {
            return 0;
        }
        
        /* 简化版：读取一个字符 */
        char c = keyboard_getc();
        buffer[0] = c;
        
        /* 回显 */
        vga_putc(c);
        
        return 1;
    }
    
    /* 普通文件读取 */
    task_t *current = get_current_task();
    if (current != NULL && fd >= 3 && fd < MAX_FDS) {
        vfs_file_t *file = current->files[fd];
        if (file != NULL) {
            return vfs_read(file, buffer, count);
        }
    }
    
    return -1;
}

/* ==========================================
 * 函数：sys_open
 * 功能：打开文件
 * 参数：
 *   pathname - 文件路径
 *   flags - 打开标志
 * 返回：文件描述符，失败返回-1
 * ========================================== */
static int sys_open(uint32_t pathname, uint32_t flags, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
    (void)arg3;
    (void)arg4;
    (void)arg5;
    
    const char *path = (const char *)pathname;
    if (path == NULL) {
        return -1;
    }
    
    task_t *current = get_current_task();
    if (current == NULL) {
        return -1;
    }
    
    /* 打开文件 */
    vfs_file_t *file = vfs_open(path, flags);
    if (file == NULL) {
        return -1;
    }
    
    /* 找到空闲的文件描述符（从3开始，0-2是标准输入输出） */
    int fd = -1;
    for (int i = 3; i < MAX_FDS; i++) {
        if (current->files[i] == NULL) {
            fd = i;
            break;
        }
    }
    
    if (fd == -1) {
        /* 没有空闲的文件描述符 */
        vfs_close(file);
        return -1;
    }
    
    /* 存入文件描述符表 */
    current->files[fd] = file;
    
    return fd;
}

/* ==========================================
 * 函数：sys_close
 * 功能：关闭文件
 * 参数：
 *   fd - 文件描述符
 * 返回：0=成功，-1=失败
 * ========================================== */
static int sys_close(uint32_t fd, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    
    task_t *current = get_current_task();
    if (current == NULL) {
        return -1;
    }
    
    /* 检查文件描述符是否有效 */
    if (fd < 3 || fd >= MAX_FDS) {
        return -1;
    }
    
    vfs_file_t *file = current->files[fd];
    if (file == NULL) {
        return -1;
    }
    
    /* 关闭文件 */
    int ret = vfs_close(file);
    
    /* 清空文件描述符表 */
    current->files[fd] = NULL;
    
    return ret;
}

/* ==========================================
 * 函数：sys_fork
 * 功能：创建子进程（简化版）
 * 返回：父进程返回子进程PID，子进程返回0
 * ========================================== */
static int sys_fork(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    
    task_t *parent = get_current_task();
    if (parent == NULL) {
        return -1;
    }
    
    /* 简化版：创建一个新的内核进程，共享地址空间
     * 真正的fork需要复制页表和用户空间，这里简化实现 */
    int pid = task_create("child", (void (*)())parent->context.eip, parent->priority);
    if (pid < 0) {
        return -1;
    }
    
    /* 注意：这是简化版fork，子进程会从fork返回处继续执行
     * 但由于我们的简化实现，子进程会从父进程的入口点开始
     * 真正的fork需要复制栈和上下文，这里只返回子进程PID */
    
    return pid;
}

/* ==========================================
 * 函数：sys_getpid
 * 功能：获取当前进程ID
 * ========================================== */
static int sys_getpid(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    
    task_t *current = get_current_task();
    if (current != NULL) {
        return current->pid;
    }
    return -1;
}

/* ==========================================
 * 函数：sys_brk
 * 功能：调整用户堆大小
 * 参数：
 *   addr - 新的堆结束地址
 * 返回：新的堆结束地址，失败返回-1
 * ========================================== */
static int sys_brk(uint32_t addr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    
    task_t *current = get_current_task();
    if (current == NULL) {
        return -1;
    }
    
    user_space_t *us = &current->user_space;
    
    /* 如果 addr 为 0，返回当前 brk */
    if (addr == 0) {
        return us->heap_brk;
    }
    
    /* 检查地址是否在合理范围内 */
    if (addr < us->heap_start) {
        return -1;
    }
    
    /* 简化版：不实际分配物理内存，只更新 brk 指针
     * 实际的内存分配会在页错误时处理 */
    us->heap_brk = addr;
    
    return addr;
}

/* ==========================================
 * 函数：sys_execve
 * 功能：执行程序（简化版）
 * 参数：
 *   pathname - 可执行文件路径
 *   argv - 参数列表
 *   envp - 环境变量列表
 * 返回：成功不返回，失败返回-1
 * ========================================== */
static int sys_execve_impl(uint32_t pathname, uint32_t argv, uint32_t envp, uint32_t arg4, uint32_t arg5)
{
    (void)argv;
    (void)envp;
    (void)arg4;
    (void)arg5;
    
    const char *path = (const char *)pathname;
    if (path == NULL) {
        return -1;
    }
    
    task_t *current = get_current_task();
    if (current == NULL) {
        return -1;
    }
    
    /* 打开 ELF 文件 */
    vfs_file_t *file = vfs_open(path, VFS_O_RDONLY);
    if (file == NULL) {
        return -1;
    }
    
    /* 读取 ELF 文件到内存（简化版：假设文件不大） */
    uint32_t file_size = file->size;
    if (file_size == 0 || file_size > 1024 * 1024) {  /* 最大 1MB */
        vfs_close(file);
        return -1;
    }
    
    void *elf_data = kmalloc(file_size);
    if (elf_data == NULL) {
        vfs_close(file);
        return -1;
    }
    
    int read_bytes = vfs_read(file, elf_data, file_size);
    vfs_close(file);
    
    if (read_bytes != (int)file_size) {
        kfree(elf_data);
        return -1;
    }
    
    /* 加载 ELF 可执行文件 */
    uint32_t entry_point;
    if (elf_load_executable(elf_data, &entry_point) != 0) {
        kfree(elf_data);
        return -1;
    }
    
    /* 简化版：直接跳转到入口点
     * 真正的 execve 需要设置用户栈、参数等
     * 这里我们直接修改当前进程的 eip */
    current->context.eip = entry_point;
    
    /* 释放 ELF 数据（实际应该在加载后释放） */
    kfree(elf_data);
    
    /* 注意：这是简化版，真正的 execve 不会返回
     * 但由于我们的实现方式，这里会返回 0 */
    return 0;
}

/* ==========================================
 * 函数：sys_waitpid_impl
 * 功能：等待子进程退出（简化版）
 * 参数：
 *   pid - 子进程PID
 *   status - 状态指针
 *   options - 选项
 * 返回：子进程PID，失败返回-1
 * ========================================== */
static int sys_waitpid_impl(uint32_t pid, uint32_t status, uint32_t options, uint32_t arg4, uint32_t arg5)
{
    (void)options;
    (void)arg4;
    (void)arg5;
    
    task_t *current = get_current_task();
    if (current == NULL) {
        return -1;
    }
    
    /* 简化版：查找僵尸进程并回收 */
    task_t *task = get_task_list();
    while (task != NULL) {
        if (task->state == TASK_ZOMBIE) {
            if (pid == -1 || task->pid == pid) {
                /* 回收进程资源（简化版） */
                int exit_status = 0;
                if (status != 0) {
                    *(int *)status = exit_status;
                }
                
                /* 简化版：这里只是返回PID，实际应该释放进程资源 */
                return task->pid;
            }
        }
        task = task->next;
    }
    
    /* 没有找到僵尸进程 */
    return -1;
}

/* ==========================================
 * 函数：syscall_dispatch
 * 功能：系统调用分发函数（从汇编调用）
 * 说明：
 *   接收isr_regs_t指针，从寄存器中读取系统调用号和参数
 *   调用对应的系统调用处理函数
 *   返回值放在regs->eax中
 * ========================================== */
void syscall_dispatch(isr_regs_t *regs)
{
    uint32_t syscall_num = regs->eax;

    /* 更新当前任务的内核栈指针 */
    task_t *current = get_current_task();
    if (current != NULL) {
        current_task_kernel_stack = (uint32_t)current->kernel_stack + current->kernel_stack_size;
    }

    /* 检查系统调用号是否有效 */
    if (syscall_num >= MAX_SYSCALLS || syscall_table[syscall_num] == NULL) {
        regs->eax = -1;  /* 无效的系统调用号 */
        return;
    }

    /* 调用对应的系统调用处理函数 */
    syscall_func_t func = syscall_table[syscall_num];
    regs->eax = func(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
}

/* ==========================================
 * 函数：syscall_init
 * 功能：初始化系统调用
 * ========================================== */
void syscall_init(void)
{
    /* 清空系统调用表 */
    memset(syscall_table, 0, sizeof(syscall_table));

    /* 注册系统调用处理函数 */
    syscall_table[SYS_EXIT] = (syscall_func_t)sys_exit;
    syscall_table[SYS_FORK] = (syscall_func_t)sys_fork;
    syscall_table[SYS_READ] = (syscall_func_t)sys_read;
    syscall_table[SYS_WRITE] = (syscall_func_t)sys_write;
    syscall_table[SYS_OPEN] = (syscall_func_t)sys_open;
    syscall_table[SYS_CLOSE] = (syscall_func_t)sys_close;
    syscall_table[SYS_GETPID] = (syscall_func_t)sys_getpid;
    syscall_table[SYS_BRK] = (syscall_func_t)sys_brk;
    syscall_table[SYS_EXECVE] = (syscall_func_t)sys_execve_impl;
    syscall_table[SYS_WAITPID] = (syscall_func_t)sys_waitpid_impl;

    /* 注意：int 0x80的门已经在isr_init中设置好了 */
}

/* ==========================================
 * 包装函数：供外部调用
 * ========================================== */

/* int sys_execve(const char *filename, const char *args) */
int sys_execve(const char *filename, const char *args)
{
    /* 通过系统调用执行 */
    int syscall_num = SYS_EXECVE;
    int result;

    /* 调用系统调用 */
    asm volatile (
        "movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "movl %3, %%ecx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        : "r"(syscall_num), "r"(filename), "r"(args)
        : "eax", "ebx", "ecx", "memory"
    );

    return result;
}

/* int sys_waitpid(int pid, int *status, int options) */
int sys_waitpid(int pid, int *status, int options)
{
    /* 直接通过系统调用表调用内部实现 */
    return sys_waitpid_impl((uint32_t)pid, (uint32_t)status, (uint32_t)options, 0, 0);
}
