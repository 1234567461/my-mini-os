/* ==========================================
 * FAT16 文件系统实现 - fat16.c
 * 功能：
 *   1. FAT16 文件系统完整支持
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
    
    buffer_t *buf = block_get_buffer(fat16_fsdata->dev_id, sector);
    if (buf == NULL) {
        return FAT16_CLUSTER_BAD;
    }
    
    uint16_t value = *(uint16_t *)(buf->data + offset);
    block_release_buffer(buf);
    
    return value;
}

/* ==========================================
 * 函数：fat16_write_fat_entry
 * 功能：写入 FAT 表中的一个条目
 * ========================================== */
static int fat16_write_fat_entry(uint16_t cluster, uint16_t value)
{
    uint32_t fat_offset = cluster * 2;  /* 每个 FAT 条目 2 字节 */
    uint32_t sector = fat16_fsdata->fat_start + fat_offset / fat16_fsdata->bytes_per_sector;
    uint32_t offset = fat_offset % fat16_fsdata->bytes_per_sector;
    
    buffer_t *buf = block_get_buffer(fat16_fsdata->dev_id, sector);
    if (buf == NULL) {
        return -1;
    }
    
    *(uint16_t *)(buf->data + offset) = value;
    buf->flags |= BUF_DIRTY;
    block_release_buffer(buf);
    
    return 0;
}

/* ==========================================
 * 函数：fat16_alloc_cluster
 * 功能：分配一个空闲簇
 * 返回：簇号，失败返回 0
 * ========================================== */
static uint16_t fat16_alloc_cluster(void)
{
    /* 从 2 号簇开始查找空闲簇 */
    for (uint16_t cluster = 2; cluster < fat16_fsdata->total_clusters + 2; cluster++) {
        uint16_t entry = fat16_read_fat_entry(cluster);
        if (entry == FAT16_CLUSTER_FREE) {
            /* 标记为已使用（文件结束） */
            fat16_write_fat_entry(cluster, FAT16_CLUSTER_LAST);
            return cluster;
        }
    }
    
    return 0;  /* 没有空闲簇 */
}

/* ==========================================
 * 函数：fat16_free_cluster_chain
 * 功能：释放整个簇链
 * 参数：first_cluster - 起始簇号
 * ========================================== */
