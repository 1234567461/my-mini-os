/* ==========================================
 * 用户态 unistd.h 头文件
 * 功能：
 *   1. 系统调用封装函数
 *   2. 基础 Unix 风格函数
 * ========================================== */
#ifndef UNISTD_H
#define UNISTD_H

#include <stdint.h>
#include <stddef.h>
#include "syscall.h"

/* 标准文件描述符 */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* ==========================================
 * 系统调用封装函数
 * ========================================== */

/* 退出进程 */
static inline void exit(int status) {
    syscall1(SYS_EXIT, status);
    while (1);  /* 不会到达这里 */
}

/* 创建子进程 */
static inline pid_t fork(void) {
    return (pid_t)syscall0(SYS_FORK);
}

/* 读取文件 */
static inline ssize_t read(int fd, void *buf, size_t count) {
    return (ssize_t)syscall3(SYS_READ, fd, (uint32_t)buf, count);
}

/* 写入文件 */
static inline ssize_t write(int fd, const void *buf, size_t count) {
    return (ssize_t)syscall3(SYS_WRITE, fd, (uint32_t)buf, count);
}

/* 打开文件 */
static inline int open(const char *pathname, int flags) {
    return syscall2(SYS_OPEN, (uint32_t)pathname, flags);
}

/* 关闭文件 */
static inline int close(int fd) {
    return syscall1(SYS_CLOSE, fd);
}

/* 获取进程ID */
static inline pid_t getpid(void) {
    return (pid_t)syscall0(SYS_GETPID);
}

/* 调整堆大小 */
static inline void *sbrk(intptr_t increment) {
    /* 简化版：先获取当前 brk，再设置新的 brk */
    uint32_t old_brk = (uint32_t)syscall1(SYS_BRK, 0);
    if (increment == 0) {
        return (void *)old_brk;
    }
    uint32_t new_brk = old_brk + increment;
    uint32_t ret = (uint32_t)syscall1(SYS_BRK, new_brk);
    if (ret == (uint32_t)-1) {
        return (void *)-1;
    }
    return (void *)old_brk;
}

#endif /* UNISTD_H */
