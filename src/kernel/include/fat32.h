/* ==========================================
 * FAT32 文件系统头文件 - fat32.h
 * 功能：
 *   1. FAT32 文件系统实现
 *   2. 支持读取和写入
 *   3. VFS 接口兼容
 * ========================================== */
#ifndef FAT32_H
#define FAT32_H

#include "types.h"
#include "vfs.h"

/* FAT32 引导扇区结构（BIOS 参数块 BPB） */
typedef struct {
    uint8_t  jump[3];                    /* 跳转指令 */
    uint8_t  oem_name[8];                /* OEM 名称 */
    uint16_t bytes_per_sector;           /* 每扇区字节数 */
    uint8_t  sectors_per_cluster;        /* 每簇扇区数 */
    uint16_t reserved_sectors;           /* 保留扇区数 */
    uint8_t  fat_count;                  /* FAT 表数量 */
    uint16_t root_entry_count;           /* 根目录条目数（FAT32 为 0） */
    uint16_t total_sectors_16;           /* 总扇区数（16位，FAT32 为 0） */
    uint8_t  media_type;                 /* 介质类型 */
    uint16_t sectors_per_fat_16;         /* 每个 FAT 的扇区数（FAT32 为 0） */
    uint16_t sectors_per_track;          /* 每道扇区数 */
    uint16_t head_count;                 /* 磁头数 */
    uint32_t hidden_sectors;             /* 隐藏扇区数 */
    uint32_t total_sectors_32;           /* 总扇区数（32位） */
    
    /* FAT32 扩展 BPB */
    uint32_t sectors_per_fat_32;         /* 每个 FAT 的扇区数（32位） */
    uint16_t ext_flags;                  /* 扩展标志 */
    uint16_t fs_version;                 /* 文件系统版本 */
    uint32_t root_cluster;               /* 根目录起始簇号（通常为 2） */
    uint16_t fs_info_sector;             /* FSInfo 扇区号 */
    uint16_t backup_boot_sector;         /* 备份引导扇区号 */
    uint8_t  reserved[12];               /* 保留 */
    uint8_t  drive_number;               /* 驱动器号 */
    uint8_t  reserved1;                  /* 保留 */
    uint8_t  boot_signature;             /* 引导签名 */
    uint32_t volume_id;                  /* 卷序列号 */
    uint8_t  volume_label[11];           /* 卷标 */
    uint8_t  fs_type[8];                 /* 文件系统类型（"FAT32   "） */
} __attribute__((packed)) fat32_boot_sector_t;

/* FAT32 FSInfo 扇区结构 */
typedef struct {
    uint32_t lead_signature;             /* 起始签名 0x41615252 */
    uint8_t  reserved[480];              /* 保留 */
    uint32_t struct_signature;           /* 结构签名 0x61417272 */
    uint32_t free_cluster_count;         /* 空闲簇数量（0xFFFFFFFF 表示未知） */
    uint32_t next_free_cluster;          /* 下一个空闲簇（0xFFFFFFFF 表示未知） */
    uint8_t  reserved2[12];              /* 保留 */
    uint32_t trail_signature;            /* 结束签名 0xAA550000 */
} __attribute__((packed)) fat32_fsinfo_t;

/* FAT 目录条目结构（与 FAT16 相同） */
typedef struct {
    uint8_t  name[8];                    /* 文件名（8.3 格式） */
    uint8_t  ext[3];                     /* 扩展名 */
    uint8_t  attr;                       /* 属性 */
    uint8_t  reserved;                   /* 保留 */
    uint8_t  create_time_tenth;          /* 创建时间（10毫秒） */
    uint16_t create_time;                /* 创建时间 */
    uint16_t create_date;                /* 创建日期 */
    uint16_t access_date;                /* 访问日期 */
    uint16_t first_cluster_hi;           /* 起始簇高16位 */
    uint16_t modify_time;                /* 修改时间 */
    uint16_t modify_date;                /* 修改日期 */
    uint16_t first_cluster_lo;           /* 起始簇低16位 */
    uint32_t file_size;                  /* 文件大小（字节） */
} __attribute__((packed)) fat32_dir_entry_t;

/* 文件属性位（与 FAT16 相同） */
#define FAT32_ATTR_READ_ONLY  0x01
#define FAT32_ATTR_HIDDEN     0x02
#define FAT32_ATTR_SYSTEM     0x04
#define FAT32_ATTR_VOLUME_ID  0x08
#define FAT32_ATTR_DIRECTORY  0x10
#define FAT32_ATTR_ARCHIVE    0x20
#define FAT32_ATTR_LONG_NAME  0x0F  /* 长文件名条目 */

/* FAT 特殊值（FAT32） */
#define FAT32_CLUSTER_FREE     0x00000000
#define FAT32_CLUSTER_BAD      0x0FFFFFF7
#define FAT32_CLUSTER_LAST     0x0FFFFFFF
#define FAT32_CLUSTER_MASK     0x0FFFFFFF  /* 有效位掩码 */

/* FAT32 签名 */
#define FAT32_FSINFO_LEAD_SIG    0x41615252
#define FAT32_FSINFO_STRUCT_SIG  0x61417272
#define FAT32_FSINFO_TRAIL_SIG   0xAA550000

/* FAT32 文件系统私有数据 */
typedef struct {
    uint32_t dev_id;                    /* 设备ID */
    uint32_t fat_start;                 /* FAT 起始扇区 */
    uint32_t data_start;                /* 数据区起始扇区 */
    uint32_t total_clusters;            /* 总簇数 */
    uint32_t root_cluster;              /* 根目录起始簇 */
    uint32_t fsinfo_sector;             /* FSInfo 扇区号 */
    uint16_t bytes_per_sector;          /* 每扇区字节数 */
    uint8_t  sectors_per_cluster;       /* 每簇扇区数 */
    uint32_t sectors_per_fat;           /* 每个 FAT 的扇区数 */
    uint8_t  fat_count;                 /* FAT 表数量 */
    
    /* 缓存的 FSInfo 数据 */
    uint32_t free_cluster_count;        /* 空闲簇数量 */
    uint32_t next_free_cluster;         /* 下一个空闲簇 */
} fat32_fsdata_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化 FAT32 文件系统 */
vfs_filesystem_t *fat32_init(uint32_t dev_id);

/* 读取引导扇区 */
int fat32_read_boot_sector(uint32_t dev_id, fat32_boot_sector_t *bs);

/* 计算文件系统参数 */
int fat32_calculate_params(fat32_boot_sector_t *bs, fat32_fsdata_t *fsdata);

/* 读取 FSInfo */
int fat32_read_fsinfo(uint32_t dev_id, uint32_t fsinfo_sector, fat32_fsinfo_t *fsinfo);

/* 读取 FAT 表项 */
uint32_t fat32_read_fat_entry(fat32_fsdata_t *fsdata, uint32_t cluster);

/* 写入 FAT 表项 */
int fat32_write_fat_entry(fat32_fsdata_t *fsdata, uint32_t cluster, uint32_t value);

/* 分配一个空闲簇 */
uint32_t fat32_alloc_cluster(fat32_fsdata_t *fsdata);

/* 释放整个簇链 */
void fat32_free_cluster_chain(fat32_fsdata_t *fsdata, uint32_t first_cluster);

/* 簇号转扇区号 */
uint32_t fat32_cluster_to_sector(fat32_fsdata_t *fsdata, uint32_t cluster);

#endif /* FAT32_H */
