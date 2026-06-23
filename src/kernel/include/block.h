/* ==========================================
 * 块设备层头文件 - block.h
 * 功能：
 *   1. 块设备抽象接口
 *   2. 缓冲区缓存（buffer cache）
 * ========================================== */

#ifndef BLOCK_H
#define BLOCK_H

#include "types.h"

/* 块大小（512字节，标准扇区大小） */
#define BLOCK_SIZE 512

/* 缓冲区缓存大小（块数） */
#define BUFFER_CACHE_SIZE 64

/* 缓冲区标志 */
#define BUF_VALID   0x01  /* 缓冲区有效（数据是最新的） */
#define BUF_DIRTY   0x02  /* 缓冲区已修改（需要写回磁盘） */
#define BUF_BUSY    0x04  /* 缓冲区正在使用 */

/* ==========================================
 * 缓冲区结构体
 * ========================================== */
typedef struct buffer {
    uint32_t block_no;      /* 块号 */
    uint8_t  data[BLOCK_SIZE];  /* 数据 */
    uint32_t flags;         /* 标志位 */
    uint32_t dev_id;        /* 设备ID */
    struct buffer *next;    /* 链表指针 */
    struct buffer *prev;
} buffer_t;

/* ==========================================
 * 块设备操作函数表
 * ========================================== */
typedef struct block_device {
    const char *name;
    uint32_t dev_id;
    uint32_t block_count;   /* 总块数 */
    
    /* 读块 */
    int (*read_block)(uint32_t dev_id, uint32_t block_no, uint8_t *buf);
    
    /* 写块 */
    int (*write_block)(uint32_t dev_id, uint32_t block_no, const uint8_t *buf);
    
    struct block_device *next;
} block_device_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化块设备层 */
void block_init(void);

/* 注册块设备 */
int block_register_device(block_device_t *dev);

/* 获取块设备 */
block_device_t *block_get_device(uint32_t dev_id);

/* ==========================================
 * 缓冲区缓存函数
 * ========================================== */

/* 从缓存获取块（如果不在缓存中则从磁盘读取） */
buffer_t *block_get_buffer(uint32_t dev_id, uint32_t block_no);

/* 释放缓冲区 */
void block_release_buffer(buffer_t *buf);

/* 读取块（返回数据指针） */
uint8_t *block_read(uint32_t dev_id, uint32_t block_no);

/* 写入块（标记为脏） */
int block_write(uint32_t dev_id, uint32_t block_no, const uint8_t *data);

/* 同步所有脏缓冲区到磁盘 */
void block_sync(void);

/* 使缓冲区无效（用于磁盘变更后） */
void block_invalidate(uint32_t dev_id, uint32_t block_no);

#endif /* BLOCK_H */
