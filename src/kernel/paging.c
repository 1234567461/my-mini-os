/* ==========================================
 * 分页机制实现 v0.4.0
 * 功能增强：
 *   1. 多页目录支持
 *   2. 内核/用户空间隔离
 *   3. 页错误处理
 * ========================================== */
#include "paging.h"
#include "memory.h"
#include "string.h"
#include "types.h"
#include "isr.h"
#include "vga.h"

/* 内核页目录（所有进程共享内核空间映射） */
static page_directory_t *kernel_page_dir = NULL;

/* 当前页目录 */
static page_directory_t *current_page_dir = NULL;

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
 * 启用分页（设置CR0的PG位）
 * ========================================== */
void enable_paging() {
    uint32_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  /* 设置PG位（第31位） */
    asm volatile ("mov %0, %%cr0" : : "r"(cr0));
}

/* ==========================================
 * 刷新TLB中指定虚拟地址的缓存
 * ========================================== */
void flush_tlb_entry(uint32_t virtual_addr) {
    asm volatile ("invlpg (%0)" : : "r"(virtual_addr));
}

/* ==========================================
 * 获取内核页目录
 * ========================================== */
page_directory_t *get_kernel_page_dir() {
    return kernel_page_dir;
}

/* ==========================================
 * 获取当前页目录
 * ========================================== */
page_directory_t *get_current_page_dir() {
    return current_page_dir;
}

/* ==========================================
 * 在指定页目录中映射一个虚拟页到物理页
 * 返回：0成功，-1失败
 * ========================================== */
int map_page_to_dir(page_directory_t *pd, uint32_t virtual_addr, 
                    uint32_t physical_addr, uint32_t flags) {
    if (pd == NULL) {
        return -1;
    }

    uint32_t pde_idx = get_pde_index(virtual_addr);
    uint32_t pte_idx = get_pte_index(virtual_addr);

    /* 检查页目录项是否存在 */
    uint32_t pde = pd->entries[pde_idx];
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
        pd->entries[pde_idx] = (uint32_t)page_table | flags | PDE_PRESENT;
    } else {
        /* 页表已存在，获取其地址 */
        page_table = (uint32_t *)(pde & 0xFFFFF000);
    }

    /* 设置页表项 */
    page_table[pte_idx] = (physical_addr & 0xFFFFF000) | flags | PTE_PRESENT;

    /* 如果是当前页目录，刷新TLB */
    if (pd == current_page_dir) {
        flush_tlb_entry(virtual_addr);
    }

    return 0;
}

/* ==========================================
 * 在当前页目录中映射一个虚拟页到物理页
 * ========================================== */
int map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    return map_page_to_dir(current_page_dir, virtual_addr, physical_addr, flags);
}

/* ==========================================
 * 在指定页目录中映射一个4MB大页
 * 返回：0成功，-1失败
 * ========================================== */
int map_large_page_to_dir(page_directory_t *pd, uint32_t virtual_addr,
                          uint32_t physical_addr, uint32_t flags) {
    if (pd == NULL) {
        return -1;
    }

    /* 大页必须按4MB对齐 */
    if ((virtual_addr % LARGE_PAGE_SIZE != 0) || 
        (physical_addr % LARGE_PAGE_SIZE != 0)) {
        return -1;
    }

    uint32_t pde_idx = get_pde_index(virtual_addr);

    /* 设置页目录项为大页模式（PS位=1） */
    pd->entries[pde_idx] = (physical_addr & 0xFFC00000) |  /* 物理地址高10位 */
                           flags | 
                           PDE_PRESENT | 
                           PDE_PAGE_SIZE;  /* 大页标志 */

    /* 如果是当前页目录，刷新TLB */
    if (pd == current_page_dir) {
        flush_tlb_entry(virtual_addr);
    }

    return 0;
}

/* ==========================================
 * 在当前页目录中映射一个4MB大页
 * ========================================== */
int map_large_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    return map_large_page_to_dir(current_page_dir, virtual_addr, physical_addr, flags);
}

/* ==========================================
 * 取消映射一个虚拟页
 * ========================================== */
void unmap_page(page_directory_t *pd, uint32_t virtual_addr) {
    if (pd == NULL) return;

    uint32_t pde_idx = get_pde_index(virtual_addr);
    uint32_t pte_idx = get_pte_index(virtual_addr);

    uint32_t pde = pd->entries[pde_idx];
    if (!(pde & PDE_PRESENT)) {
        return;  /* 页表不存在，无需处理 */
    }

    uint32_t *page_table = (uint32_t *)(pde & 0xFFFFF000);
    page_table[pte_idx] = 0;  /* 清除页表项 */

    /* 刷新TLB */
    if (pd == current_page_dir) {
        flush_tlb_entry(virtual_addr);
    }
}

/* ==========================================
 * 获取虚拟地址对应的物理地址
 * 支持4KB小页和4MB大页
 * ========================================== */
uint32_t get_physical_address(uint32_t virtual_addr) {
    if (current_page_dir == NULL) {
        return virtual_addr;  /* 分页未启用，直接返回 */
    }

    uint32_t pde_idx = get_pde_index(virtual_addr);
    uint32_t pde = current_page_dir->entries[pde_idx];

    if (!(pde & PDE_PRESENT)) {
        return 0;  /* 页目录项不存在 */
    }

    /* 检查是否是大页（4MB） */
    if (pde & PDE_PAGE_SIZE) {
        /* 大页模式：物理地址高10位 + 低22位偏移 */
        return (pde & 0xFFC00000) | (virtual_addr & 0x003FFFFF);
    }

    /* 小页模式（4KB） */
    uint32_t pte_idx = get_pte_index(virtual_addr);
    uint32_t *page_table = (uint32_t *)(pde & 0xFFFFF000);
    uint32_t pte = page_table[pte_idx];

    if (!(pte & PTE_PRESENT)) {
        return 0;  /* 页表项不存在 */
    }

    /* 返回物理地址（页基地址 + 页内偏移） */
    return (pte & 0xFFFFF000) | (virtual_addr & 0xFFF);
}

