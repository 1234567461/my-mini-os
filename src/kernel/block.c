/* ==========================================
 * 块设备层实现 - block.c
 * 功能：
 *   1. 块设备抽象接口
 *   2. 缓冲区缓存（buffer cache）
 * ========================================== */

#include "block.h"
#include "heap.h"
#include "string.h"
#include "vga.h"

/* 块设备链表 */
static block_device_t *device_list = NULL;

/* 缓冲区缓存数组 */
static buffer_t buffer_cache[BUFFER_CACHE_SIZE];

/* LRU 链表头（最近使用的在前面） */
static buffer_t *lru_head = NULL;
static buffer_t *lru_tail = NULL;

/* ==========================================
 * 函数：block_init
 * 功能：初始化块设备层和缓冲区缓存
 * ========================================== */
void block_init(void)
{
    /* 初始化缓冲区缓存 */
    memset(buffer_cache, 0, sizeof(buffer_cache));
    
    /* 初始化 LRU 链表 */
    lru_head = NULL;
    lru_tail = NULL;
    
    /* 将所有缓冲区加入空闲链表（初始都在尾部） */
    for (int i = 0; i < BUFFER_CACHE_SIZE; i++) {
        buffer_cache[i].flags = 0;
        buffer_cache[i].block_no = 0xFFFFFFFF;
        buffer_cache[i].dev_id = 0xFFFFFFFF;
        
        /* 加入 LRU 链表尾部 */
        if (lru_tail == NULL) {
            lru_head = &buffer_cache[i];
            lru_tail = &buffer_cache[i];
            buffer_cache[i].prev = NULL;
            buffer_cache[i].next = NULL;
        } else {
            lru_tail->next = &buffer_cache[i];
            buffer_cache[i].prev = lru_tail;
            buffer_cache[i].next = NULL;
            lru_tail = &buffer_cache[i];
        }
    }
    
    device_list = NULL;
}

/* ==========================================
 * 函数：block_register_device
 * 功能：注册块设备
 * ========================================== */
int block_register_device(block_device_t *dev)
{
    if (dev == NULL) {
        return -1;
    }
    
    /* 添加到设备链表头部 */
    dev->next = device_list;
    device_list = dev;
    
    return 0;
}

/* ==========================================
 * 函数：block_get_device
 * 功能：根据设备ID获取块设备
 * ========================================== */
block_device_t *block_get_device(uint32_t dev_id)
{
    block_device_t *dev = device_list;
    while (dev != NULL) {
        if (dev->dev_id == dev_id) {
            return dev;
        }
        dev = dev->next;
    }
    return NULL;
}

/* ==========================================
 * 函数：lru_remove
 * 功能：从 LRU 链表中移除缓冲区
 * ========================================== */
static void lru_remove(buffer_t *buf)
{
    if (buf->prev != NULL) {
        buf->prev->next = buf->next;
    } else {
        lru_head = buf->next;
    }
    
    if (buf->next != NULL) {
        buf->next->prev = buf->prev;
    } else {
        lru_tail = buf->prev;
    }
}

/* ==========================================
 * 函数：lru_add_head
 * 功能：将缓冲区添加到 LRU 链表头部
 * ========================================== */
static void lru_add_head(buffer_t *buf)
{
    buf->prev = NULL;
    buf->next = lru_head;
    
    if (lru_head != NULL) {
        lru_head->prev = buf;
    } else {
        lru_tail = buf;
    }
    
    lru_head = buf;
}

/* ==========================================
 * 函数：lru_touch
 * 功能：标记缓冲区为最近使用（移到链表头部）
 * ========================================== */
static void lru_touch(buffer_t *buf)
{
    lru_remove(buf);
    lru_add_head(buf);
}

/* ==========================================
 * 函数：find_buffer
 * 功能：在缓存中查找指定的块
 * ========================================== */
static buffer_t *find_buffer(uint32_t dev_id, uint32_t block_no)
{
    buffer_t *buf = lru_head;
    while (buf != NULL) {
        if ((buf->flags & BUF_VALID) && 
            buf->dev_id == dev_id && 
            buf->block_no == block_no) {
            return buf;
        }
        buf = buf->next;
    }
    return NULL;
}

/* ==========================================
 * 函数：get_free_buffer
 * 功能：获取一个空闲缓冲区（使用 LRU 替换）
 * ========================================== */
