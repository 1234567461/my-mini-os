/**
 * @file mbr.h
 * @brief MBR (Master Boot Record) 分区表解析
 * @version v0.6.0
 *
 * 支持读取 MBR 分区表，识别多个分区
 */

#ifndef MBR_H
#define MBR_H

#include <types.h>

#define MBR_SIGNATURE           0xAA55
#define MBR_PARTITION_COUNT     4
#define MBR_BOOT_CODE_SIZE      446
#define MBR_PARTITION_ENTRY_SIZE 16
#define MBR_SECTOR              0   // MBR 位于 LBA 0

// 分区类型常量
#define PART_TYPE_EMPTY         0x00
#define PART_TYPE_FAT12         0x01
#define PART_TYPE_FAT16_SMALL   0x04
#define PART_TYPE_FAT16         0x06
#define PART_TYPE_NTFS          0x07
#define PART_TYPE_FAT32         0x0B
#define PART_TYPE_FAT32_LBA     0x0C
#define PART_TYPE_FAT16_LBA     0x0E
#define PART_TYPE_LINUX         0x83
#define PART_TYPE_LINUX_SWAP    0x82
#define PART_TYPE_EXTENDED      0x05
#define PART_TYPE_EXTENDED_LBA  0x0F

// 分区状态
#define PART_STATUS_ACTIVE      0x80
#define PART_STATUS_INACTIVE    0x00

/**
 * @brief CHS 地址结构
 */
typedef struct {
    uint8_t head;
    uint8_t sector;     // 高2位是柱面高2位
    uint8_t cylinder;   // 柱面低8位
} __attribute__((packed)) chs_addr_t;

/**
 * @brief MBR 分区表条目结构（16字节）
 */
typedef struct {
    uint8_t status;             // 状态：0x80=活动，0x00=非活动
    chs_addr_t start_chs;       // CHS 起始地址
    uint8_t type;               // 分区类型
    chs_addr_t end_chs;         // CHS 结束地址
    uint32_t start_lba;         // LBA 起始扇区（小端）
    uint32_t sector_count;      // 分区扇区数（小端）
} __attribute__((packed)) mbr_partition_t;

/**
 * @brief MBR 结构（512字节）
 */
typedef struct {
    uint8_t boot_code[MBR_BOOT_CODE_SIZE];
    mbr_partition_t partitions[MBR_PARTITION_COUNT];
    uint16_t signature;         // 魔数 0xAA55
} __attribute__((packed)) mbr_t;

/**
 * @brief 分区信息结构（解析后的）
 */
typedef struct {
    int index;                  // 分区索引（0-3）
    uint8_t type;               // 分区类型
    uint32_t start_lba;         // 起始 LBA
    uint32_t sector_count;      // 扇区数
    uint32_t size_mb;           // 大小（MB）
    bool active;                // 是否活动分区
    const char *type_name;      // 分区类型名称
} partition_info_t;

/**
 * @brief 初始化 MBR 模块
 * @param dev_id 设备 ID
 * @return 成功返回 0，失败返回 -1
 */
int mbr_init(int dev_id);

/**
 * @brief 读取 MBR 分区表
 * @param dev_id 设备 ID
 * @param mbr 输出 MBR 结构
 * @return 成功返回 0，失败返回 -1
 */
int mbr_read(int dev_id, mbr_t *mbr);

/**
 * @brief 获取分区信息
 * @param dev_id 设备 ID
 * @param index 分区索引（0-3）
 * @param info 输出分区信息
 * @return 成功返回 0，失败返回 -1
 */
int mbr_get_partition(int dev_id, int index, partition_info_t *info);

/**
 * @brief 获取所有有效分区
 * @param dev_id 设备 ID
 * @param partitions 输出分区信息数组
 * @param max_count 最大数量
 * @return 有效分区数量
 */
int mbr_get_partitions(int dev_id, partition_info_t *partitions, int max_count);

/**
 * @brief 获取分区类型名称
 * @param type 分区类型
 * @return 类型名称字符串
 */
const char *mbr_get_type_name(uint8_t type);

/**
 * @brief 检测分区是否是 FAT 文件系统
 * @param type 分区类型
 * @return 是 FAT 返回 true
 */
bool mbr_is_fat_partition(uint8_t type);

#endif // MBR_H
