/* ==========================================
 * 分页机制（虚拟内存）v0.4.0
 * 功能增强：
 *   1. 支持多页目录（每个进程独立地址空间）
 *   2. 内核/用户空间隔离
 *   3. 页错误处理
 *   4. 内存保护
 * ========================================== */
#ifndef PAGING_H
#define PAGING_H

#include "types.h"
#include "isr.h"
#include "memory.h"

/* 页大小（4KB 小页） */
#define PAGE_SIZE       4096

/* 大页大小（4MB） */
#define LARGE_PAGE_SIZE (4 * 1024 * 1024)

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

/* 内存空间划分 */
#define KERNEL_SPACE_END    0x00400000  /* 内核空间结束地址（4MB） */
#define USER_SPACE_START    0x00400000  /* 用户空间起始地址 */
#define USER_SPACE_END      0x08000000  /* 用户空间结束地址（128MB） */

/* 内核默认标志：存在、可写、内核态 */
#define KERNEL_PAGE_FLAGS   (PTE_PRESENT | PTE_WRITABLE)

/* 用户默认标志：存在、可写、用户态 */
#define USER_PAGE_FLAGS     (PTE_PRESENT | PTE_WRITABLE | PTE_USER)

/* ==========================================
 * 页目录结构
 * ========================================== */
typedef struct {
    uint32_t entries[PAGE_DIR_ENTRIES];  /* 页目录条目 */
} page_directory_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化分页机制 */
void paging_init();

/* 获取内核页目录 */
page_directory_t *get_kernel_page_dir();

/* 获取当前页目录 */
page_directory_t *get_current_page_dir();

/* 创建新的页目录（复制内核空间映射） */
page_directory_t *create_page_directory();

/* 销毁页目录 */
void destroy_page_directory(page_directory_t *pd);

/* 切换页目录 */
void switch_page_directory(page_directory_t *pd);

/* 在指定页目录中映射一个虚拟页到物理页 */
int map_page_to_dir(page_directory_t *pd, uint32_t virtual_addr, 
                    uint32_t physical_addr, uint32_t flags);

/* 在当前页目录中映射一个虚拟页到物理页 */
int map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);

/* 映射一个4MB大页 */
int map_large_page_to_dir(page_directory_t *pd, uint32_t virtual_addr,
                          uint32_t physical_addr, uint32_t flags);

/* 在当前页目录中映射一个4MB大页 */
int map_large_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);

/* 取消映射一个虚拟页 */
void unmap_page(page_directory_t *pd, uint32_t virtual_addr);

/* 获取虚拟地址对应的物理地址 */
uint32_t get_physical_address(uint32_t virtual_addr);

/* 启用分页（设置CR0的PG位） */
void enable_paging();

/* 加载页目录到CR3 */
void load_page_directory(uint32_t *page_dir);

/* 刷新TLB中指定虚拟地址的缓存 */
void flush_tlb_entry(uint32_t virtual_addr);

/* 页错误处理函数 */
void page_fault_handler(isr_regs_t *regs);

/* 检查地址是否在内核空间 */
static inline bool is_kernel_address(uint32_t addr) {
    return addr < KERNEL_SPACE_END;
}

/* 检查地址是否在用户空间 */
static inline bool is_user_address(uint32_t addr) {
    return addr >= USER_SPACE_START && addr < USER_SPACE_END;
}

#endif /* PAGING_H */
