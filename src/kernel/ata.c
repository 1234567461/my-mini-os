/* ==========================================
 * ATA/IDE 磁盘驱动实现 - ata.c
 * 功能：
 *   1. ATA/IDE 磁盘驱动（PIO 模式）
 *   2. 支持读写磁盘扇区
 * ========================================== */

#include "ata.h"
#include "types.h"
#include "vga.h"
#include "block.h"
#include "string.h"

/* 驱动器信息 */
static bool drive_present[2] = {false, false};
static uint32_t drive_sectors[2] = {0, 0};

/* ==========================================
 * 函数：ata_wait_ready
 * 功能：等待驱动器就绪（BSY 位清除）
 * ========================================== */
static void ata_wait_ready(void)
{
    /* 等待 BSY 位清除 */
    while (inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BSY) {
        /* 忙等待 */
    }
}

/* ==========================================
 * 函数：ata_wait_drq
 * 功能：等待数据请求（DRQ 位置位）
 * ========================================== */
static int ata_wait_drq(void)
{
    /* 等待 DRQ 位设置或错误 */
    uint8_t status;
    do {
        status = inb(ATA_PRIMARY_STATUS);
        if (status & ATA_STATUS_ERR) {
            return -1;  /* 错误 */
        }
    } while (!(status & ATA_STATUS_DRQ));
    
    return 0;
}

/* ==========================================
 * 函数：ata_select_drive
 * 功能：选择驱动器（主/从）
 * ========================================== */
static void ata_select_drive(uint32_t dev_id)
{
    uint8_t drive = (dev_id == ATA_DEV_MASTER) ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE;
    outb(ATA_PRIMARY_DRIVE, drive);
    
    /* 等待一小段时间让驱动器切换 */
    for (volatile int i = 0; i < 1000; i++) {
        asm volatile ("nop");
    }
}

/* ==========================================
 * 函数：ata_read_sector
 * 功能：读取一个扇区（512字节）
 * 参数：
 *   dev_id - 设备ID（0=主，1=从）
 *   lba - 逻辑块地址（28位）
 *   buf - 数据缓冲区（至少512字节）
 * 返回：0=成功，-1=失败
 * ========================================== */
int ata_read_sector(uint32_t dev_id, uint32_t lba, uint8_t *buf)
{
    if (!drive_present[dev_id]) {
        return -1;
    }
    
    /* 等待驱动器就绪 */
    ata_wait_ready();
    
    /* 选择驱动器 */
    ata_select_drive(dev_id);
    
    /* 发送 LBA 地址（28位） */
    outb(ATA_PRIMARY_SECTOR, 1);  /* 读取1个扇区 */
    outb(ATA_PRIMARY_LBA_LO, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_DRIVE, 
         (dev_id == ATA_DEV_MASTER ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE) | 
         ((lba >> 24) & 0x0F));
    
    /* 发送读取命令 */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_PIO);
    
    /* 等待数据请求 */
    if (ata_wait_drq() < 0) {
        return -1;
    }
    
    /* 读取数据（256个16位字 = 512字节） */
    uint16_t *data = (uint16_t *)buf;
    for (int i = 0; i < 256; i++) {
        data[i] = inw(ATA_PRIMARY_DATA);
    }
    
    return 0;
}

/* ==========================================
 * 函数：ata_write_sector
 * 功能：写入一个扇区（512字节）
 * 参数：
 *   dev_id - 设备ID（0=主，1=从）
 *   lba - 逻辑块地址（28位）
 *   buf - 数据缓冲区（512字节）
 * 返回：0=成功，-1=失败
 * ========================================== */
int ata_write_sector(uint32_t dev_id, uint32_t lba, const uint8_t *buf)
{
    if (!drive_present[dev_id]) {
        return -1;
    }
    
    /* 等待驱动器就绪 */
    ata_wait_ready();
    
    /* 选择驱动器 */
    ata_select_drive(dev_id);
    
    /* 发送 LBA 地址（28位） */
    outb(ATA_PRIMARY_SECTOR, 1);  /* 写入1个扇区 */
    outb(ATA_PRIMARY_LBA_LO, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_DRIVE, 
         (dev_id == ATA_DEV_MASTER ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE) | 
         ((lba >> 24) & 0x0F));
    
    /* 发送写入命令 */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_PIO);
    
    /* 等待数据请求 */
    if (ata_wait_drq() < 0) {
        return -1;
    }
    
    /* 写入数据（256个16位字 = 512字节） */
    const uint16_t *data = (const uint16_t *)buf;
    for (int i = 0; i < 256; i++) {
        outw(ATA_PRIMARY_DATA, data[i]);
    }
    
    /* 等待写入完成 */
    ata_wait_ready();
    
    return 0;
}

