/**
 * @file mbr.c
 * @brief MBR (Master Boot Record) 分区表解析实现
 * @version v0.6.0
 */

#include <mbr.h>
#include <block.h>
#include <string.h>
#include <vga.h>

// 分区类型名称表
typedef struct {
    uint8_t type;
    const char *name;
} partition_type_name_t;

static const partition_type_name_t type_names[] = {
    { PART_TYPE_EMPTY,        "Empty" },
    { PART_TYPE_FAT12,        "FAT12" },
    { PART_TYPE_FAT16_SMALL,  "FAT16 (<32MB)" },
    { PART_TYPE_FAT16,        "FAT16" },
    { PART_TYPE_NTFS,         "NTFS" },
    { PART_TYPE_FAT32,        "FAT32" },
    { PART_TYPE_FAT32_LBA,    "FAT32 (LBA)" },
    { PART_TYPE_FAT16_LBA,    "FAT16 (LBA)" },
    { PART_TYPE_LINUX,        "Linux" },
    { PART_TYPE_LINUX_SWAP,   "Linux Swap" },
    { PART_TYPE_EXTENDED,     "Extended" },
    { PART_TYPE_EXTENDED_LBA, "Extended (LBA)" },
    { 0, NULL }
};

/**
 * @brief 初始化 MBR 模块
 */
int mbr_init(int dev_id)
{
    mbr_t mbr;
    if (mbr_read(dev_id, &mbr) != 0) {
        return -1;
    }
    
    // 验证魔数
    if (mbr.signature != MBR_SIGNATURE) {
        vga_printf("[MBR] Invalid MBR signature: 0x%x\n", mbr.signature);
        return -1;
    }
    
    vga_printf("[MBR] Valid MBR found on device %d\n", dev_id);
    
    // 打印分区信息
    partition_info_t parts[MBR_PARTITION_COUNT];
    int count = mbr_get_partitions(dev_id, parts, MBR_PARTITION_COUNT);
    if (count > 0) {
        vga_printf("[MBR] Found %d partition(s):\n", count);
        for (int i = 0; i < count; i++) {
            vga_printf("  Partition %d: %s, %d MB, start LBA %d\n",
                      parts[i].index,
                      parts[i].type_name,
                      parts[i].size_mb,
                      parts[i].start_lba);
        }
    } else {
        vga_printf("[MBR] No valid partitions found\n");
    }
    
    return 0;
}

/**
 * @brief 读取 MBR 分区表
 */
int mbr_read(int dev_id, mbr_t *mbr)
{
    if (!mbr) {
        return -1;
    }
    
    // 读取第一个扇区（LBA 0）
    const uint8_t *data = block_read(dev_id, MBR_SECTOR);
    if (!data) {
        return -1;
    }
    
    // 复制数据
    memcpy(mbr, data, sizeof(mbr_t));
    
    return 0;
}

/**
 * @brief 获取分区信息
 */
int mbr_get_partition(int dev_id, int index, partition_info_t *info)
{
    if (!info || index < 0 || index >= MBR_PARTITION_COUNT) {
        return -1;
    }
    
    mbr_t mbr;
    if (mbr_read(dev_id, &mbr) != 0) {
        return -1;
    }
    
    mbr_partition_t *part = &mbr.partitions[index];
    
    // 检查是否是空分区
    if (part->type == PART_TYPE_EMPTY || part->sector_count == 0) {
        return -1;
    }
    
    // 填充信息
    info->index = index;
    info->type = part->type;
    info->start_lba = part->start_lba;
    info->sector_count = part->sector_count;
    info->size_mb = (part->sector_count * 512) / (1024 * 1024);
    info->active = (part->status & PART_STATUS_ACTIVE) ? true : false;
    info->type_name = mbr_get_type_name(part->type);
    
    return 0;
}

/**
 * @brief 获取所有有效分区
 */
int mbr_get_partitions(int dev_id, partition_info_t *partitions, int max_count)
{
    if (!partitions || max_count <= 0) {
        return 0;
    }
    
    int count = 0;
    for (int i = 0; i < MBR_PARTITION_COUNT && count < max_count; i++) {
        if (mbr_get_partition(dev_id, i, &partitions[count]) == 0) {
            count++;
        }
    }
    
    return count;
}

/**
 * @brief 获取分区类型名称
 */
const char *mbr_get_type_name(uint8_t type)
{
    for (int i = 0; type_names[i].name != NULL; i++) {
        if (type_names[i].type == type) {
            return type_names[i].name;
        }
    }
    return "Unknown";
}

/**
 * @brief 检测分区是否是 FAT 文件系统
 */
bool mbr_is_fat_partition(uint8_t type)
{
    switch (type) {
        case PART_TYPE_FAT12:
        case PART_TYPE_FAT16_SMALL:
        case PART_TYPE_FAT16:
        case PART_TYPE_FAT32:
        case PART_TYPE_FAT32_LBA:
        case PART_TYPE_FAT16_LBA:
            return true;
        default:
            return false;
    }
}
