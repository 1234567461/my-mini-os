/*
 * sort.c - Sort lines of text files
 * My Mini OS User Space Tool
 * Version: 1.0.0
 * License: MIT
 */

/* System call numbers */
#define SYS_EXIT    1
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_BRK    12

#define STDIN_FD    0
#define STDOUT_FD   1
#define STDERR_FD   2

static int my_strlen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void my_write(int fd, const char *buf, int len) {
    __asm__ volatile (
        "int \tools/gen_packages.shx80"
        : : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(len)
        : "memory"
    );
}

static void my_exit(int code) {
    __asm__ volatile (
        "int \tools/gen_packages.shx80"
        : : "a"(SYS_EXIT), "b"(code)
    );
}

static void my_puts(const char *s) {
    my_write(STDOUT_FD, s, my_strlen(s));
}

int mmos_main(int argc, char **argv) {
    my_puts("sort v1.0.0\n");
    return 0;
}

void _start() {
    int argc = 0;
    char **argv = 0;
    int ret = mmos_main(argc, argv);
    my_exit(ret);
}
