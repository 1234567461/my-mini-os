/* ==========================================
 * MBR 分区表解析头文件 - mbr.h
 * 功能：
 *   1. MBR 主引导记录解析
 *   2. 分区表读取和验证
 *   3. 分区类型识别
 * ========================================== */
#ifndef MBR_H
#define MBR_H

#include "types.h"

/* MBR 魔数 */
#define MBR_SIGNATURE       0xAA55
#define MBR_SIGNATURE_OFFSET 510

/* 分区表位置 */
#define MBR_PARTITION_TABLE_OFFSET 0x1BE
#define MBR_PARTITION_COUNT        4

/* 分区类型 */
#define PART_TYPE_EMPTY           0x00
#define PART_TYPE_FAT12           0x01
#define PART_TYPE_FAT16_SMALL     0x04
#define PART_TYPE_EXTENDED        0x05
#define PART_TYPE_FAT16           0x06
#define PART_TYPE_NTFS            0x07
#define PART_TYPE_FAT32_CHS       0x0B
#define PART_TYPE_FAT32_LBA       0x0C
#define PART_TYPE_FAT16_LBA       0x0E
#define PART_TYPE_EXTENDED_LBA    0x0F
#define PART_TYPE_LINUX_SWAP      0x82
#define PART_TYPE_LINUX           0x83
#define PART_TYPE_LINUX_EXTENDED  0x85

/* 分区状态 */
#define PART_STATUS_ACTIVE        0x80
#define PART_STATUS_INACTIVE      0x00

/* ==========================================
 * CHS 地址结构体
 * ========================================== */
typedef struct {
    uint8_t head;           /* 磁头号 */
    uint8_t sector;         /* 扇区号（低6位） + 柱面高2位 */
    uint8_t cylinder;       /* 柱面号低8位 */
} __attribute__((packed)) mbr_chs_t;

/* ==========================================
 * 分区表条目结构体
 * ========================================== */
typedef struct {
    uint8_t  status;        /* 状态（0x80=活动，0x00=非活动） */
    mbr_chs_t start_chs;    /* 起始 CHS 地址 */
    uint8_t  type;          /* 分区类型 */
    mbr_chs_t end_chs;      /* 结束 CHS 地址 */
    uint32_t start_lba;     /* 起始 LBA 地址 */
    uint32_t sector_count;  /* 扇区数 */
} __attribute__((packed)) mbr_partition_t;

/* ==========================================
 * MBR 结构体
 * ========================================== */
typedef struct {
    uint8_t  boot_code[446];           /* 引导代码 */
    mbr_partition_t partitions[4];     /* 分区表（4个主分区） */
    uint16_t signature;                /* 魔数 0xAA55 */
} __attribute__((packed)) mbr_t;

/* ==========================================
 * 分区信息结构体
 * ========================================== */
typedef struct {
    int index;              /* 分区索引（0-3） */
    bool active;            /* 是否活动分区 */
    uint8_t type;           /* 分区类型 */
    uint32_t start_lba;     /* 起始 LBA */
    uint32_t sector_count;  /* 扇区数 */
    uint32_t size_mb;       /* 大小（MB） */
    char type_name[32];     /* 分区类型名称 */
} partition_info_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 读取并解析 MBR */
int mbr_read(uint32_t dev_id, mbr_t *mbr);

/* 验证 MBR 签名 */
bool mbr_validate(mbr_t *mbr);

/* 获取分区信息 */
int mbr_get_partition_info(mbr_t *mbr, int index, partition_info_t *info);

/* 获取分区类型名称 */
const char *mbr_get_partition_type_name(uint8_t type);

/* 列出所有分区 */
int mbr_list_partitions(uint32_t dev_id, partition_info_t *partitions, int max_count);

/* 查找指定类型的分区 */
int mbr_find_partition_by_type(uint32_t dev_id, uint8_t type, partition_info_t *result);

/* 查找活动分区 */
int mbr_find_active_partition(uint32_t dev_id, partition_info_t *result);

#endif /* MBR_H */
