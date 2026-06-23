/* ==========================================
 * 文件系统 - fs.h
 * 功能：
 *   1. 内存文件系统（ramfs）
 *   2. 文件操作接口
 * ========================================== */

#ifndef FS_H
#define FS_H

#include "types.h"

/* ==========================================
 * 常量定义
 * ========================================== */
#define MAX_FILES        32     /* 最大文件数 */
#define MAX_FILENAME     32     /* 最大文件名长度 */
#define MAX_FILE_SIZE    4096   /* 单个文件最大大小 */
#define FS_MAGIC         0xDEAD /* 文件系统魔数 */

/* ==========================================
 * 文件类型
 * ========================================== */
#define FILE_TYPE_REGULAR  0  /* 普通文件 */
#define FILE_TYPE_DIR      1  /* 目录 */

/* ==========================================
 * 文件描述符标志
 * ========================================== */
#define O_RDONLY    0x0001  /* 只读 */
#define O_WRONLY    0x0002  /* 只写 */
#define O_RDWR      0x0003  /* 读写 */
#define O_CREAT     0x0100  /* 创建 */
#define O_APPEND    0x0200  /* 追加 */

/* ==========================================
 * 文件inode结构
 * ========================================== */
typedef struct {
    char name[MAX_FILENAME];    /* 文件名 */
    uint32_t type;              /* 文件类型 */
    uint32_t size;              /* 文件大小 */
    uint32_t magic;             /* 魔数（验证用） */
    uint8_t data[MAX_FILE_SIZE];/* 文件数据 */
} file_inode_t;

/* ==========================================
 * 文件描述符结构
 * ========================================== */
typedef struct {
    int inode_index;    /* inode索引 */
    uint32_t offset;    /* 当前读写位置 */
    uint32_t flags;     /* 打开标志 */
    int used;           /* 是否被使用 */
} file_descriptor_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化文件系统 */
void fs_init();

/* 打开文件 */
int fs_open(const char *path, int flags);

/* 关闭文件 */
int fs_close(int fd);

/* 读文件 */
int fs_read(int fd, char *buf, int count);

/* 写文件 */
int fs_write(int fd, const char *buf, int count);

/* 移动文件指针 */
int fs_lseek(int fd, int offset, int whence);

/* 创建文件 */
int fs_creat(const char *path);

/* 删除文件 */
int fs_unlink(const char *path);

/* 获取文件大小 */
int fs_filesize(int fd);

/* 列出目录 */
int fs_listdir(const char *path, char *buf, int bufsize);

/* 检查文件是否存在 */
int fs_exists(const char *path);

#endif /* FS_H */
