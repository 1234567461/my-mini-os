/* ==========================================
 * FAT32 文件系统实现 - fat32.c
 * 功能：
 *   1. FAT32 文件系统完整支持
 *   2. VFS 接口实现
 * ========================================== */
#include "fat32.h"
#include "block.h"
#include "string.h"
#include "heap.h"
#include "vga.h"

/* 全局 FAT32 文件系统数据（简化版，只支持一个实例） */
static fat32_fsdata_t *fat32_fsdata = NULL;
static vfs_filesystem_t fat32_fs;
static vfs_ops_t fat32_ops;

/* ==========================================
 * 辅助函数：簇号转扇区号
 * ========================================== */
uint32_t fat32_cluster_to_sector(fat32_fsdata_t *fsdata, uint32_t cluster)
{
    /* 数据区从 2 号簇开始 */
    return fsdata->data_start + (cluster - 2) * fsdata->sectors_per_cluster;
}

/* ==========================================
 * 函数：fat32_read_boot_sector
 * 功能：读取引导扇区
 * ========================================== */
int fat32_read_boot_sector(uint32_t dev_id, fat32_boot_sector_t *bs)
{
    uint8_t *buf = block_read(dev_id, 0);
    if (buf == NULL) {
        return -1;
    }
    
    memcpy(bs, buf, sizeof(fat32_boot_sector_t));
    
    return 0;
}

/* ==========================================
 * 函数：fat32_calculate_params
 * 功能：计算文件系统参数
 * ========================================== */
