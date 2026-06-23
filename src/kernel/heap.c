/* ==========================================
 * 堆内存分配器实现
 * 算法：首次适应算法（First Fit）
 * ========================================== */
#include "heap.h"
#include "string.h"
#include "types.h"

/* 空闲块链表头 */
static block_header_t *free_list = NULL;

/* 已使用的内存大小 */
static size_t used_memory = 0;

/* 总内存大小 */
static size_t total_memory = 0;

/* ==========================================
 * 对齐到4字节
 * ========================================== */
static inline size_t align4(size_t size) {
    return (size + 3) & ~3;
}

/* ==========================================
 * 初始化堆内存分配器
 * ========================================== */
void heap_init() {
    /* 初始化一个大的空闲块 */
    block_header_t *block = (block_header_t *)HEAP_START;
    block->size = HEAP_SIZE;
    block->next = NULL;
    block->used = false;

    free_list = block;
    total_memory = HEAP_SIZE;
    used_memory = 0;
}

/* ==========================================
 * 分配内存
 * 返回：指向分配内存的指针，失败返回NULL
 * ========================================== */
void *kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    /* 对齐到4字节 */
    size = align4(size);

    /* 需要的总大小（包含头部） */
    size_t total_size = size + sizeof(block_header_t);

    /* 在空闲链表中查找第一个足够大的块 */
    block_header_t *prev = NULL;
    block_header_t *curr = free_list;

    while (curr != NULL) {
        if (!curr->used && curr->size >= total_size) {
            /* 找到合适的块 */
            if (curr->size - total_size > sizeof(block_header_t) + 4) {
                /* 块足够大，可以分裂 */
                block_header_t *new_block = (block_header_t *)((uint32_t)curr + total_size);
                new_block->size = curr->size - total_size;
                new_block->used = false;
                new_block->next = curr->next;

                curr->size = total_size;
                curr->next = new_block;
            }

            /* 标记为已使用 */
            curr->used = true;

            /* 从空闲链表中移除 */
            if (prev == NULL) {
                free_list = curr->next;
            } else {
                prev->next = curr->next;
            }

            used_memory += curr->size;

            /* 返回数据部分的指针（跳过头部） */
            return (void *)((uint32_t)curr + sizeof(block_header_t));
        }

        prev = curr;
        curr = curr->next;
    }

    /* 没有找到足够大的块 */
    return NULL;
}

/* ==========================================
 * 合并相邻的空闲块
 * ========================================== */
static void merge_blocks() {
    block_header_t *curr = free_list;

    while (curr != NULL && curr->next != NULL) {
        /* 检查下一个块是否紧跟在当前块之后 */
        if ((uint32_t)curr + curr->size == (uint32_t)curr->next) {
            /* 合并两个块 */
            curr->size += curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

/* ==========================================
 * 释放内存
 * ========================================== */
void kfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    /* 获取块头部 */
    block_header_t *block = (block_header_t *)((uint32_t)ptr - sizeof(block_header_t));

    if (!block->used) {
        return;  /* 已经是空闲的，防止重复释放 */
    }

    /* 标记为空闲 */
    block->used = false;
    used_memory -= block->size;

    /* 插入到空闲链表（按地址排序） */
    if (free_list == NULL || block < free_list) {
        /* 插入到链表开头 */
        block->next = free_list;
        free_list = block;
    } else {
        /* 找到合适的位置插入 */
        block_header_t *curr = free_list;
        while (curr->next != NULL && curr->next < block) {
            curr = curr->next;
        }
        block->next = curr->next;
        curr->next = block;
    }

    /* 合并相邻的空闲块 */
    merge_blocks();
}

/* ==========================================
 * 获取堆已使用大小
 * ========================================== */
size_t heap_get_used() {
    return used_memory;
}

/* ==========================================
 * 获取堆总大小
 * ========================================== */
size_t heap_get_total() {
    return total_memory;
}

/* ==========================================
 * 获取堆空闲大小
 * ========================================== */
size_t heap_get_free() {
    return total_memory - used_memory;
}
