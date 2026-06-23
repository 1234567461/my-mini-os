/* ==========================================
 * 物理内存管理（位图法）v0.4.0
 * 功能增强：
 *   1. 内存扩展到 128MB
 *   2. 支持大页管理
 * ========================================== */
#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

/* 页大小（4KB） */
#define PAGE_SIZE       4096

/* 大页大小（4MB） */
#define LARGE_PAGE_SIZE (4 * 1024 * 1024)

/* 内存大小（128MB，v0.4.0 扩展） */
#define MEMORY_SIZE     (128 * 1024 * 1024)

/* 总页数（4KB页） */
#define PAGE_COUNT      (MEMORY_SIZE / PAGE_SIZE)

/* 大页数量 */
#define LARGE_PAGE_COUNT (MEMORY_SIZE / LARGE_PAGE_SIZE)

/* 位图大小（字节） */
#define BITMAP_SIZE     (PAGE_COUNT / 8)

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化物理内存管理器 */
void pmm_init();

/* 分配一个物理页（4KB），返回物理地址 */
void *pmm_alloc_page();

/* 释放一个物理页（4KB） */
void pmm_free_page(void *addr);

/* 分配n个连续的物理页（4KB） */
void *pmm_alloc_pages(size_t n);

/* 释放n个连续的物理页（4KB） */
void pmm_free_pages(void *addr, size_t n);

/* 分配一个大页（4MB），返回物理地址 */
void *pmm_alloc_large_page();

/* 释放一个大页（4MB） */
void pmm_free_large_page(void *addr);

/* 获取已使用的页数 */
size_t pmm_get_used_pages();

/* 获取总页数 */
size_t pmm_get_total_pages();

/* 获取已使用的内存大小（字节） */
size_t pmm_get_used_memory();

/* 获取总内存大小（字节） */
size_t pmm_get_total_memory();

#endif /* MEMORY_H */
