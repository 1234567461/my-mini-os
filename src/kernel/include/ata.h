/* ==========================================
 * ATA/IDE 磁盘驱动头文件 - ata.h
 * 功能：
 *   1. ATA/IDE 磁盘驱动（PIO 模式）
 *   2. 支持读写磁盘扇区
 * ========================================== */

#ifndef ATA_H
#define ATA_H

#include "types.h"

/* ATA 端口（Primary Channel） */
#define ATA_PRIMARY_DATA     0x1F0
#define ATA_PRIMARY_ERROR    0x1F1
#define ATA_PRIMARY_SECTOR   0x1F2
#define ATA_PRIMARY_LBA_LO   0x1F3
#define ATA_PRIMARY_LBA_MID  0x1F4
#define ATA_PRIMARY_LBA_HI   0x1F5
#define ATA_PRIMARY_DRIVE    0x1F6
#define ATA_PRIMARY_STATUS   0x1F7
#define ATA_PRIMARY_COMMAND  0x1F7
#define ATA_PRIMARY_ALTSTATUS 0x3F6
#define ATA_PRIMARY_CONTROL  0x3F6

/* ATA 状态寄存器位 */
#define ATA_STATUS_ERR       0x01  /* 错误 */
#define ATA_STATUS_IDX       0x02  /* 索引 */
#define ATA_STATUS_CORR      0x04  /* 校正数据 */
#define ATA_STATUS_DRQ       0x08  /* 数据请求 */
#define ATA_STATUS_SRV       0x10  /* 重叠模式服务 */
#define ATA_STATUS_DF        0x20  /* 驱动器故障 */
#define ATA_STATUS_RDY       0x40  /* 驱动器就绪 */
#define ATA_STATUS_BSY       0x80  /* 忙 */

/* ATA 命令 */
#define ATA_CMD_READ_PIO     0x20  /* 读取扇区（PIO） */
#define ATA_CMD_WRITE_PIO    0x30  /* 写入扇区（PIO） */
#define ATA_CMD_IDENTIFY     0xEC  /* 识别驱动器 */

/* ATA 驱动器选择 */
#define ATA_DRIVE_MASTER     0xE0  /* 主驱动器 */
#define ATA_DRIVE_SLAVE      0xF0  /* 从驱动器 */

/* 设备 ID */
#define ATA_DEV_MASTER       0x00  /* 主驱动器设备ID */
#define ATA_DEV_SLAVE        0x01  /* 从驱动器设备ID */

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化 ATA/IDE 驱动 */
void ata_init(void);

/* 读取扇区 */
int ata_read_sector(uint32_t dev_id, uint32_t lba, uint8_t *buf);

/* 写入扇区 */
int ata_write_sector(uint32_t dev_id, uint32_t lba, const uint8_t *buf);

/* 检测驱动器是否存在 */
bool ata_drive_present(uint32_t dev_id);

/* 获取驱动器总扇区数 */
uint32_t ata_get_sector_count(uint32_t dev_id);

/* 注册为块设备 */
void ata_register_as_block_device(void);

#endif /* ATA_H */
