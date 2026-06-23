/* ==========================================
 * 堆内存分配器
 * 功能：
 *   1. malloc - 分配内存
 *   2. free - 释放内存
 *   3. 内存块管理（首次适应算法）
 * ========================================== */
#ifndef HEAP_H
#define HEAP_H

#include "types.h"

/* 堆起始地址（放在内核之后，1MB位置开始） */
#define HEAP_START      0x00100000
/* 堆大小（16MB） */
#define HEAP_SIZE       (16 * 1024 * 1024)
/* 堆结束地址 */
#define HEAP_END        (HEAP_START + HEAP_SIZE)

/* 内存块头部 */
typedef struct block_header {
    size_t size;           /* 块大小（包含头部） */
    struct block_header *next;  /* 下一个空闲块 */
    bool used;             /* 是否已使用 */
} block_header_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化堆内存分配器 */
void heap_init();

/* 分配内存 */
void *kmalloc(size_t size);

/* 释放内存 */
void kfree(void *ptr);

/* 获取堆已使用大小 */
size_t heap_get_used();

/* 获取堆总大小 */
size_t heap_get_total();

/* 获取堆空闲大小 */
size_t heap_get_free();

#endif /* HEAP_H */
