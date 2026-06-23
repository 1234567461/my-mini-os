/* ==========================================
 * 进程间通信（IPC）实现 - ipc.c
 * 功能：
 *   1. 管道（Pipe）通信
 *   2. 信号（Signal）机制
 * ========================================== */
#include "ipc.h"
#include "task.h"
#include "heap.h"
#include "string.h"
#include "vga.h"

/* 管道链表 */
static pipe_t *pipe_list = NULL;
static uint32_t pipe_count = 0;

/* 文件描述符起始值（避免和标准文件描述符冲突） */
#define PIPE_FD_BASE  100

/* ==========================================
 * 函数：ipc_init
 * 功能：初始化 IPC 子系统
 * ========================================== */
void ipc_init(void)
{
    pipe_list = NULL;
    pipe_count = 0;
    vga_puts("  [✓] IPC Subsystem (pipes & signals)\n");
}

/* ==========================================
 * 函数：pipe_create
 * 功能：创建管道
 * 参数：pipefd[0] = 读端，pipefd[1] = 写端
 * 返回：0=成功，-1=失败
 * ========================================== */
int pipe_create(int pipefd[2])
{
    if (pipefd == NULL) {
        return -1;
    }

    /* 分配管道结构 */
    pipe_t *pipe = (pipe_t *)kmalloc(sizeof(pipe_t));
    if (pipe == NULL) {
        return -1;
    }

    /* 初始化管道 */
    memset(pipe, 0, sizeof(pipe_t));
    pipe->read_pos = 0;
    pipe->write_pos = 0;
    pipe->data_size = 0;
    pipe->read_open = true;
    pipe->write_open = true;

    /* 获取当前进程ID */
    task_t *current = get_current_task();
    if (current != NULL) {
        pipe->read_pid = current->pid;
        pipe->write_pid = current->pid;
    } else {
        pipe->read_pid = 0;
        pipe->write_pid = 0;
    }

    /* 分配文件描述符 */
    int fd_base = PIPE_FD_BASE + pipe_count * 2;
    pipefd[0] = fd_base;      /* 读端 */
    pipefd[1] = fd_base + 1;  /* 写端 */

    /* 加入链表 */
    pipe->next = pipe_list;
    pipe_list = pipe;
    pipe_count++;

    return 0;
}

/* ==========================================
 * 辅助函数：根据文件描述符查找管道
 * ========================================== */
static pipe_t *find_pipe_by_fd(int fd, bool *is_read_end)
{
    pipe_t *p = pipe_list;
    uint32_t index = 0;

    while (p != NULL) {
        int fd_base = PIPE_FD_BASE + index * 2;
        if (fd == fd_base) {
            if (is_read_end != NULL) {
                *is_read_end = true;
            }
            return p;
        }
        if (fd == fd_base + 1) {
            if (is_read_end != NULL) {
                *is_read_end = false;
            }
            return p;
        }
        p = p->next;
        index++;
    }

    return NULL;
}

/* ==========================================
 * 函数：pipe_read
 * 功能：从管道读取数据
 * 返回：实际读取的字节数，-1=失败
 * ========================================== */
int pipe_read(int fd, void *buf, uint32_t count)
{
    if (buf == NULL || count == 0) {
        return -1;
    }

    bool is_read_end;
    pipe_t *pipe = find_pipe_by_fd(fd, &is_read_end);
    
    if (pipe == NULL || !is_read_end || !pipe->read_open) {
        return -1;
    }

    /* 如果没有数据且写端已关闭，返回 0（EOF） */
    if (pipe->data_size == 0) {
        if (!pipe->write_open) {
            return 0;
        }
        /* 简化版：非阻塞，直接返回 -1 */
        return -1;
    }

    /* 计算实际可读字节数 */
    uint32_t to_read = (count < pipe->data_size) ? count : pipe->data_size;
    uint8_t *dest = (uint8_t *)buf;
    uint32_t bytes_read = 0;

    /* 从环形缓冲区读取 */
    for (uint32_t i = 0; i < to_read; i++) {
        dest[i] = pipe->buffer[pipe->read_pos];
        pipe->read_pos = (pipe->read_pos + 1) % PIPE_BUFFER_SIZE;
        bytes_read++;
    }

    pipe->data_size -= bytes_read;

    return bytes_read;
}

/* ==========================================
 * 函数：pipe_write
 * 功能：向管道写入数据
 * 返回：实际写入的字节数，-1=失败
 * ========================================== */
