/* ==========================================
 * FAT16 文件系统头文件 - fat16.h
 * 功能：
 *   1. FAT16 文件系统实现
 *   2. 支持读取和写入
 * ========================================== */

#ifndef FAT16_H
#define FAT16_H

#include "types.h"
#include "vfs.h"

/* FAT16 引导扇区结构（BIOS 参数块 BPB） */
typedef struct {
    uint8_t  jump[3];           /* 跳转指令 */
    uint8_t  oem_name[8];       /* OEM 名称 */
    uint16_t bytes_per_sector;  /* 每扇区字节数 */
    uint8_t  sectors_per_cluster; /* 每簇扇区数 */
    uint16_t reserved_sectors;  /* 保留扇区数 */
    uint8_t  fat_count;         /* FAT 表数量 */
    uint16_t root_entry_count;  /* 根目录条目数 */
    uint16_t total_sectors_16;  /* 总扇区数（16位） */
    uint8_t  media_type;        /* 介质类型 */
    uint16_t sectors_per_fat;   /* 每个 FAT 的扇区数 */
    uint16_t sectors_per_track; /* 每道扇区数 */
    uint16_t head_count;        /* 磁头数 */
    uint32_t hidden_sectors;    /* 隐藏扇区数 */
    uint32_t total_sectors_32;  /* 总扇区数（32位） */
    
    /* 扩展 BPB（FAT16） */
    uint8_t  drive_number;      /* 驱动器号 */
    uint8_t  reserved;          /* 保留 */
    uint8_t  boot_signature;    /* 引导签名 */
    uint32_t volume_id;         /* 卷序列号 */
    uint8_t  volume_label[11];  /* 卷标 */
    uint8_t  fs_type[8];        /* 文件系统类型 */
} __attribute__((packed)) fat16_boot_sector_t;

/* FAT 目录条目结构 */
typedef struct {
    uint8_t  name[8];           /* 文件名（8.3 格式） */
    uint8_t  ext[3];            /* 扩展名 */
    uint8_t  attr;              /* 属性 */
    uint8_t  reserved;          /* 保留 */
    uint8_t  create_time_tenth; /* 创建时间（10毫秒） */
    uint16_t create_time;       /* 创建时间 */
    uint16_t create_date;       /* 创建日期 */
    uint16_t access_date;       /* 访问日期 */
    uint16_t first_cluster_hi;  /* 起始簇高16位（FAT32用，FAT16为0） */
    uint16_t modify_time;       /* 修改时间 */
    uint16_t modify_date;       /* 修改日期 */
    uint16_t first_cluster_lo;  /* 起始簇低16位 */
    uint32_t file_size;         /* 文件大小（字节） */
} __attribute__((packed)) fat16_dir_entry_t;

/* 文件属性位 */
#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME_ID  0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20

/* FAT 特殊值 */
#define FAT16_CLUSTER_FREE  0x0000
#define FAT16_CLUSTER_BAD   0xFFF7
#define FAT16_CLUSTER_LAST  0xFFFF

/* FAT16 文件系统私有数据 */
typedef struct {
    uint32_t dev_id;            /* 设备ID */
    uint32_t fat_start;         /* FAT 起始扇区 */
    uint32_t root_dir_start;    /* 根目录起始扇区 */
    uint32_t data_start;        /* 数据区起始扇区 */
    uint32_t total_clusters;    /* 总簇数 */
    uint16_t bytes_per_sector;  /* 每扇区字节数 */
    uint8_t  sectors_per_cluster; /* 每簇扇区数 */
    uint16_t root_entry_count;  /* 根目录条目数 */
    uint32_t root_dir_sectors;  /* 根目录扇区数 */
    uint32_t sectors_per_fat;   /* 每个 FAT 的扇区数 */
} fat16_fsdata_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化 FAT16 文件系统 */
vfs_filesystem_t *fat16_init(uint32_t dev_id);

/* 读取引导扇区 */
int fat16_read_boot_sector(uint32_t dev_id, fat16_boot_sector_t *bs);

/* 计算文件系统参数 */
int fat16_calculate_params(fat16_boot_sector_t *bs, fat16_fsdata_t *fsdata);

#endif /* FAT16_H */
