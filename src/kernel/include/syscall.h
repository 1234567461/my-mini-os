/* ==========================================
 * 系统调用头文件 - syscall.h
 * 功能：
 *   1. 系统调用号定义
 *   2. 系统调用函数声明
 * ========================================== */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

/* 系统调用号 */
#define SYS_EXIT    1
#define SYS_FORK    2
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_BRK     12
#define SYS_GETPID  20
#define SYS_WAITPID 26
#define SYS_EXECVE  11
#define SYS_CHDIR   12
#define SYS_GETCWD  13

/* 系统调用最大数量 */
#define MAX_SYSCALLS 64

/* ==========================================
 * 函数：syscall_init
 * 功能：初始化系统调用
 * ========================================== */
void syscall_init(void);

#endif /* SYSCALL_H */