static void fat16_free_cluster_chain(uint16_t first_cluster)
{
    uint16_t current = first_cluster;
    
    while (current != 0 && current != FAT16_CLUSTER_LAST && current < fat16_fsdata->total_clusters + 2) {
        uint16_t next = fat16_read_fat_entry(current);
        
        /* 释放当前簇 */
        fat16_write_fat_entry(current, FAT16_CLUSTER_FREE);
        
        if (next == FAT16_CLUSTER_LAST || next == 0) {
            break;
        }
        
        current = next;
    }
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
 * 函数：fat16_make_83_name
 * 功能：将普通文件名转换为 8.3 格式
 * ========================================== */
static void fat16_make_83_name(const char *name, uint8_t *name83)
{
    int i = 0;
    int j = 0;
    
    /* 初始化全为空格 */
    memset(name83, ' ', 11);
    
    /* 查找扩展名位置 */
    const char *dot = strrchr(name, '.');
    int name_len = (dot != NULL) ? (dot - name) : strlen(name);
    int ext_len = (dot != NULL) ? strlen(dot + 1) : 0;
    
    /* 复制文件名（最多8个字符，转大写） */
    for (i = 0; i < name_len && i < 8; i++) {
        if (name[i] >= 'a' && name[i] <= 'z') {
            name83[i] = name[i] - 'a' + 'A';
        } else {
            name83[i] = name[i];
        }
    }
    
    /* 复制扩展名（最多3个字符，转大写） */
    if (dot != NULL) {
        for (i = 0, j = 8; i < ext_len && i < 3; i++, j++) {
            if (dot[i + 1] >= 'a' && dot[i + 1] <= 'z') {
                name83[j] = dot[i + 1] - 'a' + 'A';
            } else {
                name83[j] = dot[i + 1];
            }
        }
    }
}

/* ==========================================
 * 函数：fat16_find_empty_dir_entry
 * 功能：在目录中查找空条目
 * 返回：条目索引，失败返回 -1
 * ========================================== */
static int fat16_find_empty_dir_entry(uint16_t dir_cluster)
{
    uint32_t entry_count;
    uint32_t start_sector;
    uint32_t sectors;
    
    if (dir_cluster == 0) {
        /* 根目录 */
        entry_count = fat16_fsdata->root_entry_count;
        start_sector = fat16_fsdata->root_dir_start;
        sectors = fat16_fsdata->root_dir_sectors;
    } else {
        /* 子目录（简化版，暂时不支持） */
        return -1;
    }
    
    /* 遍历目录条目 */
    for (uint32_t i = 0; i < entry_count; i++) {
        uint32_t sector = start_sector + (i * 32) / fat16_fsdata->bytes_per_sector;
        uint32_t offset = (i * 32) % fat16_fsdata->bytes_per_sector;
        
        buffer_t *buf = block_get_buffer(fat16_fsdata->dev_id, sector);
        if (buf == NULL) {
            return -1;
        }
        
        fat16_dir_entry_t *dir_entry = (fat16_dir_entry_t *)(buf->data + offset);
        
        /* 检查是否是空条目或已删除条目 */
        if (dir_entry->name[0] == 0x00 || dir_entry->name[0] == 0xE5) {
            block_release_buffer(buf);
            return i;
        }
        
        block_release_buffer(buf);
    }
    
    return -1;  /* 没有空条目 */
}

/* ==========================================
 * 函数：fat16_write_dir_entry
 * 功能：写入目录条目
 * ========================================== */
static int fat16_write_dir_entry(uint16_t dir_cluster, uint32_t index, fat16_dir_entry_t *entry)
{
    uint32_t start_sector;
    
    if (dir_cluster == 0) {
        /* 根目录 */
        start_sector = fat16_fsdata->root_dir_start;
    } else {
        /* 子目录（简化版，暂时不支持） */
        return -1;
    }
    
    uint32_t sector = start_sector + (index * 32) / fat16_fsdata->bytes_per_sector;
    uint32_t offset = (index * 32) % fat16_fsdata->bytes_per_sector;
    
    buffer_t *buf = block_get_buffer(fat16_fsdata->dev_id, sector);
    if (buf == NULL) {
        return -1;
    }
    
    memcpy(buf->data + offset, entry, sizeof(fat16_dir_entry_t));
    buf->flags |= BUF_DIRTY;
    block_release_buffer(buf);
    
    return 0;
}

/* ==========================================
 * 函数：fat16_find_entry
 * 功能：在目录中查找文件条目
 * ========================================== */
static int fat16_find_entry(const char *path, fat16_dir_entry_t *entry, uint32_t *entry_index)
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
        
        buffer_t *buf = block_get_buffer(fat16_fsdata->dev_id, sector);
        if (buf == NULL) {
            return -1;
        }
        
        fat16_dir_entry_t *dir_entry = (fat16_dir_entry_t *)(buf->data + offset);
        
        /* 检查是否是空条目 */
        if (dir_entry->name[0] == 0x00) {
            block_release_buffer(buf);
            break;  /* 目录结束 */
        }
        if (dir_entry->name[0] == 0xE5) {
            block_release_buffer(buf);
            continue;  /* 已删除的条目 */
        }
        
        /* 跳过卷标 */
        if (dir_entry->attr & FAT_ATTR_VOLUME_ID) {
            block_release_buffer(buf);
            continue;
        }
        
        /* 转换文件名并比较 */
        char name[13];
        fat16_convert_name(dir_entry->name, name);
        
        if (strcmp(name, filename) == 0) {
            memcpy(entry, dir_entry, sizeof(fat16_dir_entry_t));
            if (entry_index != NULL) {
                *entry_index = i;
            }
            block_release_buffer(buf);
            return 0;
        }
        
        block_release_buffer(buf);
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
    if (fat16_find_entry(path, &entry, NULL) < 0) {
        return NULL;
    }
    
    /* 分配文件描述符 */
    vfs_file_t *file = (vfs_file_t *)kmalloc(sizeof(vfs_file_t));
    if (file == NULL) {
        return NULL;
    }
    
    /* 初始化文件信息 */
    memset(file, 0, sizeof(vfs_file_t));
    file->type = (entry.attr & FAT_ATTR_DIRECTORY) ? VFS_DIR : VFS_FILE;
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
    if (file == NULL || buf == NULL) {
        return -1;
    }
    
    uint16_t first_cluster = *(uint16_t *)file->fs_data;
    uint8_t *buffer = (uint8_t *)buf;
    uint32_t bytes_read = 0;
    uint32_t bytes_left = count;
    
    /* 如果已经读到文件末尾 */
    if (file->pos >= file->size) {
        return 0;
    }
    
    /* 调整读取长度 */
    if (file->pos + count > file->size) {
        bytes_left = file->size - file->pos;
    }
    
    /* 计算起始簇和偏移 */
    uint32_t cluster_size = fat16_fsdata->sectors_per_cluster * fat16_fsdata->bytes_per_sector;
    uint16_t current_cluster = first_cluster;
    uint32_t cluster_index = file->pos / cluster_size;
    uint32_t offset_in_cluster = file->pos % cluster_size;
    
    /* 定位到起始簇 */
    for (uint32_t i = 0; i < cluster_index; i++) {
        current_cluster = fat16_read_fat_entry(current_cluster);
        if (current_cluster == FAT16_CLUSTER_LAST || current_cluster == 0) {
            return bytes_read;
        }
    }
    
    /* 读取数据 */
    while (bytes_left > 0 && current_cluster != FAT16_CLUSTER_LAST && current_cluster != 0) {
        uint32_t sector = cluster_to_sector(current_cluster);
        uint32_t bytes_in_cluster = cluster_size - offset_in_cluster;
        uint32_t to_read = (bytes_left < bytes_in_cluster) ? bytes_left : bytes_in_cluster;
        
        /* 读取簇中的数据 */
        uint32_t bytes_read_in_cluster = 0;
        while (bytes_read_in_cluster < to_read) {
            uint32_t current_sector = sector + (offset_in_cluster + bytes_read_in_cluster) / fat16_fsdata->bytes_per_sector;
            uint32_t sector_offset = (offset_in_cluster + bytes_read_in_cluster) % fat16_fsdata->bytes_per_sector;
            uint32_t sector_bytes = fat16_fsdata->bytes_per_sector - sector_offset;
            if (sector_bytes > to_read - bytes_read_in_cluster) {
                sector_bytes = to_read - bytes_read_in_cluster;
            }
            
            buffer_t *sec_buf = block_get_buffer(fat16_fsdata->dev_id, current_sector);
            if (sec_buf == NULL) {
                return bytes_read;
            }
            
            memcpy(buffer + bytes_read, sec_buf->data + sector_offset, sector_bytes);
            block_release_buffer(sec_buf);
            
            bytes_read_in_cluster += sector_bytes;
            bytes_read += sector_bytes;
            bytes_left -= sector_bytes;
        }
        
        /* 移动到下一个簇 */
        if (bytes_left > 0) {
            current_cluster = fat16_read_fat_entry(current_cluster);
        }
        offset_in_cluster = 0;
    }
    
    file->pos += bytes_read;
    return bytes_read;
}

