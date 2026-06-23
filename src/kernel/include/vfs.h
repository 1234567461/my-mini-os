/* ==========================================
 * 虚拟文件系统（VFS）- vfs.h v0.5.0
 * 功能：
 *   1. 文件系统抽象层
 *   2. 统一的文件操作接口
 *   3. 支持多种文件系统类型
 * ========================================== */

#ifndef VFS_H
#define VFS_H

#include "types.h"

/* ==========================================
 * 文件类型
 * ========================================== */
typedef enum {
    VFS_FILE = 0,      /* 普通文件 */
    VFS_DIR = 1        /* 目录 */
} vfs_file_type_t;

/* ==========================================
 * 文件属性结构体
 * ========================================== */
typedef struct {
    char name[64];         /* 文件名 */
    vfs_file_type_t type;  /* 文件类型 */
    uint32_t size;         /* 文件大小（字节） */
    uint32_t inode;        /* inode 编号 */
} vfs_stat_t;

/* ==========================================
 * 文件描述符结构体
 * ========================================== */
typedef struct vfs_file {
    char name[64];         /* 文件名 */
    vfs_file_type_t type;  /* 文件类型 */
    uint32_t size;         /* 文件大小 */
    uint32_t pos;          /* 当前读写位置 */
    void *fs_data;         /* 文件系统私有数据 */
    struct vfs_file *next; /* 链表指针 */
} vfs_file_t;

/* ==========================================
 * 目录项结构体
 * ========================================== */
typedef struct {
    char name[64];         /* 文件名 */
    vfs_file_type_t type;  /* 文件类型 */
} vfs_dirent_t;

/* ==========================================
 * 文件系统操作函数表
 * ========================================== */
typedef struct vfs_operations {
    /* 打开文件 */
    vfs_file_t *(*open)(const char *path, int flags);
    
    /* 关闭文件 */
    int (*close)(vfs_file_t *file);
    
    /* 读取文件 */
    int (*read)(vfs_file_t *file, void *buf, uint32_t count);
    
    /* 写入文件 */
    int (*write)(vfs_file_t *file, const void *buf, uint32_t count);
    
    /* 创建文件 */
    int (*create)(const char *path);
    
    /* 删除文件 */
    int (*unlink)(const char *path);
    
    /* 创建目录 */
    int (*mkdir)(const char *path);
    
    /* 删除目录 */
    int (*rmdir)(const char *path);
    
    /* 读取目录 */
    int (*readdir)(const char *path, vfs_dirent_t *dirents, int max_count);
    
    /* 获取文件属性 */
    int (*stat)(const char *path, vfs_stat_t *buf);
} vfs_ops_t;

/* ==========================================
 * 文件系统结构体
 * ========================================== */
typedef struct vfs_filesystem {
    char name[32];         /* 文件系统名称 */
    vfs_ops_t *ops;        /* 操作函数表 */
    void *data;            /* 文件系统私有数据 */
    struct vfs_filesystem *next;  /* 链表指针 */
} vfs_filesystem_t;

/* ==========================================
 * 打开标志
 * ========================================== */
#define VFS_O_RDONLY   0x0001  /* 只读 */
#define VFS_O_WRONLY   0x0002  /* 只写 */
#define VFS_O_RDWR     0x0003  /* 读写 */
#define VFS_O_CREAT    0x0100  /* 创建文件 */
#define VFS_O_TRUNC    0x0200  /* 截断文件 */

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化 VFS */
void vfs_init();

/* 注册文件系统 */
int vfs_register_filesystem(vfs_filesystem_t *fs);

/* 挂载文件系统到根目录 */
int vfs_mount_root(vfs_filesystem_t *fs);

/* 获取当前工作目录 */
const char *vfs_getcwd();

/* 改变当前工作目录 */
int vfs_chdir(const char *path);

/* 打开文件 */
vfs_file_t *vfs_open(const char *path, int flags);

/* 关闭文件 */
int vfs_close(vfs_file_t *file);

/* 读取文件 */
int vfs_read(vfs_file_t *file, void *buf, uint32_t count);

/* 写入文件 */
int vfs_write(vfs_file_t *file, const void *buf, uint32_t count);

/* 创建文件 */
int vfs_create(const char *path);

/* 删除文件 */
int vfs_unlink(const char *path);

/* 创建目录 */
int vfs_mkdir(const char *path);

/* 删除目录 */
int vfs_rmdir(const char *path);

/* 读取目录 */
int vfs_readdir(const char *path, vfs_dirent_t *dirents, int max_count);

/* 获取文件属性 */
int vfs_stat(const char *path, vfs_stat_t *buf);

#endif /* VFS_H */
