/* ==========================================
 * MBR 分区表解析实现 - mbr.c
 * 功能：
 *   1. MBR 主引导记录解析
 *   2. 分区表读取和验证
 *   3. 分区类型识别
 * ========================================== */
#include "mbr.h"
#include "block.h"
#include "string.h"
#include "vga.h"

/* ==========================================
 * 函数：mbr_read
 * 功能：读取并解析 MBR
 * 参数：
 *   dev_id - 设备ID
 *   mbr - MBR 结构体指针
 * 返回：0=成功，-1=失败
 * ========================================== */
int mbr_read(uint32_t dev_id, mbr_t *mbr)
{
    if (mbr == NULL) {
        return -1;
    }

    /* 读取第一个扇区（MBR） */
    uint8_t *buf = block_read(dev_id, 0);
    if (buf == NULL) {
        return -1;
    }

    /* 复制数据到 MBR 结构体 */
    memcpy(mbr, buf, sizeof(mbr_t));

    return 0;
}

/* ==========================================
 * 函数：mbr_validate
 * 功能：验证 MBR 签名
 * ========================================== */
bool mbr_validate(mbr_t *mbr)
{
    if (mbr == NULL) {
        return false;
    }

    return mbr->signature == MBR_SIGNATURE;
}

/* ==========================================
 * 函数：mbr_get_partition_type_name
 * 功能：获取分区类型名称
 * ========================================== */
const char *mbr_get_partition_type_name(uint8_t type)
{
    switch (type) {
        case PART_TYPE_EMPTY:           return "Empty";
        case PART_TYPE_FAT12:           return "FAT12";
        case PART_TYPE_FAT16_SMALL:     return "FAT16 (<32MB)";
        case PART_TYPE_EXTENDED:        return "Extended (CHS)";
        case PART_TYPE_FAT16:           return "FAT16";
        case PART_TYPE_NTFS:            return "NTFS / HPFS";
        case PART_TYPE_FAT32_CHS:       return "FAT32 (CHS)";
        case PART_TYPE_FAT32_LBA:       return "FAT32 (LBA)";
        case PART_TYPE_FAT16_LBA:       return "FAT16 (LBA)";
        case PART_TYPE_EXTENDED_LBA:    return "Extended (LBA)";
        case PART_TYPE_LINUX_SWAP:      return "Linux Swap";
        case PART_TYPE_LINUX:           return "Linux Native";
        case PART_TYPE_LINUX_EXTENDED:  return "Linux Extended";
        default:                        return "Unknown";
    }
}

/* ==========================================
 * 函数：mbr_get_partition_info
 * 功能：获取分区信息
 * ========================================== */
int mbr_get_partition_info(mbr_t *mbr, int index, partition_info_t *info)
{
    if (mbr == NULL || info == NULL || index < 0 || index >= MBR_PARTITION_COUNT) {
        return -1;
    }

    mbr_partition_t *part = &mbr->partitions[index];

    info->index = index;
    info->active = (part->status & PART_STATUS_ACTIVE) != 0;
    info->type = part->type;
    info->start_lba = part->start_lba;
    info->sector_count = part->sector_count;
    info->size_mb = part->sector_count / 2048;  /* 1 MB = 2048 扇区 */

    /* 获取类型名称 */
    strncpy(info->type_name, mbr_get_partition_type_name(part->type), 31);
    info->type_name[31] = '\0';

    return 0;
}

/* ==========================================
 * 函数：mbr_list_partitions
 * 功能：列出所有分区
 * 返回：找到的分区数量
 * ========================================== */
int mbr_list_partitions(uint32_t dev_id, partition_info_t *partitions, int max_count)
{
    if (partitions == NULL || max_count <= 0) {
        return -1;
    }

    mbr_t mbr;
    if (mbr_read(dev_id, &mbr) < 0) {
        return -1;
    }

    if (!mbr_validate(&mbr)) {
        return -1;
    }

    int count = 0;
    for (int i = 0; i < MBR_PARTITION_COUNT && count < max_count; i++) {
        if (mbr.partitions[i].type != PART_TYPE_EMPTY) {
            mbr_get_partition_info(&mbr, i, &partitions[count]);
            count++;
        }
    }

    return count;
}

/* ==========================================
 * 函数：mbr_find_partition_by_type
 * 功能：查找指定类型的分区
 * 返回：找到的分区索引，-1=未找到
 * ========================================== */
int mbr_find_partition_by_type(uint32_t dev_id, uint8_t type, partition_info_t *result)
{
    if (result == NULL) {
        return -1;
    }

    mbr_t mbr;
    if (mbr_read(dev_id, &mbr) < 0) {
        return -1;
    }

    if (!mbr_validate(&mbr)) {
        return -1;
    }

    for (int i = 0; i < MBR_PARTITION_COUNT; i++) {
        if (mbr.partitions[i].type == type) {
            mbr_get_partition_info(&mbr, i, result);
            return i;
        }
    }

    return -1;
}

/* ==========================================
 * 函数：mbr_find_active_partition
 * 功能：查找活动分区
 * 返回：找到的分区索引，-1=未找到
 * ========================================== */
int mbr_find_active_partition(uint32_t dev_id, partition_info_t *result)
{
    if (result == NULL) {
        return -1;
    }

    mbr_t mbr;
    if (mbr_read(dev_id, &mbr) < 0) {
        return -1;
    }

    if (!mbr_validate(&mbr)) {
        return -1;
    }

    for (int i = 0; i < MBR_PARTITION_COUNT; i++) {
        if (mbr.partitions[i].status & PART_STATUS_ACTIVE) {
            mbr_get_partition_info(&mbr, i, result);
            return i;
        }
    }

    return -1;
}
