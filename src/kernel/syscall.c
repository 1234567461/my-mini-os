/* ==========================================
 * 系统调用实现 - syscall.c
 * 功能：
 *   1. 系统调用初始化（注册中断0x80）
 *   2. 系统调用分发处理
 *   3. 具体系统调用实现
 * ========================================== */
#include "syscall.h"
#include "isr.h"
#include "process.h"
#include "heap.h"
#include "vga.h"
#include "keyboard.h"
#include "fs.h"
#include "string.h"
#include "types.h"

/* ==========================================
 * 内部函数声明
 * ========================================== */
static void syscall_isr_handler(isr_regs_t *regs);

/* ==========================================
 * 函数：sys_write
 * 功能：写输出（简化版，直接写VGA）
 * ========================================== */
static int sys_write_impl(int fd, const char *buf, int len)
{
    if (buf == NULL || len <= 0) {
        return 0;
    }

    /* fd=1 是标准输出，直接写屏幕 */
    if (fd == 1 || fd == 2) {
        for (int i = 0; i < len; i++) {
            vga_putc(buf[i]);
        }
        return len;
    }

    /* 其他fd写文件 */
    return fs_write(fd, buf, len);
}

/* ==========================================
 * 函数：sys_read
 * 功能：读输入（简化版，从键盘读）
 * ========================================== */
static int sys_read_impl(int fd, char *buf, int len)
{
    if (buf == NULL || len <= 0) {
        return 0;
    }

    /* fd=0 是标准输入，从键盘读 */
    if (fd == 0) {
        int i = 0;
        while (i < len - 1) {
            char c = keyboard_getc();
            if (c == '\n' || c == '\r') {
                break;
            }
            if (c == '\b' && i > 0) {
                i--;
                vga_putc('\b');
                continue;
            }
            buf[i++] = c;
            vga_putc(c);
        }
        buf[i] = '\0';
        return i;
    }

    /* 其他fd读文件 */
    return fs_read(fd, buf, len);
}

/* ==========================================
 * 函数：sys_exit
 * 功能：进程退出
 * ========================================== */
static void sys_exit_impl(int status)
{
    (void)status;  /* 暂时忽略退出状态 */
    process_exit(status);
}

/* ==========================================
 * 函数：sys_yield
 * 功能：让出CPU
 * ========================================== */
static void sys_yield_impl(void)
{
    process_yield();
}

/* ==========================================
 * 函数：sys_getpid
 * 功能：获取当前进程ID
 * ========================================== */
static int sys_getpid_impl(void)
{
    process_t *curr = process_get_current();
    if (curr == NULL) {
        return -1;
    }
    return curr->pid;
}

/* ==========================================
 * 函数：sys_sbrk
 * 功能：调整堆大小（简化版，用kmalloc代替）
 * ========================================== */
static void *sys_sbrk_impl(int increment)
{
    if (increment <= 0) {
        return NULL;
    }
    return kmalloc(increment);
}

/* ==========================================
 * 函数：syscall_isr_handler
 * 功能：系统调用中断处理函数
 * 参数：
 *   regs - 寄存器状态
 * 说明：
 *   eax = 系统调用号
 *   ebx = 参数1
 *   ecx = 参数2
 *   edx = 参数3
 *   返回值放在eax中
 * ========================================== */
static void syscall_isr_handler(isr_regs_t *regs)
{
    uint32_t syscall_num = regs->eax;
    uint32_t ret = 0;

    switch (syscall_num) {
        case SYS_WRITE:
            ret = (uint32_t)sys_write_impl(
                (int)regs->ebx,
                (const char *)regs->ecx,
                (int)regs->edx
            );
            break;

        case SYS_READ:
            ret = (uint32_t)sys_read_impl(
                (int)regs->ebx,
                (char *)regs->ecx,
                (int)regs->edx
            );
            break;

        case SYS_EXIT:
            sys_exit_impl((int)regs->ebx);
            break;

        case SYS_YIELD:
            sys_yield_impl();
            break;

        case SYS_GETPID:
            ret = (uint32_t)sys_getpid_impl();
            break;

        case SYS_SBRK:
            ret = (uint32_t)sys_sbrk_impl((int)regs->ebx);
            break;

        default:
            vga_printf("Unknown syscall: %d\n", syscall_num);
            break;
    }

    /* 返回值放在eax中 */
    regs->eax = ret;
}

/* ==========================================
 * 函数：syscall_init
 * 功能：初始化系统调用
 * ========================================== */
void syscall_init(void)
{
    vga_puts("Initializing system call interface...\n");

    /* 注册中断0x80的处理函数 */
    isr_register_handler(0x80, syscall_isr_handler);

    vga_puts("  [✓] Syscall interface (int 0x80)\n");
}