/* ==========================================
 * 创建新的页目录
 * 复制内核空间映射，用户空间为空
 * ========================================== */
page_directory_t *create_page_directory() {
    if (kernel_page_dir == NULL) {
        return NULL;
    }

    /* 分配新的页目录 */
    page_directory_t *new_pd = (page_directory_t *)pmm_alloc_page();
    if (new_pd == NULL) {
        return NULL;
    }

    /* 清空新页目录 */
    memset(new_pd, 0, PAGE_SIZE);

    /* 计算内核空间占用的页目录项数量 */
    uint32_t kernel_pde_count = KERNEL_SPACE_END / (PAGE_SIZE * PAGE_TABLE_ENTRIES);
    if (kernel_pde_count > PAGE_DIR_ENTRIES) {
        kernel_pde_count = PAGE_DIR_ENTRIES;
    }

    /* 复制内核空间的页目录项 */
    for (uint32_t i = 0; i < kernel_pde_count; i++) {
        new_pd->entries[i] = kernel_page_dir->entries[i];
    }

    return new_pd;
}

/* ==========================================
 * 销毁页目录
 * 注意：只释放用户空间的页表和页，内核空间共享不释放
 * ========================================== */
void destroy_page_directory(page_directory_t *pd) {
    if (pd == NULL || pd == kernel_page_dir) {
        return;
    }

    /* 计算内核空间占用的页目录项数量 */
    uint32_t kernel_pde_count = KERNEL_SPACE_END / (PAGE_SIZE * PAGE_TABLE_ENTRIES);

    /* 释放用户空间的页表 */
    for (uint32_t i = kernel_pde_count; i < PAGE_DIR_ENTRIES; i++) {
        uint32_t pde = pd->entries[i];
        if (pde & PDE_PRESENT) {
            uint32_t *page_table = (uint32_t *)(pde & 0xFFFFF000);
            
            /* 释放页表中分配的物理页 */
            for (uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++) {
                uint32_t pte = page_table[j];
                if (pte & PTE_PRESENT) {
                    void *phys_addr = (void *)(pte & 0xFFFFF000);
                    pmm_free_page(phys_addr);
                }
            }
            
            /* 释放页表本身 */
            pmm_free_page(page_table);
        }
    }

    /* 释放页目录本身 */
    pmm_free_page(pd);
}

/* ==========================================
 * 切换页目录
 * ========================================== */
void switch_page_directory(page_directory_t *pd) {
    if (pd == NULL) {
        return;
    }

    current_page_dir = pd;
    load_page_directory((uint32_t *)pd);
}

/* ==========================================
 * 页错误处理函数（ISR 14）
 * ========================================== */
void page_fault_handler(isr_regs_t *regs) {
    /* 获取导致页错误的虚拟地址（CR2寄存器） */
    uint32_t fault_addr;
    asm volatile ("mov %%cr2, %0" : "=r"(fault_addr));

    /* 解析错误码 */
    bool present = !(regs->err_code & 0x1);  /* 0=缺页，1=权限错误 */
    bool write = regs->err_code & 0x2;       /* 0=读，1=写 */
    bool user = regs->err_code & 0x4;        /* 0=内核态，1=用户态 */
    bool reserved = regs->err_code & 0x8;    /* 保留位被覆盖 */

    /* 显示页错误信息 */
    vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
    vga_printf("\n*** PAGE FAULT ***\n");
    vga_printf("Fault address: 0x%x\n", fault_addr);
    vga_printf("Error code: 0x%x\n", regs->err_code);
    vga_printf("Type: %s, %s, %s mode\n",
               present ? "page not present" : "protection fault",
               write ? "write" : "read",
               user ? "user" : "kernel");
    vga_printf("EIP: 0x%x\n", regs->eip);

    if (reserved) {
        vga_puts("Reserved bits overwritten!\n");
    }

    vga_puts("\nSystem halted!\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* 停机 */
    for (;;) {
        asm volatile ("cli; hlt");
    }
}

/* ==========================================
 * 初始化分页机制
 * ========================================== */
void paging_init() {
    /* 分配一页作为内核页目录 */
    kernel_page_dir = (page_directory_t *)pmm_alloc_page();
    if (kernel_page_dir == NULL) {
        return;  /* 内存不足 */
    }

    /* 清空页目录 */
    memset(kernel_page_dir, 0, PAGE_SIZE);

    /* 恒等映射内核空间（前4MB）
     * 使用4MB大页映射，减少页表开销，提高TLB命中率
     * 虚拟地址 = 物理地址
     * 权限：内核态、可写
     */
    map_large_page_to_dir(kernel_page_dir, 0, 0, KERNEL_PAGE_FLAGS);

    /* 设置当前页目录为内核页目录 */
    current_page_dir = kernel_page_dir;

    /* 注册页错误处理函数 */
    isr_register_handler(14, page_fault_handler);

    /* 加载页目录 */
    load_page_directory((uint32_t *)kernel_page_dir);

    /* 启用分页 */
    enable_paging();
}