int pipe_write(int fd, const void *buf, uint32_t count)
{
    if (buf == NULL || count == 0) {
        return -1;
    }

    bool is_read_end;
    pipe_t *pipe = find_pipe_by_fd(fd, &is_read_end);
    
    if (pipe == NULL || is_read_end || !pipe->write_open) {
        return -1;
    }

    /* 如果读端已关闭，触发 SIGPIPE 信号（简化版：直接返回 -1） */
    if (!pipe->read_open) {
        return -1;
    }

    /* 计算剩余空间 */
    uint32_t free_space = PIPE_BUFFER_SIZE - pipe->data_size;
    uint32_t to_write = (count < free_space) ? count : free_space;
    
    if (to_write == 0) {
        /* 缓冲区满，简化版：返回 -1 */
        return -1;
    }

    const uint8_t *src = (const uint8_t *)buf;
    uint32_t bytes_written = 0;

    /* 写入环形缓冲区 */
    for (uint32_t i = 0; i < to_write; i++) {
        pipe->buffer[pipe->write_pos] = src[i];
        pipe->write_pos = (pipe->write_pos + 1) % PIPE_BUFFER_SIZE;
        bytes_written++;
    }

    pipe->data_size += bytes_written;

    return bytes_written;
}

/* ==========================================
 * 函数：pipe_close
 * 功能：关闭管道端
 * 返回：0=成功，-1=失败
 * ========================================== */
int pipe_close(int fd)
{
    bool is_read_end;
    pipe_t *pipe = find_pipe_by_fd(fd, &is_read_end);
    
    if (pipe == NULL) {
        return -1;
    }

    if (is_read_end) {
        pipe->read_open = false;
    } else {
        pipe->write_open = false;
    }

    /* 如果两端都关闭了，释放管道 */
    if (!pipe->read_open && !pipe->write_open) {
        /* 从链表中移除 */
        if (pipe_list == pipe) {
            pipe_list = pipe->next;
        } else {
            pipe_t *prev = pipe_list;
            while (prev != NULL && prev->next != pipe) {
                prev = prev->next;
            }
            if (prev != NULL) {
                prev->next = pipe->next;
            }
        }
        
        kfree(pipe);
        pipe_count--;
    }

    return 0;
}

/* ==========================================
 * 函数：pipe_data_size
 * 功能：获取管道中的数据大小
 * ========================================== */
uint32_t pipe_data_size(int fd)
{
    bool is_read_end;
    pipe_t *pipe = find_pipe_by_fd(fd, &is_read_end);
    
    if (pipe == NULL) {
        return 0;
    }

    return pipe->data_size;
}

/* ==========================================
 * 信号相关实现
 * ========================================== */

/* 每个进程的信号信息（简化版：存在task扩展中）
 * 由于我们不能直接修改task_t，我们用一个数组来存储 */
#define MAX_SIGNAL_TASKS  64
static signal_info_t *task_signals[MAX_SIGNAL_TASKS] = {NULL};

/* ==========================================
 * 函数：signal_init_process
 * 功能：初始化进程的信号信息
 * ========================================== */
void signal_init_process(signal_info_t *sig_info)
{
    if (sig_info == NULL) {
        return;
    }

    memset(sig_info, 0, sizeof(signal_info_t));
    
    /* 设置默认处理函数 */
    for (int i = 0; i < SIG_MAX; i++) {
        sig_info->handlers[i] = SIG_DFL;
    }
}

/* ==========================================
 * 辅助函数：获取当前进程的信号信息
 * ========================================== */
static signal_info_t *get_current_signal_info(void)
{
    task_t *current = get_current_task();
    if (current == NULL) {
        return NULL;
    }

    /* 简化版：用 PID 作为索引 */
    pid_t pid = current->pid;
    if (pid >= MAX_SIGNAL_TASKS) {
        return NULL;
    }

    /* 如果还没有初始化，创建一个 */
    if (task_signals[pid] == NULL) {
        task_signals[pid] = (signal_info_t *)kmalloc(sizeof(signal_info_t));
        if (task_signals[pid] != NULL) {
            signal_init_process(task_signals[pid]);
        }
    }

    return task_signals[pid];
}

/* ==========================================
 * 函数：signal
 * 功能：注册信号处理函数
 * 返回：旧的处理函数，失败返回 SIG_ERR
 * ========================================== */
sighandler_t signal(int sig, sighandler_t handler)
{
    if (sig <= 0 || sig >= SIG_MAX) {
        return SIG_ERR;
    }

    /* SIGKILL 和 SIGSTOP 不能被捕获或忽略 */
    if (sig == SIGKILL || sig == SIGSTOP) {
        return SIG_ERR;
    }

    signal_info_t *sig_info = get_current_signal_info();
    if (sig_info == NULL) {
        return SIG_ERR;
    }

    sighandler_t old_handler = sig_info->handlers[sig];
    sig_info->handlers[sig] = handler;

    return old_handler;
}

