/* ==========================================
 * MPK - My Mini OS Package Format
 * 完全自研的包文件格式
 * ==========================================
 *
 * 文件结构:
 * +-----------------------+  <-- 0
 * |   MPK File Header     |   64 bytes
 * +-----------------------+  <-- sizeof(mpk_header_t)
 * |   Metadata Section    |   可变长度
 * |   (JSON-like)         |
 * +-----------------------+  <-- header.meta_size
 * |   File Table          |   文件条目表
 * +-----------------------+  <-- header.file_table_offset
 * |   File Data           |   文件内容（压缩）
 * +-----------------------+  <-- 文件末尾
 *
 * ========================================== */

#ifndef MPK_H
#define MPK_H

#include "types.h"

/* MPK 魔数 */
#define MPK_MAGIC       0x4B504D4B  /* "KPMK" - My Mini OS PacKage */
#define MPK_VERSION     1
#define MPK_HEADER_SIZE 64

/* 包标志 */
#define MPK_FLAG_COMPRESSED  0x0001  /* 内容已压缩 */
#define MPK_FLAG_ENCRYPTED   0x0002  /* 内容已加密 */
#define MPK_FLAG_SIGNED      0x0004  /* 已签名 */

/* 压缩算法 */
#define MPK_COMPRESS_NONE    0
#define MPK_COMPRESS_RLE     1
#define MPK_COMPRESS_LZSS    2

/* 最大名称长度 */
#define MPK_NAME_LEN        64
#define MPK_VERSION_LEN     32
#define MPK_DESC_LEN        256
#define MPK_DEP_MAX         16
#define MPK_FILE_MAX        256

/* MPK 文件头 */
typedef struct {
    uint32_t magic;           /* 魔数 MPK_MAGIC */
    uint16_t version;         /* 格式版本 */
    uint16_t flags;           /* 标志位 */
    uint32_t header_size;     /* 头部大小 */
    uint32_t meta_offset;     /* 元数据偏移 */
    uint32_t meta_size;       /* 元数据大小 */
    uint32_t file_table_offset;  /* 文件表偏移 */
    uint32_t file_count;      /* 文件数量 */
    uint32_t data_offset;     /* 数据区偏移 */
    uint32_t data_size;       /* 数据区大小（压缩后） */
    uint32_t data_size_orig;  /* 数据区原始大小 */
    uint8_t  compress_algo;   /* 压缩算法 */
    uint8_t  _reserved[19];   /* 保留 */
    uint32_t checksum;        /* 整个文件的校验和（CRC32） */
} __attribute__((packed)) mpk_header_t;

/* 文件条目 */
typedef struct {
    char name[MPK_NAME_LEN];   /* 文件名（含路径） */
    uint32_t offset;           /* 在数据区的偏移 */
    uint32_t size;             /* 文件大小（压缩后） */
    uint32_t size_orig;        /* 文件原始大小 */
    uint32_t mode;             /* 文件权限 */
    uint32_t flags;            /* 文件标志 */
    uint32_t checksum;         /* 文件内容校验和 */
} __attribute__((packed)) mpk_file_entry_t;

/* 包元数据（内存中） */
typedef struct {
    char name[MPK_NAME_LEN];
    char version[MPK_VERSION_LEN];
    char description[MPK_DESC_LEN];
    char author[MPK_NAME_LEN];
    uint32_t build_time;
    char dependencies[MPK_DEP_MAX][MPK_NAME_LEN];
    int dep_count;
} mpk_metadata_t;

/* ==========================================
 * MPK 操作函数
 * ========================================== */

/* 验证MPK文件 */
int mpk_validate(const uint8_t *data, uint32_t size);

/* 解析MPK头部 */
int mpk_read_header(const uint8_t *data, uint32_t size, mpk_header_t *header);

/* 解析元数据 */
int mpk_read_metadata(const uint8_t *data, uint32_t size, mpk_metadata_t *meta);

/* 获取文件数量 */
int mpk_get_file_count(const uint8_t *data, uint32_t size);

/* 获取文件条目 */
int mpk_get_file_entry(const uint8_t *data, uint32_t size,
                       int index, mpk_file_entry_t *entry);

/* 按名称查找文件 */
int mpk_find_file(const uint8_t *data, uint32_t size,
                  const char *filename, mpk_file_entry_t *entry);

/* 读取文件内容 */
int mpk_read_file(const uint8_t *data, uint32_t size,
                  const char *filename, uint8_t *buf, uint32_t buf_size);

/* 解压整个包 */
int mpk_extract_all(const uint8_t *data, uint32_t size,
                    const char *dest_path);

/* 计算包校验和 */
uint32_t mpk_calculate_checksum(const uint8_t *data, uint32_t size);

#endif /* MPK_H */
