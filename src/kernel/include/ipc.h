/* ==========================================
 * 进程间通信（IPC）头文件 - ipc.h
 * 功能：
 *   1. 管道（Pipe）通信
 *   2. 信号（Signal）机制
 *   3. 进程间数据传输
 * ========================================== */
#ifndef IPC_H
#define IPC_H

#include "types.h"

/* 管道大小 */
#define PIPE_BUFFER_SIZE    4096

/* 信号编号 */
#define SIGHUP      1   /* 挂起 */
#define SIGINT      2   /* 中断（Ctrl+C） */
#define SIGQUIT     3   /* 退出 */
#define SIGILL      4   /* 非法指令 */
#define SIGTRAP     5   /* 跟踪陷阱 */
#define SIGABRT     6   /* 异常终止 */
#define SIGBUS      7   /* 总线错误 */
#define SIGFPE      8   /* 浮点异常 */
#define SIGKILL     9   /* 强制终止（不可捕获） */
#define SIGUSR1     10  /* 用户自定义信号1 */
#define SIGSEGV     11  /* 段错误 */
#define SIGUSR2     12  /* 用户自定义信号2 */
#define SIGPIPE     13  /* 管道破裂 */
#define SIGALRM     14  /* 闹钟 */
#define SIGTERM     15  /* 终止 */
#define SIGCHLD     17  /* 子进程状态变化 */
#define SIGCONT     18  /* 继续执行 */
#define SIGSTOP     19  /* 停止（不可捕获） */
#define SIGTSTP     20  /* 终端停止 */
#define SIGTTIN     21  /* 后台进程读终端 */
#define SIGTTOU     22  /* 后台进程写终端 */

#define SIG_MAX     32  /* 最大信号数 */

/* 信号处理函数类型 */
typedef void (*sighandler_t)(int sig);

/* 信号默认处理 */
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)

/* ==========================================
 * 管道结构体
 * ========================================== */
typedef struct pipe {
    uint8_t buffer[PIPE_BUFFER_SIZE];  /* 管道缓冲区 */
    uint32_t read_pos;                 /* 读位置 */
    uint32_t write_pos;                /* 写位置 */
    uint32_t data_size;                /* 数据大小 */
    
    int read_pid;      /* 读端进程ID */
    int write_pid;     /* 写端进程ID */
    
    bool read_open;    /* 读端是否打开 */
    bool write_open;   /* 写端是否打开 */
    
    struct pipe *next; /* 链表指针 */
} pipe_t;

/* ==========================================
 * 信号信息结构体
 * ========================================== */
typedef struct {
    uint32_t pending_signals;    /* 待处理信号位图 */
    uint32_t blocked_signals;    /* 被阻塞的信号位图 */
    sighandler_t handlers[SIG_MAX];  /* 信号处理函数 */
} signal_info_t;

/* ==========================================
 * 函数声明：管道
 * ========================================== */

/* 初始化 IPC 子系统 */
void ipc_init(void);

/* 创建管道 */
int pipe_create(int pipefd[2]);

/* 从管道读取数据 */
int pipe_read(int fd, void *buf, uint32_t count);

/* 向管道写入数据 */
int pipe_write(int fd, const void *buf, uint32_t count);

/* 关闭管道端 */
int pipe_close(int fd);

/* 获取管道中的数据大小 */
uint32_t pipe_data_size(int fd);

/* ==========================================
 * 函数声明：信号
 * ========================================== */

/* 注册信号处理函数 */
sighandler_t signal(int sig, sighandler_t handler);

/* 发送信号给进程 */
int kill(pid_t pid, int sig);

/* 发送信号给当前进程 */
int raise(int sig);

/* 阻塞信号 */
int sigprocmask(int how, const uint32_t *set, uint32_t *oldset);

/* 等待信号 */
int pause(void);

/* 检查并处理待处理的信号 */
void check_pending_signals(void);

/* 初始化进程的信号信息 */
void signal_init_process(signal_info_t *sig_info);

#endif /* IPC_H */