/* ==========================================
 * 函数：fat16_get_cluster_at_offset
 * 功能：获取文件指定偏移处的簇号
 * ========================================== */
static uint16_t fat16_get_cluster_at_offset(uint16_t first_cluster, uint32_t offset)
{
    uint32_t cluster_size = fat16_fsdata->sectors_per_cluster * fat16_fsdata->bytes_per_sector;
    uint32_t cluster_index = offset / cluster_size;
    uint16_t current_cluster = first_cluster;
    
    for (uint32_t i = 0; i < cluster_index; i++) {
        current_cluster = fat16_read_fat_entry(current_cluster);
        if (current_cluster == FAT16_CLUSTER_LAST || current_cluster == 0) {
            return 0;
        }
    }
    
    return current_cluster;
}

/* ==========================================
 * 函数：fat16_extend_file
 * 功能：扩展文件到指定大小（分配新簇）
 * 返回：成功返回 0，失败返回 -1
 * ========================================== */
static int fat16_extend_file(uint16_t first_cluster, uint32_t current_size, uint32_t new_size)
{
    uint32_t cluster_size = fat16_fsdata->sectors_per_cluster * fat16_fsdata->bytes_per_sector;
    uint32_t current_clusters = (current_size + cluster_size - 1) / cluster_size;
    uint32_t new_clusters = (new_size + cluster_size - 1) / cluster_size;
    
    if (new_clusters <= current_clusters) {
        return 0;  /* 不需要扩展 */
    }
    
    /* 找到最后一个簇 */
    uint16_t last_cluster = first_cluster;
    for (uint32_t i = 1; i < current_clusters; i++) {
        last_cluster = fat16_read_fat_entry(last_cluster);
        if (last_cluster == FAT16_CLUSTER_LAST || last_cluster == 0) {
            return -1;
        }
    }
    
    /* 分配新簇 */
    uint32_t clusters_to_add = new_clusters - current_clusters;
    for (uint32_t i = 0; i < clusters_to_add; i++) {
        uint16_t new_cluster = fat16_alloc_cluster();
        if (new_cluster == 0) {
            return -1;  /* 磁盘满 */
        }
        
        /* 链接到最后一个簇 */
        fat16_write_fat_entry(last_cluster, new_cluster);
        last_cluster = new_cluster;
    }
    
    return 0;
}