/* ==========================================
 * 函数：kill
 * 功能：发送信号给进程
 * 返回：0=成功，-1=失败
 * ========================================== */
int kill(pid_t pid, int sig)
{
    if (sig <= 0 || sig >= SIG_MAX) {
        return -1;
    }

    /* 查找进程 */
    task_t *task_list = get_task_list();
    task_t *target = NULL;
    
    task_t *p = task_list;
    while (p != NULL) {
        if (p->pid == pid) {
            target = p;
            break;
        }
        p = p->next;
    }

    if (target == NULL) {
        return -1;
    }

    /* 获取目标进程的信号信息 */
    if (pid >= MAX_SIGNAL_TASKS) {
        return -1;
    }

    if (task_signals[pid] == NULL) {
        task_signals[pid] = (signal_info_t *)kmalloc(sizeof(signal_info_t));
        if (task_signals[pid] != NULL) {
            signal_init_process(task_signals[pid]);
        } else {
            return -1;
        }
    }

    signal_info_t *sig_info = task_signals[pid];

    /* 设置待处理信号位 */
    sig_info->pending_signals |= (1 << (sig - 1));

    return 0;
}

/* ==========================================
 * 函数：raise
 * 功能：发送信号给当前进程
 * 返回：0=成功，-1=失败
 * ========================================== */
int raise(int sig)
{
    task_t *current = get_current_task();
    if (current == NULL) {
        return -1;
    }

    return kill(current->pid, sig);
}

/* ==========================================
 * 函数：sigprocmask
 * 功能：阻塞信号
 * 返回：0=成功，-1=失败
 * ========================================== */
int sigprocmask(int how, const uint32_t *set, uint32_t *oldset)
{
    signal_info_t *sig_info = get_current_signal_info();
    if (sig_info == NULL) {
        return -1;
    }

    /* 保存旧的掩码 */
    if (oldset != NULL) {
        *oldset = sig_info->blocked_signals;
    }

    if (set == NULL) {
        return 0;
    }

    /* how 参数：0 = SIG_BLOCK, 1 = SIG_UNBLOCK, 2 = SIG_SETMASK */
    switch (how) {
        case 0:  /* SIG_BLOCK */
            sig_info->blocked_signals |= *set;
            break;
        case 1:  /* SIG_UNBLOCK */
            sig_info->blocked_signals &= ~(*set);
            break;
        case 2:  /* SIG_SETMASK */
            sig_info->blocked_signals = *set;
            break;
        default:
            return -1;
    }

    /* SIGKILL 和 SIGSTOP 不能被阻塞 */
    sig_info->blocked_signals &= ~((1 << (SIGKILL - 1)) | (1 << (SIGSTOP - 1)));

    return 0;
}

/* ==========================================
 * 函数：pause
 * 功能：等待信号
 * 返回：-1（总是返回 -1，errno 设为 EINTR）
 * ========================================== */
int pause(void)
{
    /* 简化版：直接返回，实际应该挂起进程直到信号到来 */
    return -1;
}

/* ==========================================
 * 函数：check_pending_signals
 * 功能：检查并处理待处理的信号
 * 在调度器或系统调用返回时调用
 * ========================================== */
void check_pending_signals(void)
{
    signal_info_t *sig_info = get_current_signal_info();
    if (sig_info == NULL) {
        return;
    }

    /* 检查所有待处理的信号 */
    for (int sig = 1; sig < SIG_MAX; sig++) {
        uint32_t sig_mask = (1 << (sig - 1));
        
        /* 如果信号待处理且未被阻塞 */
        if ((sig_info->pending_signals & sig_mask) && 
            !(sig_info->blocked_signals & sig_mask)) {
            
            /* 清除待处理标志 */
            sig_info->pending_signals &= ~sig_mask;

            /* 获取处理函数 */
            sighandler_t handler = sig_info->handlers[sig];

            if (handler == SIG_IGN) {
                /* 忽略信号 */
                continue;
            } else if (handler == SIG_DFL) {
                /* 默认处理 */
                switch (sig) {
                    case SIGCHLD:
                    case SIGCONT:
                    case SIGWINCH:
                        /* 默认忽略 */
                        break;
                    case SIGSTOP:
                    case SIGTSTP:
                    case SIGTTIN:
                    case SIGTTOU:
                        /* 停止进程（简化版：暂不实现） */
                        break;
                    default:
                        /* 终止进程 */
                        task_exit();
                        break;
                }
            } else {
                /* 调用用户处理函数（简化版：直接调用） */
                handler(sig);
            }
        }
    }
}
