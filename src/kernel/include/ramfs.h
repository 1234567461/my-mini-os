/* ==========================================
 * RAM 文件系统（ramfs）- ramfs.h v0.5.0
 * 功能：
 *   1. 内存中的文件系统
 *   2. 支持目录和文件
 *   3. 动态内存分配存储
 * ========================================== */

#ifndef RAMFS_H
#define RAMFS_H

#include "vfs.h"

/* ==========================================
 * RAM 文件系统节点结构体
 * ========================================== */
typedef struct ramfs_node {
    char name[64];                 /* 节点名称 */
    vfs_file_type_t type;          /* 节点类型（文件/目录） */
    uint32_t size;                 /* 文件大小 */
    void *data;                    /* 文件数据指针 */
    struct ramfs_node *children;   /* 子节点链表（目录用） */
    struct ramfs_node *next;       /* 兄弟节点链表 */
    struct ramfs_node *parent;     /* 父节点 */
} ramfs_node_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化 RAM 文件系统 */
vfs_filesystem_t *ramfs_init();

/* 创建根节点 */
ramfs_node_t *ramfs_create_root();

/* 查找节点 */
ramfs_node_t *ramfs_find_node(ramfs_node_t *root, const char *path);

/* 创建文件节点 */
ramfs_node_t *ramfs_create_file(ramfs_node_t *parent, const char *name);

/* 创建目录节点 */
ramfs_node_t *ramfs_create_dir(ramfs_node_t *parent, const char *name);

/* 删除节点 */
int ramfs_delete_node(ramfs_node_t *parent, const char *name);

#endif /* RAMFS_H */