/* ==========================================
 * 函数：fat16_update_file_size
 * 功能：更新目录条目中的文件大小
 * ========================================== */
static int fat16_update_file_size(const char *path, uint32_t new_size)
{
    fat16_dir_entry_t entry;
    uint32_t entry_index;
    
    /* 查找文件 */
    if (fat16_find_entry(path, &entry, &entry_index) < 0) {
        return -1;
    }
    
    /* 更新文件大小 */
    entry.file_size = new_size;
    
    /* 写回目录条目 */
    return fat16_write_dir_entry(0, entry_index, &entry);
}

/* ==========================================
 * VFS 接口：写入文件
 * ========================================== */
static int fat16_write(vfs_file_t *file, const void *buf, uint32_t count)
{
    if (file == NULL || buf == NULL) {
        return -1;
    }
    
    const uint8_t *buffer = (const uint8_t *)buf;
    uint32_t bytes_written = 0;
    uint32_t bytes_left = count;
    
    /* 计算新的文件大小 */
    uint32_t new_size = file->pos + count;
    
    /* 如果需要，扩展文件 */
    if (new_size > file->size) {
        uint16_t first_cluster = *(uint16_t *)file->fs_data;
        if (fat16_extend_file(first_cluster, file->size, new_size) < 0) {
            return -1;
        }
    }
    
    /* 计算起始簇和偏移 */
    uint32_t cluster_size = fat16_fsdata->sectors_per_cluster * fat16_fsdata->bytes_per_sector;
    uint16_t first_cluster = *(uint16_t *)file->fs_data;
    uint16_t current_cluster = fat16_get_cluster_at_offset(first_cluster, file->pos);
    uint32_t offset_in_cluster = file->pos % cluster_size;
    
    if (current_cluster == 0) {
        return -1;
    }
    
    /* 写入数据 */
    while (bytes_left > 0 && current_cluster != 0) {
        uint32_t sector = cluster_to_sector(current_cluster);
        uint32_t bytes_in_cluster = cluster_size - offset_in_cluster;
        uint32_t to_write = (bytes_left < bytes_in_cluster) ? bytes_left : bytes_in_cluster;
        
        /* 写入簇中的数据 */
        uint32_t bytes_written_in_cluster = 0;
        while (bytes_written_in_cluster < to_write) {
            uint32_t current_sector = sector + (offset_in_cluster + bytes_written_in_cluster) / fat16_fsdata->bytes_per_sector;
            uint32_t sector_offset = (offset_in_cluster + bytes_written_in_cluster) % fat16_fsdata->bytes_per_sector;
            uint32_t sector_bytes = fat16_fsdata->bytes_per_sector - sector_offset;
            if (sector_bytes > to_write - bytes_written_in_cluster) {
                sector_bytes = to_write - bytes_written_in_cluster;
            }
            
            buffer_t *sec_buf = block_get_buffer(fat16_fsdata->dev_id, current_sector);
            if (sec_buf == NULL) {
                return bytes_written;
            }
            
            memcpy(sec_buf->data + sector_offset, buffer + bytes_written, sector_bytes);
            sec_buf->flags |= BUF_DIRTY;
            block_release_buffer(sec_buf);
            
            bytes_written_in_cluster += sector_bytes;
            bytes_written += sector_bytes;
            bytes_left -= sector_bytes;
        }
        
        /* 移动到下一个簇 */
        if (bytes_left > 0) {
            current_cluster = fat16_read_fat_entry(current_cluster);
        }
        offset_in_cluster = 0;
    }
    
    /* 更新文件位置和大小 */
    file->pos += bytes_written;
    if (file->pos > file->size) {
        file->size = file->pos;
    }
    
    return bytes_written;
}

