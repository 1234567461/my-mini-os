/* ==========================================
 * 用户态系统调用头文件 - syscall.h
 * 功能：
 *   1. 系统调用号定义
 *   2. 系统调用宏
 * ========================================== */
#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#include <stdint.h>

/* 系统调用号（与内核保持一致） */
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

/* ==========================================
 * 系统调用宏
 * 通过 int 0x80 触发系统调用
 * ========================================== */

/* 无参数系统调用 */
#define syscall0(num) ({ \
    int ret; \
    __asm__ volatile ( \
        "int $0x80" \
        : "=a"(ret) \
        : "a"(num) \
        : "memory" \
    ); \
    ret; \
})

/* 1个参数系统调用 */
#define syscall1(num, arg1) ({ \
    int ret; \
    __asm__ volatile ( \
        "int $0x80" \
        : "=a"(ret) \
        : "a"(num), "b"(arg1) \
        : "memory" \
    ); \
    ret; \
})

/* 2个参数系统调用 */
#define syscall2(num, arg1, arg2) ({ \
    int ret; \
    __asm__ volatile ( \
        "int $0x80" \
        : "=a"(ret) \
        : "a"(num), "b"(arg1), "c"(arg2) \
        : "memory" \
    ); \
    ret; \
})

/* 3个参数系统调用 */
#define syscall3(num, arg1, arg2, arg3) ({ \
    int ret; \
    __asm__ volatile ( \
        "int $0x80" \
        : "=a"(ret) \
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3) \
        : "memory" \
    ); \
    ret; \
})

#endif /* USER_SYSCALL_H */
