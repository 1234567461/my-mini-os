/* ==========================================
 *  My Mini OS 用户态工具
 *  使用内联汇编直接调用系统调用 (int 0x80)
 * ========================================== */

#ifndef MMOS_TOOLS_H
#define MMOS_TOOLS_H

#include <stdint.h>

/* 系统调用号 */
#define SYS_EXIT    1
#define SYS_FORK    2
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_BRK     12
#define SYS_GETPID  20

/* 标准文件描述符 */
#define STDIN_FD    0
#define STDOUT_FD   1
#define STDERR_FD   2

/* 工具函数 */
static inline int sys_write(int fd, const char *buf, int count) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(count)
        : "memory"
    );
    return ret;
}

static inline int sys_read(int fd, char *buf, int count) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(count)
        : "memory"
    );
    return ret;
}

static inline void sys_exit(int status) {
    __asm__ volatile (
        "int $0x80"
        :
        : "a"(SYS_EXIT), "b"(status)
    );
}

static inline int strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static inline void puts(const char *s) {
    sys_write(STDOUT_FD, s, strlen(s));
}

static inline void putchar(char c) {
    sys_write(STDOUT_FD, &c, 1);
}

static inline int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

static inline char *strchr(const char *s, char c) {
    while (*s) {
        if (*s == c) return (char*)s;
        s++;
    }
    return 0;
}

/* 入口点 */
#define MMOS_MAIN() \
    void _start() { \
        int argc = 0; \
        char **argv = 0; \
        __asm__ volatile ( \
            "movl %%eax, %0\n\t" \
            "movl %%ebx, %1\n\t" \
            : "=r"(argc), "=r"(argv) \
            : \
            : "eax", "ebx" \
        ); \
        int ret = mmos_main(argc, argv); \
        sys_exit(ret); \
    } \
    int mmos_main(int argc, char **argv)

#endif /* MMOS_TOOLS_H */
