/* ==========================================
 * 分页机制实现
 * ========================================== */

#include "paging.h"
#include "memory.h"
#include "string.h"
#include "types.h"

/* 页目录（物理地址，按4KB对齐） */
static uint32_t *page_directory = NULL;

/* ==========================================
 * 获取页目录索引（虚拟地址的高10位）
 * ========================================== */
static inline uint32_t get_pde_index(uint32_t virtual_addr) {
    return (virtual_addr >> 22) & 0x3FF;
}

/* ==========================================
 * 获取页表索引（虚拟地址的中间10位）
 * ========================================== */
static inline uint32_t get_pte_index(uint32_t virtual_addr) {
    return (virtual_addr >> 12) & 0x3FF;
}

/* ==========================================
 * 加载页目录到CR3寄存器
 * ========================================== */
void load_page_directory(uint32_t *page_dir) {
    asm volatile ("mov %0, %%cr3" : : "r"(page_dir));
}

/* ==========================================
 * 获取内核页目录
 * ========================================== */
uint32_t *get_kernel_page_dir() {
    return page_directory;
}

/* ==========================================
 * 启用分页（设置CR0的PG位）
 * ========================================== */
void enable_paging() {
    uint32_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  /* 设置PG位（第31位） */
    asm volatile ("mov %0, %%cr0" : : "r"(cr0));
}

/* ==========================================
 * 映射一个虚拟页到物理页
 * 返回：0成功，-1失败
 * ========================================== */
int map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    if (page_directory == NULL) {
        return -1;
    }

    uint32_t pde_idx = get_pde_index(virtual_addr);
    uint32_t pte_idx = get_pte_index(virtual_addr);

    /* 检查页目录项是否存在 */
    uint32_t pde = page_directory[pde_idx];
    uint32_t *page_table;

    if (!(pde & PDE_PRESENT)) {
        /* 页表不存在，分配一个新的页表 */
        page_table = (uint32_t *)pmm_alloc_page();
        if (page_table == NULL) {
            return -1;  /* 内存不足 */
        }

        /* 清空页表 */
        memset(page_table, 0, PAGE_SIZE);

        /* 设置页目录项 */
        page_directory[pde_idx] = (uint32_t)page_table | flags | PDE_PRESENT;
    } else {
        /* 页表已存在，获取其地址 */
        page_table = (uint32_t *)(pde & 0xFFFFF000);
    }

    /* 如果物理地址为0，自动分配一个物理页 */
    if (physical_addr == 0) {
        physical_addr = (uint32_t)pmm_alloc_page();
        if (physical_addr == 0) {
            return -1;  /* 内存不足 */
        }
    }

    /* 设置页表项 */
    page_table[pte_idx] = (physical_addr & 0xFFFFF000) | flags | PTE_PRESENT;

    return 0;
}

/* ==========================================
 * 获取虚拟地址对应的物理地址
 * ========================================== */
uint32_t get_physical_address(uint32_t virtual_addr) {
    if (page_directory == NULL) {
        return virtual_addr;  /* 分页未启用，直接返回 */
    }

    uint32_t pde_idx = get_pde_index(virtual_addr);
    uint32_t pte_idx = get_pte_index(virtual_addr);

    uint32_t pde = page_directory[pde_idx];
    if (!(pde & PDE_PRESENT)) {
        return 0;  /* 页目录项不存在 */
    }

    uint32_t *page_table = (uint32_t *)(pde & 0xFFFFF000);
    uint32_t pte = page_table[pte_idx];
    if (!(pte & PTE_PRESENT)) {
        return 0;  /* 页表项不存在 */
    }

    /* 返回物理地址（页基地址 + 页内偏移） */
    return (pte & 0xFFFFF000) | (virtual_addr & 0xFFF);
}

/* ==========================================
 * 初始化分页机制
 * ========================================== */
void paging_init() {
    /* 分配一页作为页目录 */
    page_directory = (uint32_t *)pmm_alloc_page();
    if (page_directory == NULL) {
        return;  /* 内存不足 */
    }

    /* 清空页目录 */
    memset(page_directory, 0, PAGE_SIZE);

    /* 恒等映射前4MB内存（虚拟地址 = 物理地址）
     * 这样内核可以正常运行，因为我们的内核在低地址空间
     */
    for (uint32_t i = 0; i < 1024; i++) {
        uint32_t virtual_addr = i * PAGE_SIZE;
        uint32_t physical_addr = i * PAGE_SIZE;
        map_page(virtual_addr, physical_addr, PTE_WRITABLE | PTE_PRESENT);
    }

    /* 加载页目录 */
    load_page_directory(page_directory);

    /* 启用分页 */
    enable_paging();
}
