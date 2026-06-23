/* ==========================================
 * FAT16 文件系统实现 - fat16.c
 * 功能：
 *   1. FAT16 文件系统读取支持
 *   2. VFS 接口实现
 * ========================================== */

#include "fat16.h"
#include "block.h"
#include "string.h"
#include "heap.h"
#include "vga.h"

/* 全局 FAT16 文件系统数据（简化版，只支持一个实例） */
static fat16_fsdata_t *fat16_fsdata = NULL;
static vfs_filesystem_t fat16_fs;
static vfs_ops_t fat16_ops;

/* ==========================================
 * 辅助函数：簇号转扇区号
 * ========================================== */
static uint32_t cluster_to_sector(uint16_t cluster)
{
    /* 数据区从 2 号簇开始 */
    return fat16_fsdata->data_start + (cluster - 2) * fat16_fsdata->sectors_per_cluster;
}

/* ==========================================
 * 函数：fat16_read_boot_sector
 * 功能：读取引导扇区
 * ========================================== */
int fat16_read_boot_sector(uint32_t dev_id, fat16_boot_sector_t *bs)
{
    uint8_t *buf = block_read(dev_id, 0);
    if (buf == NULL) {
        return -1;
    }
    
    memcpy(bs, buf, sizeof(fat16_boot_sector_t));
    
    return 0;
}

/* ==========================================
 * 函数：fat16_calculate_params
 * 功能：计算文件系统参数
 * ========================================== */
int fat16_calculate_params(fat16_boot_sector_t *bs, fat16_fsdata_t *fsdata)
{
    /* 基本参数 */
    fsdata->bytes_per_sector = bs->bytes_per_sector;
    fsdata->sectors_per_cluster = bs->sectors_per_cluster;
    fsdata->root_entry_count = bs->root_entry_count;
    fsdata->sectors_per_fat = bs->sectors_per_fat;
    
    /* 计算各部分起始扇区 */
    fsdata->fat_start = bs->reserved_sectors;
    fsdata->root_dir_start = fsdata->fat_start + bs->fat_count * bs->sectors_per_fat;
    
    /* 根目录占用的扇区数 */
    fsdata->root_dir_sectors = (bs->root_entry_count * 32 + bs->bytes_per_sector - 1) / bs->bytes_per_sector;
    
    /* 数据区起始扇区 */
    fsdata->data_start = fsdata->root_dir_start + fsdata->root_dir_sectors;
    
    /* 总扇区数 */
    uint32_t total_sectors = bs->total_sectors_16;
    if (total_sectors == 0) {
        total_sectors = bs->total_sectors_32;
    }
    
    /* 总簇数 */
    uint32_t data_sectors = total_sectors - fsdata->data_start;
    fsdata->total_clusters = data_sectors / bs->sectors_per_cluster;
    
    return 0;
}

/* ==========================================
 * 函数：fat16_read_fat_entry
 * 功能：读取 FAT 表中的一个条目
 * ========================================== */
static uint16_t fat16_read_fat_entry(uint16_t cluster)
{
    uint32_t fat_offset = cluster * 2;  /* 每个 FAT 条目 2 字节 */
    uint32_t sector = fat16_fsdata->fat_start + fat_offset / fat16_fsdata->bytes_per_sector;
    uint32_t offset = fat_offset % fat16_fsdata->bytes_per_sector;
    
    uint8_t *buf = block_read(fat16_fsdata->dev_id, sector);
    if (buf == NULL) {
        return FAT16_CLUSTER_BAD;
    }
    
    uint16_t value = *(uint16_t *)(buf + offset);
    return value;
}

/* ==========================================
 * 函数：fat16_convert_name
 * 功能：将 8.3 格式转换为普通文件名
 * ========================================== */
static void fat16_convert_name(uint8_t *name83, char *name)
{
    int i = 0;
    int j = 0;
    
    /* 复制文件名部分（8字节） */
    for (i = 0; i < 8 && name83[i] != ' '; i++) {
        name[j++] = name83[i];
    }
    
    /* 如果有扩展名，添加点和扩展名 */
    if (name83[8] != ' ') {
        name[j++] = '.';
        for (i = 0; i < 3 && name83[8 + i] != ' '; i++) {
            name[j++] = name83[8 + i];
        }
    }
    
    name[j] = '\0';
}

/* ==========================================
 * 函数：fat16_find_entry
 * 功能：在目录中查找文件条目
 * ========================================== */
static int fat16_find_entry(const char *path, fat16_dir_entry_t *entry)
{
    /* 简化版：只支持根目录下的文件 */
    if (path[0] != '/' || path[1] == '\0') {
        return -1;
    }
    
    /* 跳过开头的 / */
    const char *filename = path + 1;
    
    /* 遍历根目录 */
    for (uint32_t i = 0; i < fat16_fsdata->root_entry_count; i++) {
        uint32_t sector = fat16_fsdata->root_dir_start + (i * 32) / fat16_fsdata->bytes_per_sector;
        uint32_t offset = (i * 32) % fat16_fsdata->bytes_per_sector;
        
        uint8_t *buf = block_read(fat16_fsdata->dev_id, sector);
        if (buf == NULL) {
            return -1;
        }
        
        fat16_dir_entry_t *dir_entry = (fat16_dir_entry_t *)(buf + offset);
        
        /* 检查是否是空条目 */
        if (dir_entry->name[0] == 0x00) {
            break;  /* 目录结束 */
        }
        if (dir_entry->name[0] == 0xE5) {
            continue;  /* 已删除的条目 */
        }
        
        /* 跳过卷标和子目录（简化版） */
        if (dir_entry->attr & FAT_ATTR_VOLUME_ID) {
            continue;
        }
        
        /* 转换文件名并比较 */
        char name[13];
        fat16_convert_name(dir_entry->name, name);
        
        if (strcmp(name, filename) == 0) {
            memcpy(entry, dir_entry, sizeof(fat16_dir_entry_t));
            return 0;
        }
    }
    
    return -1;  /* 未找到 */
}

