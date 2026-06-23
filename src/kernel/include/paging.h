/* ==========================================
 * 分页机制（虚拟内存）
 * ========================================== */

#ifndef PAGING_H
#define PAGING_H

#include "types.h"

/* 页大小 */
#define PAGE_SIZE       4096

/* 页目录和页表的条目数 */
#define PAGE_TABLE_ENTRIES  1024
#define PAGE_DIR_ENTRIES    1024

/* 页表项（PTE）标志位 */
#define PTE_PRESENT     0x001   /* 存在 */
#define PTE_WRITABLE    0x002   /* 可写 */
#define PTE_USER        0x004   /* 用户态可访问 */
#define PTE_WRITE_THRU  0x008   /* 写通 */
#define PTE_CACHE_DIS   0x010   /* 禁用缓存 */
#define PTE_ACCESSED    0x020   /* 已访问 */
#define PTE_DIRTY       0x040   /* 已修改 */
#define PTE_PAT         0x080   /* PAT */
#define PTE_GLOBAL      0x100   /* 全局页 */

/* 页目录项（PDE）标志位（和PTE大部分相同） */
#define PDE_PRESENT     0x001
#define PDE_WRITABLE    0x002
#define PDE_USER        0x004
#define PDE_WRITE_THRU  0x008
#define PDE_CACHE_DIS   0x010
#define PDE_ACCESSED    0x020
#define PDE_PAGE_SIZE   0x080   /* 4MB页（0=4KB） */

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化分页机制 */
void paging_init();

/* 映射一个虚拟页到物理页 */
int map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);

/* 获取虚拟地址对应的物理地址 */
uint32_t get_physical_address(uint32_t virtual_addr);

/* 启用分页（设置CR0的PG位） */
void enable_paging();

/* 加载页目录 */
void load_page_directory(uint32_t *page_dir);

#endif /* PAGING_H */
