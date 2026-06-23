/* ==========================================
 * 系统功能 - system.h v0.4.0
 * 功能：
 *   1. 系统重启（reboot）
 *   2. 系统停机（halt）
 * ========================================== */
#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

/* ==========================================
 * 函数声明
 * ========================================== */

/* 系统重启（通过8042键盘控制器） */
void system_reboot();

/* 系统停机 */
void system_halt();

/* 检测CPU是否支持大页（4MB） */
bool cpu_supports_large_pages();

#endif /* SYSTEM_H */