/* ==========================================
 * VFS 接口：创建文件
 * ========================================== */
static int fat16_create(const char *path)
{
    /* 简化版：只支持根目录 */
    if (path[0] != '/' || path[1] == '\0') {
        return -1;
    }
    
    const char *filename = path + 1;
    
    /* 检查文件是否已存在 */
    fat16_dir_entry_t existing_entry;
    if (fat16_find_entry(path, &existing_entry, NULL) == 0) {
        return -1;  /* 文件已存在 */
    }
    
    /* 查找空目录条目 */
    int entry_index = fat16_find_empty_dir_entry(0);
    if (entry_index < 0) {
        return -1;  /* 目录已满 */
    }
    
    /* 分配一个簇 */
    uint16_t first_cluster = fat16_alloc_cluster();
    if (first_cluster == 0) {
        return -1;  /* 磁盘满 */
    }
    
    /* 创建目录条目 */
    fat16_dir_entry_t entry;
    memset(&entry, 0, sizeof(fat16_dir_entry_t));
    
    /* 设置文件名（8.3 格式） */
    fat16_make_83_name(filename, entry.name);
    
    /* 设置属性 */
    entry.attr = 0;  /* 普通文件 */
    
    /* 设置起始簇 */
    entry.first_cluster_lo = first_cluster;
    entry.first_cluster_hi = 0;
    
    /* 设置文件大小为 0 */
    entry.file_size = 0;
    
    /* 写入目录条目 */
    if (fat16_write_dir_entry(0, entry_index, &entry) < 0) {
        fat16_free_cluster_chain(first_cluster);
        return -1;
    }
    
    return 0;
}

/* ==========================================
 * VFS 接口：删除文件
 * ========================================== */
