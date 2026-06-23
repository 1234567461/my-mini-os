/* ==========================================
 * 系统调用 - syscall.h
 * 功能：
 *   1. 系统调用号定义
 *   2. 系统调用处理函数声明
 *   3. 用户态系统调用接口
 * ========================================== */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

/* ==========================================
 * 系统调用号
 * ========================================== */
#define SYS_WRITE   1   /* 写输出 */
#define SYS_READ    2   /* 读输入 */
#define SYS_EXIT    3   /* 进程退出 */
#define SYS_YIELD   4   /* 让出CPU */
#define SYS_GETPID  5   /* 获取进程ID */
#define SYS_MALLOC  6   /* 分配内存 */
#define SYS_FREE    7   /* 释放内存 */

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化系统调用 */
void syscall_init(void);

/* 系统调用处理函数（由中断处理调用） */
void syscall_handler(void);

/* ==========================================
 * 用户态系统调用接口（用汇编实现）
 * ========================================== */

/* 系统调用：写 */
int sys_write(const char *buf, int len);

/* 系统调用：读 */
int sys_read(char *buf, int len);

/* 系统调用：退出 */
void sys_exit(int status);

/* 系统调用：让出CPU */
void sys_yield(void);

/* 系统调用：获取PID */
int sys_getpid(void);

/* 系统调用：分配内存 */
void *sys_malloc(uint32_t size);

/* 系统调用：释放内存 */
void sys_free(void *ptr);

#endif /* SYSCALL_H */
