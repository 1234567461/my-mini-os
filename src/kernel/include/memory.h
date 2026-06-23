/* ==========================================
 * 物理内存管理（位图法）
 * ========================================== */

#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

/* 页大小（4KB） */
#define PAGE_SIZE       4096

/* 内存大小（默认32MB，后面可以通过BIOS检测） */
#define MEMORY_SIZE     (32 * 1024 * 1024)

/* 总页数 */
#define PAGE_COUNT      (MEMORY_SIZE / PAGE_SIZE)

/* 位图大小（字节） */
#define BITMAP_SIZE     (PAGE_COUNT / 8)

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化物理内存管理器 */
void pmm_init();

/* 分配一个物理页，返回物理地址 */
void *pmm_alloc_page();

/* 释放一个物理页 */
void pmm_free_page(void *addr);

/* 分配n个连续的物理页 */
void *pmm_alloc_pages(size_t n);

/* 释放n个连续的物理页 */
void pmm_free_pages(void *addr, size_t n);

/* 获取已使用的页数 */
size_t pmm_get_used_pages();

/* 获取总页数 */
size_t pmm_get_total_pages();

#endif /* MEMORY_H */
