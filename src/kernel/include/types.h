/* ==========================================
 * 基础类型定义
 * ========================================== */

#ifndef TYPES_H
#define TYPES_H

/* 无符号整数类型 */
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned long long uint64_t;

/* 有符号整数类型 */
typedef signed char     int8_t;
typedef signed short    int16_t;
typedef signed int      int32_t;
typedef signed long long int64_t;

/* 大小类型 */
typedef uint32_t        size_t;

/* 布尔类型 */
typedef uint8_t         bool;
#define true            1
#define false           0

/* NULL指针 */
#define NULL            ((void*)0)

/* 常用宏 */
#define MIN(a, b)       ((a) < (b) ? (a) : (b))
#define MAX(a, b)       ((a) > (b) ? (a) : (b))
#define ABS(x)          ((x) < 0 ? -(x) : (x))

/* 对齐宏 */
#define ALIGN_UP(x, align)   (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

/* 端口读写函数（汇编实现） */
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void outw(uint16_t port, uint16_t value);
uint16_t inw(uint16_t port);
void outl(uint16_t port, uint32_t value);
uint32_t inl(uint16_t port);

/* 中断开关 */
void cli();
void sti();
void hlt();

#endif /* TYPES_H */