static buffer_t *get_free_buffer(void)
{
    /* 从 LRU 尾部获取最久未使用的缓冲区 */
    buffer_t *buf = lru_tail;
    
    if (buf == NULL) {
        return NULL;
    }
    
    /* 如果是脏的，先写回磁盘 */
    if (buf->flags & BUF_DIRTY) {
        block_device_t *dev = block_get_device(buf->dev_id);
        if (dev != NULL && dev->write_block != NULL) {
            dev->write_block(buf->dev_id, buf->block_no, buf->data);
        }
    }
    
    /* 从 LRU 链表移除 */
    lru_remove(buf);
    
    /* 重置缓冲区 */
    buf->flags = 0;
    buf->block_no = 0xFFFFFFFF;
    buf->dev_id = 0xFFFFFFFF;
    
    return buf;
}

/* ==========================================
 * 函数：block_get_buffer
 * 功能：从缓存获取块（如果不在缓存中则从磁盘读取）
 * ========================================== */
buffer_t *block_get_buffer(uint32_t dev_id, uint32_t block_no)
{
    /* 先在缓存中查找 */
    buffer_t *buf = find_buffer(dev_id, block_no);
    
    if (buf != NULL) {
        /* 缓存命中，标记为最近使用 */
        lru_touch(buf);
        buf->flags |= BUF_BUSY;
        return buf;
    }
    
    /* 缓存未命中，获取一个空闲缓冲区 */
    buf = get_free_buffer();
    if (buf == NULL) {
        return NULL;
    }
    
    /* 从磁盘读取块 */
    block_device_t *dev = block_get_device(dev_id);
    if (dev == NULL || dev->read_block == NULL) {
        /* 设备不存在，返回错误 */
        /* 将缓冲区放回 LRU 尾部 */
        lru_add_head(buf);  /* 先加回头部 */
        lru_tail = buf;     /* 然后移到尾部 */
        return NULL;
    }
    
    int result = dev->read_block(dev_id, block_no, buf->data);
    if (result < 0) {
        /* 读取失败 */
        lru_add_head(buf);
        return NULL;
    }
    
    /* 设置缓冲区信息 */
    buf->dev_id = dev_id;
    buf->block_no = block_no;
    buf->flags = BUF_VALID | BUF_BUSY;
    
    /* 加入 LRU 链表头部 */
    lru_add_head(buf);
    
    return buf;
}

/* ==========================================
 * 函数：block_release_buffer
 * 功能：释放缓冲区
 * ========================================== */
void block_release_buffer(buffer_t *buf)
{
    if (buf == NULL) {
        return;
    }
    
    buf->flags &= ~BUF_BUSY;
}

/* ==========================================
 * 函数：block_read
 * 功能：读取块（返回数据指针）
 * ========================================== */
uint8_t *block_read(uint32_t dev_id, uint32_t block_no)
{
    buffer_t *buf = block_get_buffer(dev_id, block_no);
    if (buf == NULL) {
        return NULL;
    }
    
    /* 返回数据指针 */
    return buf->data;
    /* 注意：调用者需要调用 block_release_buffer 释放 */
}

/* ==========================================
 * 函数：block_write
 * 功能：写入块（标记为脏）
 * ========================================== */
int block_write(uint32_t dev_id, uint32_t block_no, const uint8_t *data)
{
    buffer_t *buf = block_get_buffer(dev_id, block_no);
    if (buf == NULL) {
        return -1;
    }
    
    /* 复制数据 */
    memcpy(buf->data, data, BLOCK_SIZE);
    
    /* 标记为脏 */
    buf->flags |= BUF_DIRTY;
    
    block_release_buffer(buf);
    
    return 0;
}

/* ==========================================
 * 函数：block_sync
 * 功能：同步所有脏缓冲区到磁盘
 * ========================================== */
void block_sync(void)
{
    buffer_t *buf = lru_head;
    
    while (buf != NULL) {
        if ((buf->flags & BUF_VALID) && (buf->flags & BUF_DIRTY)) {
            block_device_t *dev = block_get_device(buf->dev_id);
            if (dev != NULL && dev->write_block != NULL) {
                dev->write_block(buf->dev_id, buf->block_no, buf->data);
                buf->flags &= ~BUF_DIRTY;
            }
        }
        buf = buf->next;
    }
}

/* ==========================================
 * 函数：block_invalidate
 * 功能：使缓冲区无效
 * ========================================== */
void block_invalidate(uint32_t dev_id, uint32_t block_no)
{
    buffer_t *buf = find_buffer(dev_id, block_no);
    if (buf != NULL) {
        buf->flags &= ~BUF_VALID;
    }
}