int fat32_calculate_params(fat32_boot_sector_t *bs, fat32_fsdata_t *fsdata)
{
    /* 基本参数 */
    fsdata->bytes_per_sector = bs->bytes_per_sector;
    fsdata->sectors_per_cluster = bs->sectors_per_cluster;
    fsdata->fat_count = bs->fat_count;
    fsdata->sectors_per_fat = bs->sectors_per_fat_32;
    fsdata->root_cluster = bs->root_cluster;
    fsdata->fsinfo_sector = bs->fs_info_sector;
    
    /* 计算各部分起始扇区 */
    fsdata->fat_start = bs->reserved_sectors;
    
    /* 数据区起始扇区 */
    fsdata->data_start = fsdata->fat_start + bs->fat_count * bs->sectors_per_fat_32;
    
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
 * 函数：fat32_read_fsinfo
 * 功能：读取 FSInfo 扇区
 * ========================================== */
int fat32_read_fsinfo(uint32_t dev_id, uint32_t fsinfo_sector, fat32_fsinfo_t *fsinfo)
{
    uint8_t *buf = block_read(dev_id, fsinfo_sector);
    if (buf == NULL) {
        return -1;
    }
    
    memcpy(fsinfo, buf, sizeof(fat32_fsinfo_t));
    
    /* 验证签名 */
    if (fsinfo->lead_signature != FAT32_FSINFO_LEAD_SIG ||
        fsinfo->struct_signature != FAT32_FSINFO_STRUCT_SIG ||
        fsinfo->trail_signature != FAT32_FSINFO_TRAIL_SIG) {
        return -1;
    }
    
    return 0;
}

/* ==========================================
 * 函数：fat32_read_fat_entry
 * 功能：读取 FAT 表中的一个条目
 * ========================================== */
uint32_t fat32_read_fat_entry(fat32_fsdata_t *fsdata, uint32_t cluster)
{
    uint32_t fat_offset = cluster * 4;  /* 每个 FAT 条目 4 字节 */
    uint32_t sector = fsdata->fat_start + fat_offset / fsdata->bytes_per_sector;
    uint32_t offset = fat_offset % fsdata->bytes_per_sector;
    
    buffer_t *buf = block_get_buffer(fsdata->dev_id, sector);
    if (buf == NULL) {
        return FAT32_CLUSTER_BAD;
    }
    
    uint32_t value = *(uint32_t *)(buf->data + offset) & FAT32_CLUSTER_MASK;
    block_release_buffer(buf);
    
    return value;
}

/* ==========================================
 * 函数：fat32_write_fat_entry
 * 功能：写入 FAT 表中的一个条目
 * ========================================== */
int fat32_write_fat_entry(fat32_fsdata_t *fsdata, uint32_t cluster, uint32_t value)
{
    value &= FAT32_CLUSTER_MASK;
    
    /* 写入所有 FAT 表副本 */
    for (uint8_t fat = 0; fat < fsdata->fat_count; fat++) {
        uint32_t fat_offset = cluster * 4;
        uint32_t fat_start = fsdata->fat_start + fat * fsdata->sectors_per_fat;
        uint32_t sector = fat_start + fat_offset / fsdata->bytes_per_sector;
        uint32_t offset = fat_offset % fsdata->bytes_per_sector;
        
        buffer_t *buf = block_get_buffer(fsdata->dev_id, sector);
        if (buf == NULL) {
            return -1;
        }
        
        *(uint32_t *)(buf->data + offset) = value;
        buf->flags |= BUF_DIRTY;
        block_release_buffer(buf);
    }
    
    return 0;
}

/* ==========================================
 * 函数：fat32_alloc_cluster
 * 功能：分配一个空闲簇
 * 返回：簇号，失败返回 0
 * ========================================== */
uint32_t fat32_alloc_cluster(fat32_fsdata_t *fsdata)
{
    uint32_t start_cluster = 2;
    
    /* 如果有下一个空闲簇的提示，从那里开始 */
    if (fsdata->next_free_cluster != 0xFFFFFFFF && fsdata->next_free_cluster >= 2) {
        start_cluster = fsdata->next_free_cluster;
    }
    
    /* 从起始位置开始查找 */
    for (uint32_t cluster = start_cluster; cluster < fsdata->total_clusters + 2; cluster++) {
        uint32_t entry = fat32_read_fat_entry(fsdata, cluster);
        if (entry == FAT32_CLUSTER_FREE) {
            /* 标记为已使用（文件结束） */
            fat32_write_fat_entry(fsdata, cluster, FAT32_CLUSTER_LAST);
            
            /* 更新下一个空闲簇提示 */
            fsdata->next_free_cluster = cluster + 1;
            
            /* 更新空闲簇计数 */
            if (fsdata->free_cluster_count != 0xFFFFFFFF) {
                fsdata->free_cluster_count--;
            }
            
            return cluster;
        }
    }
    
    /* 如果从提示位置没找到，从头再找一遍 */
    if (start_cluster != 2) {
        for (uint32_t cluster = 2; cluster < start_cluster; cluster++) {
            uint32_t entry = fat32_read_fat_entry(fsdata, cluster);
            if (entry == FAT32_CLUSTER_FREE) {
                fat32_write_fat_entry(fsdata, cluster, FAT32_CLUSTER_LAST);
                fsdata->next_free_cluster = cluster + 1;
                if (fsdata->free_cluster_count != 0xFFFFFFFF) {
                    fsdata->free_cluster_count--;
                }
                return cluster;
            }
        }
    }
    
    return 0;  /* 没有空闲簇 */
}

/* ==========================================
 * 函数：fat32_free_cluster_chain
 * 功能：释放整个簇链
 * ========================================== */
void fat32_free_cluster_chain(fat32_fsdata_t *fsdata, uint32_t first_cluster)
{
    uint32_t current = first_cluster;
    
    while (current != 0 && current != FAT32_CLUSTER_LAST && current < fsdata->total_clusters + 2) {
        uint32_t next = fat32_read_fat_entry(fsdata, current);
        
        /* 释放当前簇 */
        fat32_write_fat_entry(fsdata, current, FAT32_CLUSTER_FREE);
        
        /* 更新空闲簇计数 */
        if (fsdata->free_cluster_count != 0xFFFFFFFF) {
            fsdata->free_cluster_count++;
        }
        
        if (next == FAT32_CLUSTER_LAST || next == 0) {
            break;
        }
        
        current = next;
    }
}

/* ==========================================
 * 辅助函数：将 8.3 格式转换为普通文件名
 * ========================================== */
static void fat32_convert_name(uint8_t *name83, char *name)
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
 * 辅助函数：将普通文件名转换为 8.3 格式
 * ========================================== */
static void fat32_make_83_name(const char *name, uint8_t *name83)
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
 * 辅助函数：读取目录中的条目
 * 参数：dir_cluster - 目录起始簇（0 表示根目录）
 * ========================================== */
static int fat32_read_dir_entry(uint32_t dir_cluster, uint32_t index, fat32_dir_entry_t *entry)
{
    uint32_t entries_per_cluster = (fat32_fsdata->sectors_per_cluster * fat32_fsdata->bytes_per_sector) / 32;
    uint32_t cluster_index = index / entries_per_cluster;
    uint32_t entry_in_cluster = index % entries_per_cluster;
    
    /* 找到对应的簇 */
    uint32_t current_cluster = dir_cluster;
    for (uint32_t i = 0; i < cluster_index; i++) {
        current_cluster = fat32_read_fat_entry(fat32_fsdata, current_cluster);
        if (current_cluster == FAT32_CLUSTER_LAST || current_cluster == FAT32_CLUSTER_BAD) {
            return -1;
        }
    }
    
    /* 计算扇区和偏移 */
    uint32_t start_sector = fat32_cluster_to_sector(fat32_fsdata, current_cluster);
    uint32_t sector = start_sector + (entry_in_cluster * 32) / fat32_fsdata->bytes_per_sector;
    uint32_t offset = (entry_in_cluster * 32) % fat32_fsdata->bytes_per_sector;
    
    buffer_t *buf = block_get_buffer(fat32_fsdata->dev_id, sector);
    if (buf == NULL) {
        return -1;
    }
    
    memcpy(entry, buf->data + offset, sizeof(fat32_dir_entry_t));
    block_release_buffer(buf);
    
    return 0;
}

/* ==========================================
 * 辅助函数：写入目录条目
 * ========================================== */
static int fat32_write_dir_entry(uint32_t dir_cluster, uint32_t index, fat32_dir_entry_t *entry)
{
    uint32_t entries_per_cluster = (fat32_fsdata->sectors_per_cluster * fat32_fsdata->bytes_per_sector) / 32;
    uint32_t cluster_index = index / entries_per_cluster;
    uint32_t entry_in_cluster = index % entries_per_cluster;
    
    /* 找到对应的簇，需要时分配新簇 */
    uint32_t current_cluster = dir_cluster;
    uint32_t prev_cluster = 0;
    
    for (uint32_t i = 0; i < cluster_index; i++) {
        prev_cluster = current_cluster;
        current_cluster = fat32_read_fat_entry(fat32_fsdata, current_cluster);
        
        if (current_cluster == FAT32_CLUSTER_LAST || current_cluster == FAT32_CLUSTER_BAD) {
            /* 需要分配新簇 */
            uint32_t new_cluster = fat32_alloc_cluster(fat32_fsdata);
            if (new_cluster == 0) {
                return -1;
            }
            
            /* 链接到簇链 */
            fat32_write_fat_entry(fat32_fsdata, prev_cluster, new_cluster);
            fat32_write_fat_entry(fat32_fsdata, new_cluster, FAT32_CLUSTER_LAST);
            
            /* 清空新簇 */
            uint32_t new_sector = fat32_cluster_to_sector(fat32_fsdata, new_cluster);
            for (uint32_t s = 0; s < fat32_fsdata->sectors_per_cluster; s++) {
                buffer_t *buf = block_get_buffer(fat32_fsdata->dev_id, new_sector + s);
                if (buf != NULL) {
                    memset(buf->data, 0, fat32_fsdata->bytes_per_sector);
                    buf->flags |= BUF_DIRTY;
                    block_release_buffer(buf);
                }
            }
            
            current_cluster = new_cluster;
        }
    }
    
    /* 计算扇区和偏移 */
    uint32_t start_sector = fat32_cluster_to_sector(fat32_fsdata, current_cluster);
    uint32_t sector = start_sector + (entry_in_cluster * 32) / fat32_fsdata->bytes_per_sector;
    uint32_t offset = (entry_in_cluster * 32) % fat32_fsdata->bytes_per_sector;
    
    buffer_t *buf = block_get_buffer(fat32_fsdata->dev_id, sector);
    if (buf == NULL) {
        return -1;
    }
    
    memcpy(buf->data + offset, entry, sizeof(fat32_dir_entry_t));
    buf->flags |= BUF_DIRTY;
    block_release_buffer(buf);
    
    return 0;
}

/* ==========================================
 * 辅助函数：在目录中查找空条目
 * ========================================== */
static int fat32_find_empty_dir_entry(uint32_t dir_cluster)
{
    uint32_t index = 0;
    fat32_dir_entry_t entry;
    
    while (1) {
        if (fat32_read_dir_entry(dir_cluster, index, &entry) < 0) {
            /* 超出范围，返回当前索引作为新条目位置 */
            return index;
        }
        
        /* 检查是否是空条目或已删除条目 */
        if (entry.name[0] == 0x00 || entry.name[0] == 0xE5) {
            return index;
        }
        
        index++;
    }
}

/* ==========================================
 * 辅助函数：在目录中查找文件
 * ========================================== */
static int fat32_find_entry(const char *path, fat32_dir_entry_t *entry, uint32_t *entry_index, uint32_t *dir_cluster)
{
    /* 简化版：只支持根目录下的文件 */
    if (path[0] != '/' || path[1] == '\0') {
        return -1;
    }
    
    /* 跳过开头的 / */
    const char *filename = path + 1;
    
    /* 根目录簇 */
    uint32_t root_dir = fat32_fsdata->root_cluster;
    if (dir_cluster != NULL) {
        *dir_cluster = root_dir;
    }
    
    /* 遍历根目录 */
    uint32_t index = 0;
    fat32_dir_entry_t dir_entry;
    
    while (1) {
        if (fat32_read_dir_entry(root_dir, index, &dir_entry) < 0) {
            break;
        }
        
        /* 检查是否是空条目 */
        if (dir_entry.name[0] == 0x00) {
            break;  /* 目录结束 */
        }
        if (dir_entry.name[0] == 0xE5) {
            index++;
            continue;  /* 已删除的条目 */
        }
        
        /* 跳过卷标和长文件名 */
        if (dir_entry.attr & FAT32_ATTR_VOLUME_ID) {
            index++;
            continue;
        }
        if ((dir_entry.attr & FAT32_ATTR_LONG_NAME) == FAT32_ATTR_LONG_NAME) {
            index++;
            continue;
        }
        
        /* 转换文件名并比较 */
        char name[13];
        fat32_convert_name(dir_entry.name, name);
        
        if (strcmp(name, filename) == 0) {
            memcpy(entry, &dir_entry, sizeof(fat32_dir_entry_t));
            if (entry_index != NULL) {
                *entry_index = index;
            }
            return 0;
        }
        
        index++;
    }
    
    return -1;  /* 未找到 */
}

/* ==========================================
 * VFS 接口：打开文件
 * ========================================== */
static vfs_file_t *fat32_open(const char *path, int flags)
{
    (void)flags;
    
    fat32_dir_entry_t entry;
    if (fat32_find_entry(path, &entry, NULL, NULL) < 0) {
        return NULL;
    }
    
    /* 分配文件描述符 */
    vfs_file_t *file = (vfs_file_t *)kmalloc(sizeof(vfs_file_t));
    if (file == NULL) {
        return NULL;
    }
    
    /* 初始化文件信息 */
    memset(file, 0, sizeof(vfs_file_t));
    file->type = (entry.attr & FAT32_ATTR_DIRECTORY) ? VFS_DIR : VFS_FILE;
    file->size = entry.file_size;
    file->pos = 0;
    
    /* 保存起始簇号作为 fs_data */
    uint32_t *first_cluster = (uint32_t *)kmalloc(sizeof(uint32_t));
    *first_cluster = ((uint32_t)entry.first_cluster_hi << 16) | entry.first_cluster_lo;
    file->fs_data = first_cluster;
    
    return file;
}

/* ==========================================
 * VFS 接口：关闭文件
 * ========================================== */
static int fat32_close(vfs_file_t *file)
{
    if (file == NULL) {
        return -1;
    }
    
    if (file->fs_data != NULL) {
        kfree(file->fs_data);
    }
    
    kfree(file);
    
    return 0;
}

/* ==========================================
 * VFS 接口：读取文件
 * ========================================== */
static int fat32_read(vfs_file_t *file, void *buf, uint32_t count)
{
    if (file == NULL || buf == NULL || file->type != VFS_FILE) {
        return -1;
    }
    
    uint32_t first_cluster = *(uint32_t *)file->fs_data;
    uint32_t bytes_per_cluster = fat32_fsdata->sectors_per_cluster * fat32_fsdata->bytes_per_sector;
    
    /* 计算剩余可读字节数 */
    uint32_t remaining = file->size - file->pos;
    if (count > remaining) {
        count = remaining;
    }
    
    if (count == 0) {
        return 0;
    }
    
    uint8_t *dest = (uint8_t *)buf;
    uint32_t bytes_read = 0;
    
    /* 计算起始簇和偏移 */
    uint32_t start_cluster_index = file->pos / bytes_per_cluster;
    uint32_t offset_in_cluster = file->pos % bytes_per_cluster;
    
    /* 找到起始簇 */
    uint32_t current_cluster = first_cluster;
    for (uint32_t i = 0; i < start_cluster_index; i++) {
        current_cluster = fat32_read_fat_entry(fat32_fsdata, current_cluster);
        if (current_cluster == FAT32_CLUSTER_LAST || current_cluster == FAT32_CLUSTER_BAD) {
            return bytes_read;
        }
    }
    
    /* 读取数据 */
    while (bytes_read < count) {
        uint32_t cluster_start = fat32_cluster_to_sector(fat32_fsdata, current_cluster);
        uint32_t bytes_in_cluster = bytes_per_cluster - offset_in_cluster;
        uint32_t to_read = (count - bytes_read) < bytes_in_cluster ? (count - bytes_read) : bytes_in_cluster;
        
        /* 从当前簇读取 */
        uint32_t start_sector = cluster_start + offset_in_cluster / fat32_fsdata->bytes_per_sector;
        uint32_t start_offset = offset_in_cluster % fat32_fsdata->bytes_per_sector;
        uint32_t sectors_to_read = (to_read + start_offset + fat32_fsdata->bytes_per_sector - 1) / fat32_fsdata->bytes_per_sector;
        
        for (uint32_t s = 0; s < sectors_to_read && bytes_read < count; s++) {
            buffer_t *b = block_get_buffer(fat32_fsdata->dev_id, start_sector + s);
            if (b == NULL) {
                break;
            }
            
            uint32_t copy_offset = (s == 0) ? start_offset : 0;
            uint32_t copy_size = fat32_fsdata->bytes_per_sector - copy_offset;
            if (copy_size > count - bytes_read) {
                copy_size = count - bytes_read;
            }
            
            memcpy(dest + bytes_read, b->data + copy_offset, copy_size);
            bytes_read += copy_size;
            block_release_buffer(b);
        }
        
        offset_in_cluster = 0;
        
        /* 下一个簇 */
        current_cluster = fat32_read_fat_entry(fat32_fsdata, current_cluster);
        if (current_cluster == FAT32_CLUSTER_LAST || current_cluster == FAT32_CLUSTER_BAD) {
            break;
        }
    }
    
    file->pos += bytes_read;
    return bytes_read;
}

/* ==========================================
 * 辅助函数：扩展文件簇链
 * 参数：
 *   first_cluster - 起始簇号
 *   new_clusters - 需要新增的簇数
 * 返回：新的起始簇号（如果文件为空），或0表示失败
 * ========================================== */
static uint32_t fat32_extend_chain(uint32_t first_cluster, uint32_t new_clusters)
{
    if (new_clusters == 0) {
        return first_cluster;
    }
    
    uint32_t current_cluster = first_cluster;
    
    /* 如果文件为空，分配第一个簇 */
    if (current_cluster == 0) {
        current_cluster = fat32_alloc_cluster(fat32_fsdata);
        if (current_cluster == 0) {
            return 0;
        }
        first_cluster = current_cluster;
        new_clusters--;
    }
    
    /* 找到链表的最后一个簇 */
    while (1) {
        uint32_t next = fat32_read_fat_entry(fat32_fsdata, current_cluster);
        if (next == FAT32_CLUSTER_LAST || next == 0) {
            break;
        }
        current_cluster = next;
    }
    
    /* 追加新簇 */
    for (uint32_t i = 0; i < new_clusters; i++) {
        uint32_t new_cluster = fat32_alloc_cluster(fat32_fsdata);
        if (new_cluster == 0) {
            /* 分配失败，已经分配的不释放（简化处理） */
            return 0;
        }
        
        /* 链接到链表末尾 */
        fat32_write_fat_entry(fat32_fsdata, current_cluster, new_cluster);
        fat32_write_fat_entry(fat32_fsdata, new_cluster, FAT32_CLUSTER_LAST);
        current_cluster = new_cluster;
    }
    
    return first_cluster;
}

/* ==========================================
 * 辅助函数：更新目录条目（更新文件大小和时间）
 * ========================================== */
static int fat32_update_dir_entry(const char *path, uint32_t new_size, uint32_t new_cluster)
{
    fat32_dir_entry_t entry;
    uint32_t entry_index;
    uint32_t dir_cluster;
    
    if (fat32_find_entry(path, &entry, &entry_index, &dir_cluster) < 0) {
        return -1;
    }
    
    /* 更新文件大小 */
    entry.file_size = new_size;
    
    /* 更新起始簇号 */
    entry.first_cluster_lo = (uint16_t)(new_cluster & 0xFFFF);
    entry.first_cluster_hi = (uint16_t)((new_cluster >> 16) & 0xFFFF);
    
    /* 更新修改时间和日期（简化：使用固定值） */
    entry.modify_time = 0;
    entry.modify_date = 0;
    
    /* 写回目录条目 */
    return fat32_write_dir_entry(dir_cluster, entry_index, &entry);
}

/* ==========================================
 * VFS 接口：写入文件
 * ========================================== */
static int fat32_write(vfs_file_t *file, const void *buf, uint32_t count)
{
    if (file == NULL || buf == NULL || file->type != VFS_FILE) {
        return -1;
    }
    
    uint32_t first_cluster = *(uint32_t *)file->fs_data;
    uint32_t bytes_per_cluster = fat32_fsdata->sectors_per_cluster * fat32_fsdata->bytes_per_sector;
    
    /* 计算写入后的文件大小 */
    uint32_t new_size = file->pos + count;
    
    /* 计算需要的总簇数 */
    uint32_t clusters_needed = (new_size + bytes_per_cluster - 1) / bytes_per_cluster;
    uint32_t current_clusters = (file->size + bytes_per_cluster - 1) / bytes_per_cluster;
    
    /* 如果需要更多簇，扩展文件 */
    if (clusters_needed > current_clusters) {
        uint32_t new_clusters = clusters_needed - current_clusters;
        uint32_t new_first = fat32_extend_chain(first_cluster, new_clusters);
        if (new_first == 0 && first_cluster == 0) {
            return -1;  /* 空间不足 */
        }
        if (new_first != 0) {
            first_cluster = new_first;
            *(uint32_t *)file->fs_data = first_cluster;
        }
    }
    
    const uint8_t *src = (const uint8_t *)buf;
    uint32_t bytes_written = 0;
    
    /* 计算起始簇和偏移 */
    uint32_t start_cluster_index = file->pos / bytes_per_cluster;
    uint32_t offset_in_cluster = file->pos % bytes_per_cluster;
    
    /* 找到起始簇 */
    uint32_t current_cluster = first_cluster;
    for (uint32_t i = 0; i < start_cluster_index; i++) {
        current_cluster = fat32_read_fat_entry(fat32_fsdata, current_cluster);
        if (current_cluster == FAT32_CLUSTER_LAST || current_cluster == FAT32_CLUSTER_BAD) {
            return bytes_written;
        }
    }
    
    /* 写入数据 */
    while (bytes_written < count) {
        uint32_t cluster_start = fat32_cluster_to_sector(fat32_fsdata, current_cluster);
        uint32_t bytes_in_cluster = bytes_per_cluster - offset_in_cluster;
        uint32_t to_write = (count - bytes_written) < bytes_in_cluster ? (count - bytes_written) : bytes_in_cluster;
        
        /* 写入当前簇 */
        uint32_t start_sector = cluster_start + offset_in_cluster / fat32_fsdata->bytes_per_sector;
        uint32_t start_offset = offset_in_cluster % fat32_fsdata->bytes_per_sector;
        uint32_t sectors_to_write = (to_write + start_offset + fat32_fsdata->bytes_per_sector - 1) / fat32_fsdata->bytes_per_sector;
        
        for (uint32_t s = 0; s < sectors_to_write && bytes_written < count; s++) {
            buffer_t *b = block_get_buffer(fat32_fsdata->dev_id, start_sector + s);
            if (b == NULL) {
                break;
            }
            
            uint32_t copy_offset = (s == 0) ? start_offset : 0;
            uint32_t copy_size = fat32_fsdata->bytes_per_sector - copy_offset;
            if (copy_size > count - bytes_written) {
                copy_size = count - bytes_written;
            }
            
            memcpy(b->data + copy_offset, src + bytes_written, copy_size);
            b->flags |= BUF_DIRTY;
            bytes_written += copy_size;
            block_release_buffer(b);
        }
        
        offset_in_cluster = 0;
        
        /* 下一个簇 */
        current_cluster = fat32_read_fat_entry(fat32_fsdata, current_cluster);
        if (current_cluster == FAT32_CLUSTER_LAST || current_cluster == FAT32_CLUSTER_BAD) {
            break;
        }
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
static int fat32_create(const char *path)
{
    /* 简化版：只支持根目录下的文件 */
    if (path[0] != '/' || path[1] == '\0') {
        return -1;
    }
    
    const char *filename = path + 1;
    
    /* 检查文件是否已存在 */
    fat32_dir_entry_t existing;
    if (fat32_find_entry(path, &existing, NULL, NULL) == 0) {
        return -1;  /* 文件已存在 */
    }
    
    /* 转换为 8.3 格式 */
    uint8_t name83[11];
    fat32_make_83_name(filename, name83);
    
    /* 在根目录中找一个空条目 */
    uint32_t root_dir = fat32_fsdata->root_cluster;
    int index = fat32_find_empty_dir_entry(root_dir);
    if (index < 0) {
        return -1;  /* 目录已满 */
    }
    
    /* 创建目录条目 */
    fat32_dir_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    memcpy(entry.name, name83, 11);
    entry.attr = FAT32_ATTR_ARCHIVE;
    entry.file_size = 0;
    entry.first_cluster_lo = 0;
    entry.first_cluster_hi = 0;
    entry.create_time = 0;
    entry.create_date = 0;
    entry.modify_time = 0;
    entry.modify_date = 0;
    
    /* 写入目录条目 */
    if (fat32_write_dir_entry(root_dir, index, &entry) < 0) {
        return -1;
    }
    
    return 0;
}

/* ==========================================
 * VFS 接口：删除文件
 * ========================================== */
static int fat32_unlink(const char *path)
{
    fat32_dir_entry_t entry;
    uint32_t entry_index;
    uint32_t dir_cluster;
    
    if (fat32_find_entry(path, &entry, &entry_index, &dir_cluster) < 0) {
        return -1;  /* 文件不存在 */
    }
    
    /* 不能删除目录 */
    if (entry.attr & FAT32_ATTR_DIRECTORY) {
        return -1;
    }
    
    /* 释放文件数据簇 */
    uint32_t first_cluster = ((uint32_t)entry.first_cluster_hi << 16) | entry.first_cluster_lo;
    if (first_cluster != 0) {
        fat32_free_cluster_chain(fat32_fsdata, first_cluster);
    }
    
    /* 标记目录条目为已删除（第一个字节设为 0xE5） */
    entry.name[0] = 0xE5;
    fat32_write_dir_entry(dir_cluster, entry_index, &entry);
    
    return 0;
}

/* ==========================================
 * VFS 接口：创建目录
 * ========================================== */
static int fat32_mkdir(const char *path)
{
    /* 简化版：只支持根目录下创建子目录 */
    if (path[0] != '/' || path[1] == '\0') {
        return -1;
    }
    
    const char *dirname = path + 1;
    
    /* 检查是否已存在 */
    fat32_dir_entry_t existing;
    if (fat32_find_entry(path, &existing, NULL, NULL) == 0) {
        return -1;  /* 已存在 */
    }
    
    /* 转换为 8.3 格式 */
    uint8_t name83[11];
    fat32_make_83_name(dirname, name83);
    
    /* 分配一个簇给新目录 */
    uint32_t dir_cluster = fat32_alloc_cluster(fat32_fsdata);
    if (dir_cluster == 0) {
        return -1;  /* 空间不足 */
    }
    
    /* 清空新目录簇 */
    uint32_t start_sector = fat32_cluster_to_sector(fat32_fsdata, dir_cluster);
    for (uint32_t s = 0; s < fat32_fsdata->sectors_per_cluster; s++) {
        buffer_t *buf = block_get_buffer(fat32_fsdata->dev_id, start_sector + s);
        if (buf != NULL) {
            memset(buf->data, 0, fat32_fsdata->bytes_per_sector);
            buf->flags |= BUF_DIRTY;
            block_release_buffer(buf);
        }
    }
    
    /* 在根目录中找一个空条目 */
    uint32_t root_dir = fat32_fsdata->root_cluster;
    int index = fat32_find_empty_dir_entry(root_dir);
    if (index < 0) {
        fat32_free_cluster_chain(fat32_fsdata, dir_cluster);
        return -1;
    }
    
    /* 创建目录条目 */
    fat32_dir_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    memcpy(entry.name, name83, 11);
    entry.attr = FAT32_ATTR_DIRECTORY;
    entry.file_size = 0;
    entry.first_cluster_lo = (uint16_t)(dir_cluster & 0xFFFF);
    entry.first_cluster_hi = (uint16_t)((dir_cluster >> 16) & 0xFFFF);
    entry.create_time = 0;
    entry.create_date = 0;
    entry.modify_time = 0;
    entry.modify_date = 0;
    
    /* 写入目录条目 */
    if (fat32_write_dir_entry(root_dir, index, &entry) < 0) {
        fat32_free_cluster_chain(fat32_fsdata, dir_cluster);
        return -1;
    }
    
    return 0;
}

/* ==========================================
 * VFS 接口：删除目录
 * ========================================== */
static int fat32_rmdir(const char *path)
{
    fat32_dir_entry_t entry;
    uint32_t entry_index;
    uint32_t dir_cluster;
    
    if (fat32_find_entry(path, &entry, &entry_index, &dir_cluster) < 0) {
        return -1;  /* 目录不存在 */
    }
    
    /* 必须是目录 */
    if (!(entry.attr & FAT32_ATTR_DIRECTORY)) {
        return -1;
    }
    
    /* 检查目录是否为空（简化：只检查第一个条目） */
    uint32_t subdir_cluster = ((uint32_t)entry.first_cluster_hi << 16) | entry.first_cluster_lo;
    fat32_dir_entry_t first_entry;
    if (fat32_read_dir_entry(subdir_cluster, 0, &first_entry) == 0) {
        if (first_entry.name[0] != 0x00 && first_entry.name[0] != 0xE5) {
            /* 检查是否只有 . 和 .. 条目 */
            bool has_dot = false;
            bool has_dotdot = false;
            uint32_t idx = 0;
            
            while (1) {
                fat32_dir_entry_t e;
                if (fat32_read_dir_entry(subdir_cluster, idx, &e) < 0) {
                    break;
                }
                if (e.name[0] == 0x00) {
                    break;
                }
                if (e.name[0] == 0xE5) {
                    idx++;
                    continue;
                }
                
                /* 检查是否是 . 或 .. */
                if (e.name[0] == '.' && e.name[1] == ' ' && e.name[2] == ' ') {
                    has_dot = true;
                } else if (e.name[0] == '.' && e.name[1] == '.' && e.name[2] == ' ') {
                    has_dotdot = true;
                } else {
                    /* 有其他文件，目录不为空 */
                    return -1;
                }
                
                idx++;
            }
        }
    }
    
    /* 释放目录簇 */
    if (subdir_cluster != 0) {
        fat32_free_cluster_chain(fat32_fsdata, subdir_cluster);
    }
    
    /* 标记目录条目为已删除 */
    entry.name[0] = 0xE5;
    fat32_write_dir_entry(dir_cluster, entry_index, &entry);
    
    return 0;
}

/* ==========================================
 * VFS 接口：读取目录
 * ========================================== */
static int fat32_readdir(const char *path, vfs_dirent_t *dirents, int max_count)
{
    if (dirents == NULL || max_count <= 0) {
        return -1;
    }
    
    /* 只支持根目录 */
    if (strcmp(path, "/") != 0 && strcmp(path, "") != 0) {
        return -1;
    }
    
    uint32_t root_dir = fat32_fsdata->root_cluster;
    int count = 0;
    uint32_t index = 0;
    fat32_dir_entry_t entry;
    
    while (count < max_count) {
        if (fat32_read_dir_entry(root_dir, index, &entry) < 0) {
            break;
        }
        
        /* 检查是否是空条目 */
        if (entry.name[0] == 0x00) {
            break;  /* 目录结束 */
        }
        if (entry.name[0] == 0xE5) {
            index++;
            continue;  /* 已删除的条目 */
        }
        
        /* 跳过卷标和长文件名 */
        if (entry.attr & FAT32_ATTR_VOLUME_ID) {
            index++;
            continue;
        }
        if ((entry.attr & FAT32_ATTR_LONG_NAME) == FAT32_ATTR_LONG_NAME) {
            index++;
            continue;
        }
        
        /* 转换文件名 */
        char name[13];
        fat32_convert_name(entry.name, name);
        
        /* 填充目录项 */
        strncpy(dirents[count].name, name, 63);
        dirents[count].name[63] = '\0';
        dirents[count].type = (entry.attr & FAT32_ATTR_DIRECTORY) ? VFS_DIR : VFS_FILE;
        
        count++;
        index++;
    }
    
    return count;
}

/* ==========================================
 * VFS 接口：获取文件属性
 * ========================================== */
static int fat32_stat(const char *path, vfs_stat_t *buf)
{
    if (buf == NULL) {
        return -1;
    }
    
    fat32_dir_entry_t entry;
    if (fat32_find_entry(path, &entry, NULL, NULL) < 0) {
        return -1;
    }
    
    /* 转换文件名 */
    char name[13];
    fat32_convert_name(entry.name, name);
    
    strncpy(buf->name, name, 63);
    buf->name[63] = '\0';
    buf->type = (entry.attr & FAT32_ATTR_DIRECTORY) ? VFS_DIR : VFS_FILE;
    buf->size = entry.file_size;
    buf->inode = ((uint32_t)entry.first_cluster_hi << 16) | entry.first_cluster_lo;
    
    return 0;
}

/* ==========================================
 * 函数：fat32_init
 * 功能：初始化 FAT32 文件系统
 * ========================================== */
vfs_filesystem_t *fat32_init(uint32_t dev_id)
{
    fat32_boot_sector_t bs;
    
    /* 读取引导扇区 */
    if (fat32_read_boot_sector(dev_id, &bs) < 0) {
        return NULL;
    }
    
    /* 检查是否是 FAT32（通过 fs_type 字段） */
    if (strncmp((char *)bs.fs_type, "FAT32", 5) != 0) {
        return NULL;
    }
    
    /* 分配文件系统数据 */
    fat32_fsdata = (fat32_fsdata_t *)kmalloc(sizeof(fat32_fsdata_t));
    if (fat32_fsdata == NULL) {
        return NULL;
    }
    
    memset(fat32_fsdata, 0, sizeof(fat32_fsdata_t));
    fat32_fsdata->dev_id = dev_id;
    
    /* 计算参数 */
    if (fat32_calculate_params(&bs, fat32_fsdata) < 0) {
        kfree(fat32_fsdata);
        fat32_fsdata = NULL;
        return NULL;
    }
    
    /* 读取 FSInfo */
    fat32_fsinfo_t fsinfo;
    if (fat32_read_fsinfo(dev_id, fat32_fsdata->fsinfo_sector, &fsinfo) == 0) {
        fat32_fsdata->free_cluster_count = fsinfo.free_cluster_count;
        fat32_fsdata->next_free_cluster = fsinfo.next_free_cluster;
    } else {
        fat32_fsdata->free_cluster_count = 0xFFFFFFFF;
        fat32_fsdata->next_free_cluster = 0xFFFFFFFF;
    }
    
    /* 设置 VFS 操作函数表 */
    memset(&fat32_ops, 0, sizeof(vfs_ops_t));
    fat32_ops.open = fat32_open;
    fat32_ops.close = fat32_close;
    fat32_ops.read = fat32_read;
    fat32_ops.write = fat32_write;
    fat32_ops.create = fat32_create;
    fat32_ops.unlink = fat32_unlink;
    fat32_ops.mkdir = fat32_mkdir;
    fat32_ops.rmdir = fat32_rmdir;
    fat32_ops.readdir = fat32_readdir;
    fat32_ops.stat = fat32_stat;
    
    /* 初始化文件系统结构体 */
    memset(&fat32_fs, 0, sizeof(vfs_filesystem_t));
    strncpy(fat32_fs.name, "fat32", 31);
    fat32_fs.ops = &fat32_ops;
    fat32_fs.data = fat32_fsdata;
    
    return &fat32_fs;
}
