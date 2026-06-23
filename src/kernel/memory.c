/* ==========================================
 * 物理内存管理实现（位图法）v0.4.0
 * 功能增强：
 *   1. 128MB 物理内存
 *   2. 4MB 大页分配支持
 * ========================================== */
#include "memory.h"
#include "string.h"
#include "types.h"

/* 内存位图（每一位代表一个4KB页，0=空闲，1=已用） */
static uint8_t memory_bitmap[BITMAP_SIZE];

/* 已使用的页数 */
static size_t used_pages = 0;

/* 内核结束地址（由链接脚本提供） */
extern uint32_t kernel_end;

/* ==========================================
 * 设置某一位（标记为已用）
 * ========================================== */
static inline void bitmap_set(size_t page) {
    memory_bitmap[page / 8] |= (1 << (page % 8));
}

/* ==========================================
 * 清除某一位（标记为空闲）
 * ========================================== */
static inline void bitmap_clear(size_t page) {
    memory_bitmap[page / 8] &= ~(1 << (page % 8));
}

/* ==========================================
 * 测试某一位
 * ========================================== */
static inline bool bitmap_test(size_t page) {
    return (memory_bitmap[page / 8] & (1 << (page % 8))) != 0;
}

/* ==========================================
 * 查找第一个空闲页
 * 返回：页号，找不到返回-1
 * ========================================== */
static ssize_t find_first_free_page() {
    for (size_t i = 0; i < BITMAP_SIZE; i++) {
        if (memory_bitmap[i] != 0xFF) {  /* 这个字节里有空闲页 */
            for (int j = 0; j < 8; j++) {
                size_t page = i * 8 + j;
                if (page < PAGE_COUNT && !bitmap_test(page)) {
                    return page;
                }
            }
        }
    }
    return -1;
}

/* ==========================================
 * 查找n个连续的空闲页
 * 返回：起始页号，找不到返回-1
 * ========================================== */
static ssize_t find_free_pages(size_t n) {
    if (n == 0) return -1;
    if (n == 1) return find_first_free_page();

    size_t start = 0;
    size_t count = 0;
    for (size_t i = 0; i < PAGE_COUNT; i++) {
        if (!bitmap_test(i)) {
            count++;
            if (count == n) {
                return start;
            }
        } else {
            start = i + 1;
            count = 0;
        }
    }
    return -1;
}

/* ==========================================
 * 查找对齐到大页边界的连续空闲页
 * 参数：n = 需要的页数（必须是大页对齐的）
 * 返回：起始页号，找不到返回-1
 * ========================================== */
static ssize_t find_aligned_free_pages(size_t n, size_t alignment_pages) {
    if (n == 0) return -1;

    size_t start = 0;
    while (start < PAGE_COUNT) {
        /* 对齐到指定边界 */
        if (start % alignment_pages != 0) {
            start += alignment_pages - (start % alignment_pages);
            continue;
        }

        /* 检查是否有足够的连续空闲页 */
        bool enough = true;
        for (size_t i = 0; i < n; i++) {
            if (start + i >= PAGE_COUNT || bitmap_test(start + i)) {
                enough = false;
                break;
            }
        }

        if (enough) {
            return start;
        }

        start += alignment_pages;
    }
    return -1;
}

/* ==========================================
 * 初始化物理内存管理器
 * ========================================== */
void pmm_init() {
    /* 先把所有页标记为空闲 */
    memset(memory_bitmap, 0, BITMAP_SIZE);
    used_pages = 0;

    /* 计算内核占用的页数 */
    uint32_t kernel_end_addr = (uint32_t)&kernel_end;
    size_t kernel_pages = (kernel_end_addr + PAGE_SIZE - 1) / PAGE_SIZE;

    /* 把内核占用的页标记为已用 */
    for (size_t i = 0; i < kernel_pages; i++) {
        bitmap_set(i);
        used_pages++;
    }

    /* 把第0页也标记为已用（空指针保护） */
    /* 其实上面已经包含了，这里只是强调一下 */
}

/* ==========================================
 * 分配一个物理页（4KB）
 * ========================================== */
void *pmm_alloc_page() {
    ssize_t page = find_first_free_page();
    if (page == -1) {
        return NULL;  /* 内存不足 */
    }
    bitmap_set(page);
    used_pages++;
    return (void *)(page * PAGE_SIZE);
}

/* ==========================================
 * 释放一个物理页（4KB）
 * ========================================== */
void pmm_free_page(void *addr) {
    if (addr == NULL) return;
    size_t page = (uint32_t)addr / PAGE_SIZE;
    if (page >= PAGE_COUNT) return;
    if (bitmap_test(page)) {
        bitmap_clear(page);
        used_pages--;
    }
}

/* ==========================================
 * 分配n个连续的物理页（4KB）
 * ========================================== */
void *pmm_alloc_pages(size_t n) {
    if (n == 0) return NULL;
    ssize_t start = find_free_pages(n);
    if (start == -1) {
        return NULL;
    }
    for (size_t i = 0; i < n; i++) {
        bitmap_set(start + i);
        used_pages++;
    }
    return (void *)(start * PAGE_SIZE);
}

/* ==========================================
 * 释放n个连续的物理页（4KB）
 * ========================================== */
void pmm_free_pages(void *addr, size_t n) {
    if (addr == NULL || n == 0) return;
    size_t start = (uint32_t)addr / PAGE_SIZE;
    if (start + n > PAGE_COUNT) return;
    for (size_t i = 0; i < n; i++) {
        if (bitmap_test(start + i)) {
            bitmap_clear(start + i);
            used_pages--;
        }
    }
}

/* ==========================================
 * 分配一个大页（4MB）
 * 大页必须按 4MB 对齐
 * ========================================== */
void *pmm_alloc_large_page() {
    size_t pages_per_large = LARGE_PAGE_SIZE / PAGE_SIZE;  /* 1024 页 */
    ssize_t start = find_aligned_free_pages(pages_per_large, pages_per_large);
    if (start == -1) {
        return NULL;
    }
    for (size_t i = 0; i < pages_per_large; i++) {
        bitmap_set(start + i);
        used_pages++;
    }
    return (void *)(start * PAGE_SIZE);
}

/* ==========================================
 * 释放一个大页（4MB）
 * ========================================== */
void pmm_free_large_page(void *addr) {
    if (addr == NULL) return;
    
    /* 检查是否按大页对齐 */
    if ((uint32_t)addr % LARGE_PAGE_SIZE != 0) {
        return;
    }
    
    size_t start_page = (uint32_t)addr / PAGE_SIZE;
    size_t pages_per_large = LARGE_PAGE_SIZE / PAGE_SIZE;
    
    if (start_page + pages_per_large > PAGE_COUNT) return;
    
    for (size_t i = 0; i < pages_per_large; i++) {
        if (bitmap_test(start_page + i)) {
            bitmap_clear(start_page + i);
            used_pages--;
        }
    }
}

/* ==========================================
 * 获取已使用的页数
 * ========================================== */
size_t pmm_get_used_pages() {
    return used_pages;
}

/* ==========================================
 * 获取总页数
 * ========================================== */
size_t pmm_get_total_pages() {
    return PAGE_COUNT;
}

/* ==========================================
 * 获取已使用的内存大小（字节）
 * ========================================== */
size_t pmm_get_used_memory() {
    return used_pages * PAGE_SIZE;
}

/* ==========================================
 * 获取总内存大小（字节）
 * ========================================== */
size_t pmm_get_total_memory() {
    return MEMORY_SIZE;
}
