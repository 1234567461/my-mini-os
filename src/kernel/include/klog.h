/* ==========================================
 * 内核日志 - klog.h v0.4.0
 * 功能：
 *   1. 环形缓冲区存储内核日志
 *   2. printk 函数
 *   3. dmesg 命令支持
 * ========================================== */
#ifndef KLOG_H
#define KLOG_H

#include "types.h"

/* 日志缓冲区大小（4KB） */
#define KLOG_BUF_SIZE   4096

/* 日志级别 */
#define KLOG_EMERG      0   /* 系统不可用 */
#define KLOG_ALERT      1   /* 必须立即采取行动 */
#define KLOG_CRIT       2   /* 严重情况 */
#define KLOG_ERR        3   /* 错误情况 */
#define KLOG_WARNING    4   /* 警告情况 */
#define KLOG_NOTICE     5   /* 正常但重要的情况 */
#define KLOG_INFO       6   /* 信息性消息 */
#define KLOG_DEBUG      7   /* 调试级消息 */

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化内核日志 */
void klog_init();

/* 记录模块日志（简化版，带模块名） */
void klog_log(const char *module, const char *message);

/* 打印内核日志（带级别） */
void printk(int level, const char *fmt, ...);

/* 打印内核日志（默认INFO级别） */
void printk_info(const char *fmt, ...);

/* 读取内核日志（dmesg） */
int klog_read(char *buf, int len);

/* 清空内核日志 */
void klog_clear();

/* 获取日志缓冲区 */
const char *klog_get_buffer();

/* 获取日志长度 */
int klog_get_length();

#endif /* KLOG_H */
