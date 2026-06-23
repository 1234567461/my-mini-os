/* ==========================================
 * 堆内存分配器 - heap.h
 * 功能：
 *   1. 内核堆的初始化
 *   2. malloc/free 动态内存分配
 *   3. 首次适应算法 + 块合并
 * ========================================== */

#ifndef HEAP_H
#define HEAP_H

#include "types.h"

/* 堆的起始地址和大小（虚拟地址） */
#define HEAP_START  0x00200000  /* 2MB处开始（内核之后） */
#define HEAP_SIZE   0x00200000  /* 2MB大小 */

/* 内存块头部 */
typedef struct block_header {
    uint32_t size;              /* 块大小（包括头部） */
    uint8_t  used;              /* 是否被使用：1=已用，0=空闲 */
    struct block_header *next;  /* 下一个块（仅空闲块使用） */
} block_header_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化堆 */
void heap_init(void);

/* 分配内存 */
void *kmalloc(uint32_t size);

/* 释放内存 */
void kfree(void *ptr);

/* 获取堆的使用情况（调试用） */
void heap_stats(uint32_t *used, uint32_t *free);

#endif /* HEAP_H */