static int fat16_unlink(const char *path)
{
    fat16_dir_entry_t entry;
    uint32_t entry_index;
    
    /* 查找文件 */
    if (fat16_find_entry(path, &entry, &entry_index) < 0) {
        return -1;  /* 文件不存在 */
    }
    
    /* 检查是否是目录 */
    if (entry.attr & FAT_ATTR_DIRECTORY) {
        return -1;  /* 不能用 unlink 删除目录 */
    }
    
    /* 释放簇链 */
    if (entry.first_cluster_lo != 0) {
        fat16_free_cluster_chain(entry.first_cluster_lo);
    }
    
    /* 标记目录条目为已删除 */
    uint32_t sector = fat16_fsdata->root_dir_start + (entry_index * 32) / fat16_fsdata->bytes_per_sector;
    uint32_t offset = (entry_index * 32) % fat16_fsdata->bytes_per_sector;
    
    buffer_t *buf = block_get_buffer(fat16_fsdata->dev_id, sector);
    if (buf == NULL) {
        return -1;
    }
    
    buf->data[offset] = 0xE5;  /* 标记为已删除 */
    buf->flags |= BUF_DIRTY;
    block_release_buffer(buf);
    
    return 0;
}

/* ==========================================
 * VFS 接口：创建目录
 * ========================================== */
static int fat16_mkdir(const char *path)
{
    /* 简化版：暂时不实现 */
    (void)path;
    return -1;
}

/* ==========================================
 * VFS 接口：删除目录
 * ========================================== */
static int fat16_rmdir(const char *path)
{
    /* 简化版：暂时不实现 */
    (void)path;
    return -1;
}

/* ==========================================
 * VFS 接口：读取目录
 * ========================================== */
static int fat16_readdir(const char *path, vfs_dirent_t *dirents, int max_count)
{
    /* 简化版：只支持根目录 */
    if (strcmp(path, "/") != 0) {
        return -1;
    }
    
    int count = 0;
    
    /* 遍历根目录 */
    for (uint32_t i = 0; i < fat16_fsdata->root_entry_count && count < max_count; i++) {
        uint32_t sector = fat16_fsdata->root_dir_start + (i * 32) / fat16_fsdata->bytes_per_sector;
        uint32_t offset = (i * 32) % fat16_fsdata->bytes_per_sector;
        
        buffer_t *buf = block_get_buffer(fat16_fsdata->dev_id, sector);
        if (buf == NULL) {
            break;
        }
        
        fat16_dir_entry_t *dir_entry = (fat16_dir_entry_t *)(buf->data + offset);
        
        /* 检查是否是空条目 */
        if (dir_entry->name[0] == 0x00) {
            block_release_buffer(buf);
            break;  /* 目录结束 */
        }
        if (dir_entry->name[0] == 0xE5) {
            block_release_buffer(buf);
            continue;  /* 已删除的条目 */
        }
        
        /* 跳过卷标 */
        if (dir_entry->attr & FAT_ATTR_VOLUME_ID) {
            block_release_buffer(buf);
            continue;
        }
        
        /* 转换文件名 */
        char name[13];
        fat16_convert_name(dir_entry->name, name);
        
        /* 填充目录项 */
        strncpy(dirents[count].name, name, 63);
        dirents[count].name[63] = '\0';
        dirents[count].type = (dir_entry->attr & FAT_ATTR_DIRECTORY) ? VFS_DIR : VFS_FILE;
        
        count++;
        block_release_buffer(buf);
    }
    
    return count;
}

/* ==========================================
 * VFS 接口：获取文件状态
 * ========================================== */
static int fat16_stat(const char *path, vfs_stat_t *st)
{
    fat16_dir_entry_t entry;
    
    /* 查找文件 */
    if (fat16_find_entry(path, &entry, NULL) < 0) {
        return -1;
    }
    
    /* 填充 stat 结构 */
    char name[13];
    fat16_convert_name(entry.name, name);
    strncpy(st->name, name, 63);
    st->name[63] = '\0';
    
    st->type = (entry.attr & FAT_ATTR_DIRECTORY) ? VFS_DIR : VFS_FILE;
    st->size = entry.file_size;
    st->inode = entry.first_cluster_lo;  /* 用起始簇号作为 inode */
    
    return 0;
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
