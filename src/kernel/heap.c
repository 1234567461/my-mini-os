/* ==========================================
 * 堆内存分配器实现 - heap.c
 * 算法：首次适应（First Fit）+ 边界标记合并
 * ========================================== */

#include "heap.h"
#include "paging.h"
#include "string.h"
#include "types.h"

/* 空闲块链表头 */
static block_header_t *free_list = NULL;

/* 堆的起始和结束地址 */
static uint32_t heap_start = HEAP_START;
static uint32_t heap_end   = HEAP_START + HEAP_SIZE;

/* ==========================================
 * 内部辅助函数：对齐到4字节
 * ========================================== */
static uint32_t align4(uint32_t size)
{
    return (size + 3) & ~3;
}

/* ==========================================
 * 函数：heap_init
 * 功能：初始化堆
 * ========================================== */
void heap_init(void)
{
    /* 先映射堆需要的物理页 */
    uint32_t num_pages = HEAP_SIZE / 4096;
    for (uint32_t i = 0; i < num_pages; i++) {
        uint32_t virt = HEAP_START + i * 4096;
        /* 映射到物理页，标志：存在 + 可写 */
        map_page(virt, 0, PTE_PRESENT | PTE_WRITABLE);
    }

    /* 初始化一个大的空闲块，覆盖整个堆 */
    block_header_t *initial_block = (block_header_t *)heap_start;
    initial_block->size = HEAP_SIZE;
    initial_block->used = 0;
    initial_block->next = NULL;

    /* 空闲链表指向这个初始块 */
    free_list = initial_block;
}

/* ==========================================
 * 函数：kmalloc
 * 功能：分配size字节的内存
 * ========================================== */
void *kmalloc(uint32_t size)
{
    if (size == 0) {
        return NULL;
    }

    /* 对齐到4字节 */
    uint32_t real_size = align4(size + sizeof(block_header_t));

    /* 遍历空闲链表，找第一个足够大的块 */
    block_header_t *prev = NULL;
    block_header_t *curr = free_list;

    while (curr != NULL) {
        if (curr->size >= real_size) {
            /* 找到了！检查是否需要分裂 */
            if (curr->size - real_size > sizeof(block_header_t) + 4) {
                /* 分裂成两个块 */
                block_header_t *new_block = (block_header_t *)((uint32_t)curr + real_size);
                new_block->size = curr->size - real_size;
                new_block->used = 0;
                new_block->next = curr->next;

                /* 更新当前块的大小 */
                curr->size = real_size;

                /* 把新块加入空闲链表 */
                if (prev == NULL) {
                    free_list = new_block;
                } else {
                    prev->next = new_block;
                }
            } else {
                /* 不需要分裂，直接从空闲链表移除 */
                if (prev == NULL) {
                    free_list = curr->next;
                } else {
                    prev->next = curr->next;
                }
            }

            /* 标记为已使用 */
            curr->used = 1;
            curr->next = NULL;

            /* 返回数据部分的指针（跳过头部） */
            return (void *)((uint32_t)curr + sizeof(block_header_t));
        }

        prev = curr;
        curr = curr->next;
    }

    /* 没有找到足够大的块，返回NULL */
    return NULL;
}

/* ==========================================
 * 内部辅助函数：合并相邻的空闲块
 * ========================================== */
static void merge_blocks(block_header_t *block)
{
    /* 检查下一个块是否空闲且相邻 */
    block_header_t *next = (block_header_t *)((uint32_t)block + block->size);
    if ((uint32_t)next < heap_end && next->used == 0) {
        /* 合并：把下一个块的大小加进来 */
        block->size += next->size;

        /* 从空闲链表中移除next */
        block_header_t *prev = NULL;
        block_header_t *curr = free_list;
        while (curr != NULL) {
            if (curr == next) {
                if (prev == NULL) {
                    free_list = curr->next;
                } else {
                    prev->next = curr->next;
                }
                break;
            }
            prev = curr;
            curr = curr->next;
        }
    }

    /* 检查前一个块是否空闲且相邻（需要遍历空闲链表找） */
    /* 简单实现：不合并前一个块，只合并后一个块 */
    /* 更完善的实现可以用双向链表或者边界标记 */
}

/* ==========================================
 * 函数：kfree
 * 功能：释放之前分配的内存
 * ========================================== */
void kfree(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    /* 得到块头部指针 */
    block_header_t *block = (block_header_t *)((uint32_t)ptr - sizeof(block_header_t));

    if (block->used == 0) {
        /* 重复释放，忽略 */
        return;
    }

    /* 标记为空闲 */
    block->used = 0;

    /* 加入空闲链表（插到开头） */
    block->next = free_list;
    free_list = block;

    /* 尝试合并相邻的空闲块 */
    merge_blocks(block);
}

/* ==========================================
 * 函数：heap_stats
 * 功能：获取堆的使用情况
 * ========================================== */
void heap_stats(uint32_t *used, uint32_t *free)
{
    uint32_t used_size = 0;
    uint32_t free_size = 0;

    /* 遍历整个堆 */
    uint32_t addr = heap_start;
    while (addr < heap_end) {
        block_header_t *block = (block_header_t *)addr;
        if (block->used) {
            used_size += block->size;
        } else {
            free_size += block->size;
        }
        addr += block->size;
    }

    if (used) *used = used_size;
    if (free) *free = free_size;
}