/* ==========================================
 * 函数：ata_identify
 * 功能：识别驱动器，获取信息
 * ========================================== */
static int ata_identify(uint32_t dev_id, uint16_t *identify_data)
{
    /* 选择驱动器 */
    ata_select_drive(dev_id);
    
    /* 发送 IDENTIFY 命令 */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);
    
    /* 等待状态变化 */
    uint8_t status = inb(ATA_PRIMARY_STATUS);
    if (status == 0) {
        return -1;  /* 驱动器不存在 */
    }
    
    /* 等待就绪 */
    ata_wait_ready();
    
    /* 检查是否有数据请求 */
    if (!(inb(ATA_PRIMARY_STATUS) & ATA_STATUS_DRQ)) {
        return -1;
    }
    
    /* 读取识别数据（256个16位字） */
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(ATA_PRIMARY_DATA);
    }
    
    return 0;
}

/* ==========================================
 * 函数：ata_drive_present
 * 功能：检测驱动器是否存在
 * ========================================== */
bool ata_drive_present(uint32_t dev_id)
{
    return drive_present[dev_id];
}

/* ==========================================
 * 函数：ata_get_sector_count
 * 功能：获取驱动器总扇区数
 * ========================================== */
uint32_t ata_get_sector_count(uint32_t dev_id)
{
    return drive_sectors[dev_id];
}

/* ==========================================
 * 函数：ata_init
 * 功能：初始化 ATA/IDE 驱动
 * ========================================== */
void ata_init(void)
{
    uint16_t identify_data[256];
    
    /* 检测主驱动器 */
    if (ata_identify(ATA_DEV_MASTER, identify_data) == 0) {
        drive_present[ATA_DEV_MASTER] = true;
        /* 获取总扇区数（字60-61，28位LBA容量） */
        drive_sectors[ATA_DEV_MASTER] = identify_data[60] | (identify_data[61] << 16);
        vga_printf("  [✓] ATA Master Drive: %d sectors (%d MB)\n", 
                   drive_sectors[ATA_DEV_MASTER],
                   drive_sectors[ATA_DEV_MASTER] / 2048);
    } else {
        drive_present[ATA_DEV_MASTER] = false;
        vga_puts("  [ ] ATA Master Drive: not present\n");
    }
    
    /* 检测从驱动器 */
    if (ata_identify(ATA_DEV_SLAVE, identify_data) == 0) {
        drive_present[ATA_DEV_SLAVE] = true;
        drive_sectors[ATA_DEV_SLAVE] = identify_data[60] | (identify_data[61] << 16);
        vga_printf("  [✓] ATA Slave Drive: %d sectors (%d MB)\n",
                   drive_sectors[ATA_DEV_SLAVE],
                   drive_sectors[ATA_DEV_SLAVE] / 2048);
    } else {
        drive_present[ATA_DEV_SLAVE] = false;
        vga_puts("  [ ] ATA Slave Drive: not present\n");
    }
}

/* ==========================================
 * 块设备接口：读块
 * ========================================== */
static int ata_block_read(uint32_t dev_id, uint32_t block_no, uint8_t *buf)
{
    return ata_read_sector(dev_id, block_no, buf);
}

/* ==========================================
 * 块设备接口：写块
 * ========================================== */
static int ata_block_write(uint32_t dev_id, uint32_t block_no, const uint8_t *buf)
{
    return ata_write_sector(dev_id, block_no, buf);
}

/* ==========================================
 * 函数：ata_register_as_block_device
 * 功能：将 ATA 驱动器注册为块设备
 * ========================================== */
void ata_register_as_block_device(void)
{
    static block_device_t ata_master_dev;
    static block_device_t ata_slave_dev;
    
    if (drive_present[ATA_DEV_MASTER]) {
        memset(&ata_master_dev, 0, sizeof(block_device_t));
        ata_master_dev.name = "ata-master";
        ata_master_dev.dev_id = ATA_DEV_MASTER;
        ata_master_dev.block_count = drive_sectors[ATA_DEV_MASTER];
        ata_master_dev.read_block = ata_block_read;
        ata_master_dev.write_block = ata_block_write;
        block_register_device(&ata_master_dev);
    }
    
    if (drive_present[ATA_DEV_SLAVE]) {
        memset(&ata_slave_dev, 0, sizeof(block_device_t));
        ata_slave_dev.name = "ata-slave";
        ata_slave_dev.dev_id = ATA_DEV_SLAVE;
        ata_slave_dev.block_count = drive_sectors[ATA_DEV_SLAVE];
        ata_slave_dev.read_block = ata_block_read;
        ata_slave_dev.write_block = ata_block_write;
        block_register_device(&ata_slave_dev);
    }
}
