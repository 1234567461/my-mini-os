/* ==========================================
 * 系统调用实现 - syscall.c
 * 功能：
 *   1. 系统调用中断处理（int 0x80）
 *   2. 系统调用分发
 *   3. 基础系统调用实现
 * ========================================== */

#include "syscall.h"
#include "isr.h"
#include "idt.h"
#include "task.h"
#include "vga.h"
#include "vfs.h"
#include "string.h"
#include "types.h"

/* 系统调用函数指针类型 */
typedef int (*syscall_func_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

/* 系统调用表 */
static syscall_func_t syscall_table[MAX_SYSCALLS];

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
    
    /* 其他文件描述符暂时不支持 */
    return -1;
}

/* ==========================================
 * 函数：sys_read
 * 功能：读取文件/设备（暂时未实现）
 * ========================================== */
static int sys_read(uint32_t fd, uint32_t buf, uint32_t count, uint32_t arg4, uint32_t arg5)
{
    (void)fd;
    (void)buf;
    (void)count;
    (void)arg4;
    (void)arg5;
    
    /* 暂时未实现 */
    return -1;
}

/* ==========================================
 * 函数：sys_open
 * 功能：打开文件（暂时未实现）
 * ========================================== */
static int sys_open(uint32_t pathname, uint32_t flags, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
    (void)pathname;
    (void)flags;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    
    /* 暂时未实现 */
    return -1;
}

/* ==========================================
 * 函数：sys_close
 * 功能：关闭文件（暂时未实现）
 * ========================================== */
static int sys_close(uint32_t fd, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
    (void)fd;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    
    /* 暂时未实现 */
    return -1;
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
 * 功能：调整用户堆大小（暂时未实现）
 * ========================================== */
static int sys_brk(uint32_t addr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
    (void)addr;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    
    /* 暂时未实现 */
    return -1;
}

/* ==========================================
 * 函数：syscall_handler
 * 功能：系统调用中断处理函数
 * 说明：
 *   从寄存器中读取系统调用号和参数
 *   调用对应的系统调用处理函数
 *   返回值放在 eax 中
 * ========================================== */
void syscall_handler(isr_regs_t *regs)
{
    uint32_t syscall_num = regs->eax;
    
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
    syscall_table[SYS_WRITE] = (syscall_func_t)sys_write;
    syscall_table[SYS_READ] = (syscall_func_t)sys_read;
    syscall_table[SYS_OPEN] = (syscall_func_t)sys_open;
    syscall_table[SYS_CLOSE] = (syscall_func_t)sys_close;
    syscall_table[SYS_GETPID] = (syscall_func_t)sys_getpid;
    syscall_table[SYS_BRK] = (syscall_func_t)sys_brk;
    
    /* 注册中断处理程序（ISR 0x80） */
    isr_register_handler(0x80, syscall_handler);
}