/* ==========================================
 * VFS 接口：打开文件
 * ========================================== */
static vfs_file_t *fat16_open(const char *path, int flags)
{
    (void)flags;
    
    fat16_dir_entry_t entry;
    if (fat16_find_entry(path, &entry) < 0) {
        return NULL;
    }
    
    /* 分配文件描述符 */
    vfs_file_t *file = (vfs_file_t *)kmalloc(sizeof(vfs_file_t));
    if (file == NULL) {
        return NULL;
    }
    
    /* 初始化文件信息 */
    memset(file, 0, sizeof(vfs_file_t));
    file->type = VFS_FILE;
    file->size = entry.file_size;
    file->pos = 0;
    
    /* 保存起始簇号作为 fs_data */
    uint16_t *first_cluster = (uint16_t *)kmalloc(sizeof(uint16_t));
    *first_cluster = entry.first_cluster_lo;
    file->fs_data = first_cluster;
    
    return file;
}

/* ==========================================
 * VFS 接口：关闭文件
 * ========================================== */
static int fat16_close(vfs_file_t *file)
{
    if (file->fs_data != NULL) {
        kfree(file->fs_data);
    }
    kfree(file);
    return 0;
}

/* ==========================================
 * VFS 接口：读取文件
 * ========================================== */
static int fat16_read(vfs_file_t *file, void *buf, uint32_t count)
{
    /* 简化版：暂时不实现完整的读取 */
    (void)file;
    (void)buf;
    (void)count;
    return -1;
}

/* ==========================================
 * VFS 接口：写入文件
 * ========================================== */
static int fat16_write(vfs_file_t *file, const void *buf, uint32_t count)
{
    (void)file;
    (void)buf;
    (void)count;
    return -1;
}

/* ==========================================
 * VFS 接口：创建文件
 * ========================================== */
static int fat16_create(const char *path)
{
    (void)path;
    return -1;
}

/* ==========================================
 * VFS 接口：删除文件
 * ========================================== */
static int fat16_unlink(const char *path)
{
    (void)path;
    return -1;
}

/* ==========================================
 * VFS 接口：创建目录
 * ========================================== */
static int fat16_mkdir(const char *path)
{
    (void)path;
    return -1;
}

/* ==========================================
 * VFS 接口：删除目录
 * ========================================== */
static int fat16_rmdir(const char *path)
{
    (void)path;
    return -1;
}

/* ==========================================
 * VFS 接口：读取目录
 * ========================================== */
static int fat16_readdir(const char *path, vfs_dirent_t *dirents, int max_count)
{
    (void)path;
    (void)dirents;
    (void)max_count;
    return -1;
}

/* ==========================================
 * VFS 接口：获取文件状态
 * ========================================== */
static int fat16_stat(const char *path, vfs_stat_t *st)
{
    (void)path;
    (void)st;
    return -1;
}

/* ==========================================
 * 函数：fat16_init
 * 功能：初始化 FAT16 文件系统
 * ========================================== */
vfs_filesystem_t *fat16_init(uint32_t dev_id)
{
    fat16_boot_sector_t bs;
    
    /* 读取引导扇区 */
    if (fat16_read_boot_sector(dev_id, &bs) < 0) {
        return NULL;
    }
    
    /* 检查是否是 FAT16（简化检查） */
    if (bs.bytes_per_sector != 512) {
        return NULL;
    }
    
    /* 分配文件系统私有数据 */
    fat16_fsdata = (fat16_fsdata_t *)kmalloc(sizeof(fat16_fsdata_t));
    if (fat16_fsdata == NULL) {
        return NULL;
    }
    
    memset(fat16_fsdata, 0, sizeof(fat16_fsdata_t));
    fat16_fsdata->dev_id = dev_id;
    
    /* 计算参数 */
    fat16_calculate_params(&bs, fat16_fsdata);
    
    /* 初始化 VFS 操作函数表 */
    fat16_ops.open = fat16_open;
    fat16_ops.close = fat16_close;
    fat16_ops.read = fat16_read;
    fat16_ops.write = fat16_write;
    fat16_ops.create = fat16_create;
    fat16_ops.unlink = fat16_unlink;
    fat16_ops.mkdir = fat16_mkdir;
    fat16_ops.rmdir = fat16_rmdir;
    fat16_ops.readdir = fat16_readdir;
    fat16_ops.stat = fat16_stat;
    
    /* 初始化 VFS 文件系统结构 */
    memset(&fat16_fs, 0, sizeof(vfs_filesystem_t));
    strncpy(fat16_fs.name, "fat16", 31);
    fat16_fs.name[31] = '\0';
    fat16_fs.ops = &fat16_ops;
    fat16_fs.data = fat16_fsdata;
    
    vga_printf("  [✓] FAT16 filesystem mounted (dev %d)\n", dev_id);
    vga_printf("      Total clusters: %d\n", fat16_fsdata->total_clusters);
    vga_printf("      Bytes per sector: %d\n", fat16_fsdata->bytes_per_sector);
    vga_printf("      Sectors per cluster: %d\n", fat16_fsdata->sectors_per_cluster);
    
    return &fat16_fs;
}
