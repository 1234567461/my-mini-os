/* ==========================================
 * RAM 文件系统（ramfs）实现 v0.5.0
 * 功能：
 *   1. 内存中的树形文件系统
 *   2. 支持文件创建、读取、写入、删除
 *   3. 支持目录创建、删除、遍历
 * ========================================== */

#include "ramfs.h"
#include "heap.h"
#include "string.h"

/* ==========================================
 * 全局变量
 * ========================================== */
static ramfs_node_t *root_node = NULL;
static vfs_filesystem_t ramfs_fs;
static vfs_ops_t ramfs_ops;

/* ==========================================
 * 创建根节点
 * ========================================== */
ramfs_node_t *ramfs_create_root() {
    ramfs_node_t *root = (ramfs_node_t *)kmalloc(sizeof(ramfs_node_t));
    if (root == NULL) {
        return NULL;
    }
    
    strncpy(root->name, "/", 63);
    root->name[63] = '\0';
    root->type = VFS_DIR;
    root->size = 0;
    root->data = NULL;
    root->children = NULL;
    root->next = NULL;
    root->parent = NULL;
    
    return root;
}

/* ==========================================
 * 按名称查找子节点
 * ========================================== */
static ramfs_node_t *find_child(ramfs_node_t *parent, const char *name) {
    if (parent == NULL || name == NULL) {
        return NULL;
    }
    
    ramfs_node_t *child = parent->children;
    while (child != NULL) {
        if (strcmp(child->name, name) == 0) {
            return child;
        }
        child = child->next;
    }
    
    return NULL;
}

/* ==========================================
 * 查找节点（按路径）
 * ========================================== */
ramfs_node_t *ramfs_find_node(ramfs_node_t *root, const char *path) {
    if (root == NULL || path == NULL) {
        return NULL;
    }
    
    /* 根路径 */
    if (strcmp(path, "/") == 0 || strcmp(path, "") == 0) {
        return root;
    }
    
    const char *p = path;
    ramfs_node_t *current = root;
    
    /* 跳过开头的 / */
    if (*p == '/') {
        p++;
    }
    
    while (*p != '\0') {
        /* 提取路径组件 */
        char component[64];
        int i = 0;
        while (*p != '/' && *p != '\0' && i < 63) {
            component[i++] = *p++;
        }
        component[i] = '\0';
        
        /* 查找子节点 */
        current = find_child(current, component);
        if (current == NULL) {
            return NULL;
        }
        
        /* 跳过 / */
        if (*p == '/') {
            p++;
        }
    }
    
    return current;
}

/* ==========================================
 * 创建文件节点
 * ========================================== */
ramfs_node_t *ramfs_create_file(ramfs_node_t *parent, const char *name) {
    if (parent == NULL || name == NULL) {
        return NULL;
    }
    
    /* 检查是否已存在 */
    if (find_child(parent, name) != NULL) {
        return NULL;
    }
    
    /* 创建新节点 */
    ramfs_node_t *node = (ramfs_node_t *)kmalloc(sizeof(ramfs_node_t));
    if (node == NULL) {
        return NULL;
    }
    
    strncpy(node->name, name, 63);
    node->name[63] = '\0';
    node->type = VFS_FILE;
    node->size = 0;
    node->data = NULL;
    node->children = NULL;
    node->next = NULL;
    node->parent = parent;
    
    /* 添加到父节点的子链表 */
    if (parent->children == NULL) {
        parent->children = node;
    } else {
        ramfs_node_t *last = parent->children;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = node;
    }
    
    return node;
}

/* ==========================================
 * 创建目录节点
 * ========================================== */
ramfs_node_t *ramfs_create_dir(ramfs_node_t *parent, const char *name) {
    if (parent == NULL || name == NULL) {
        return NULL;
    }
    
    /* 检查是否已存在 */
    if (find_child(parent, name) != NULL) {
        return NULL;
    }
    
    /* 创建新节点 */
    ramfs_node_t *node = (ramfs_node_t *)kmalloc(sizeof(ramfs_node_t));
    if (node == NULL) {
        return NULL;
    }
    
    strncpy(node->name, name, 63);
    node->name[63] = '\0';
    node->type = VFS_DIR;
    node->size = 0;
    node->data = NULL;
    node->children = NULL;
    node->next = NULL;
    node->parent = parent;
    
    /* 添加到父节点的子链表 */
    if (parent->children == NULL) {
        parent->children = node;
    } else {
        ramfs_node_t *last = parent->children;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = node;
    }
    
    return node;
}

/* ==========================================
 * 删除节点
 * ========================================== */
int ramfs_delete_node(ramfs_node_t *parent, const char *name) {
    if (parent == NULL || name == NULL) {
        return -1;
    }
    
    ramfs_node_t *prev = NULL;
    ramfs_node_t *current = parent->children;
    
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            /* 从链表中移除 */
            if (prev == NULL) {
                parent->children = current->next;
            } else {
                prev->next = current->next;
            }
            
            /* 释放文件数据 */
            if (current->type == VFS_FILE && current->data != NULL) {
                kfree(current->data);
            }
            
            /* 释放节点 */
            kfree(current);
            
            return 0;
        }
        
        prev = current;
        current = current->next;
    }
    
    return -1;  /* 未找到 */
}

/* ==========================================
 * 获取父目录和文件名
 * ========================================== */
static int split_path(const char *path, char *dir_path, char *filename) {
    if (path == NULL || dir_path == NULL || filename == NULL) {
        return -1;
    }
    
    const char *last_slash = strrchr(path, '/');
    
    if (last_slash == NULL) {
        /* 没有 /，说明是当前目录下的文件 */
        strncpy(dir_path, ".", 255);
        strncpy(filename, path, 63);
    } else if (last_slash == path) {
        /* 根目录下的文件 */
        strncpy(dir_path, "/", 255);
        strncpy(filename, last_slash + 1, 63);
    } else {
        /* 普通路径 */
        int dir_len = last_slash - path;
        strncpy(dir_path, path, dir_len);
        dir_path[dir_len] = '\0';
        strncpy(filename, last_slash + 1, 63);
    }
    
    filename[63] = '\0';
    dir_path[255] = '\0';
    
    return 0;
}

/* ==========================================
 * VFS 操作：打开文件
 * ========================================== */
static vfs_file_t *ramfs_open(const char *path, int flags) {
    ramfs_node_t *node = ramfs_find_node(root_node, path);
    
    if (node == NULL) {
        /* 如果指定了创建标志，创建新文件 */
        if (flags & VFS_O_CREAT) {
            char dir_path[256];
            char filename[64];
            split_path(path, dir_path, filename);
            
            ramfs_node_t *parent = ramfs_find_node(root_node, dir_path);
            if (parent == NULL || parent->type != VFS_DIR) {
                return NULL;
            }
            
            node = ramfs_create_file(parent, filename);
            if (node == NULL) {
                return NULL;
            }
        } else {
            return NULL;
        }
    }
    
    /* 如果是目录，不能打开为文件 */
    if (node->type == VFS_DIR) {
        return NULL;
    }
    
    /* 创建文件描述符 */
    vfs_file_t *file = (vfs_file_t *)kmalloc(sizeof(vfs_file_t));
    if (file == NULL) {
        return NULL;
    }
    
    strncpy(file->name, node->name, 63);
    file->name[63] = '\0';
    file->type = node->type;
    file->size = node->size;
    file->pos = 0;
    file->fs_data = node;
    file->next = NULL;
    
    /* 如果指定了截断标志，清空文件 */
    if (flags & VFS_O_TRUNC) {
        if (node->data != NULL) {
            kfree(node->data);
            node->data = NULL;
        }
        node->size = 0;
        file->size = 0;
    }
    
    return file;
}

/* ==========================================
 * VFS 操作：关闭文件
 * ========================================== */
static int ramfs_close(vfs_file_t *file) {
    if (file == NULL) {
        return -1;
    }
    
    kfree(file);
    return 0;
}

/* ==========================================
 * VFS 操作：读取文件
 * ========================================== */
static int ramfs_read(vfs_file_t *file, void *buf, uint32_t count) {
    if (file == NULL || buf == NULL) {
        return -1;
    }
    
    ramfs_node_t *node = (ramfs_node_t *)file->fs_data;
    if (node == NULL || node->data == NULL) {
        return 0;  /* 空文件 */
    }
    
    /* 计算实际可读字节数 */
    uint32_t available = node->size - file->pos;
    if (count > available) {
        count = available;
    }
    
    if (count == 0) {
        return 0;
    }
    
    /* 复制数据 */
    memcpy(buf, (char *)node->data + file->pos, count);
    file->pos += count;
    
    return count;
}

/* ==========================================
 * VFS 操作：写入文件
 * ========================================== */
static int ramfs_write(vfs_file_t *file, const void *buf, uint32_t count) {
    if (file == NULL || buf == NULL) {
        return -1;
    }
    
    ramfs_node_t *node = (ramfs_node_t *)file->fs_data;
    if (node == NULL) {
        return -1;
    }
    
    /* 计算新的文件大小 */
    uint32_t new_size = file->pos + count;
    
    /* 如果需要，重新分配内存 */
    if (new_size > node->size) {
        void *new_data = kmalloc(new_size);
        if (new_data == NULL) {
            return -1;
        }
        
        /* 复制旧数据 */
        if (node->data != NULL && node->size > 0) {
            memcpy(new_data, node->data, node->size);
            kfree(node->data);
        }
        
        node->data = new_data;
        node->size = new_size;
    }
    
    /* 写入新数据 */
    memcpy((char *)node->data + file->pos, buf, count);
    file->pos += count;
    file->size = node->size;
    
    return count;
}

/* ==========================================
 * VFS 操作：创建文件
 * ========================================== */
static int ramfs_create(const char *path) {
    char dir_path[256];
    char filename[64];
    split_path(path, dir_path, filename);
    
    ramfs_node_t *parent = ramfs_find_node(root_node, dir_path);
    if (parent == NULL || parent->type != VFS_DIR) {
        return -1;
    }
    
    ramfs_node_t *node = ramfs_create_file(parent, filename);
    if (node == NULL) {
        return -1;
    }
    
    return 0;
}

/* ==========================================
 * VFS 操作：删除文件
 * ========================================== */
static int ramfs_unlink(const char *path) {
    char dir_path[256];
    char filename[64];
    split_path(path, dir_path, filename);
    
    ramfs_node_t *parent = ramfs_find_node(root_node, dir_path);
    if (parent == NULL || parent->type != VFS_DIR) {
        return -1;
    }
    
    return ramfs_delete_node(parent, filename);
}

/* ==========================================
 * VFS 操作：创建目录
 * ========================================== */
static int ramfs_mkdir(const char *path) {
    char dir_path[256];
    char dirname[64];
    split_path(path, dir_path, dirname);
    
    ramfs_node_t *parent = ramfs_find_node(root_node, dir_path);
    if (parent == NULL || parent->type != VFS_DIR) {
        return -1;
    }
    
    ramfs_node_t *node = ramfs_create_dir(parent, dirname);
    if (node == NULL) {
        return -1;
    }
    
    return 0;
}

/* ==========================================
 * VFS 操作：删除目录
 * ========================================== */
static int ramfs_rmdir(const char *path) {
    char dir_path[256];
    char dirname[64];
    split_path(path, dir_path, dirname);
    
    ramfs_node_t *parent = ramfs_find_node(root_node, dir_path);
    if (parent == NULL || parent->type != VFS_DIR) {
        return -1;
    }
    
    /* 检查目录是否为空 */
    ramfs_node_t *target = find_child(parent, dirname);
    if (target == NULL || target->type != VFS_DIR) {
        return -1;
    }
    
    if (target->children != NULL) {
        return -1;  /* 目录不为空 */
    }
    
    return ramfs_delete_node(parent, dirname);
}

/* ==========================================
 * VFS 操作：读取目录
 * ========================================== */
static int ramfs_readdir(const char *path, vfs_dirent_t *dirents, int max_count) {
    ramfs_node_t *dir = ramfs_find_node(root_node, path);
    if (dir == NULL || dir->type != VFS_DIR) {
        return -1;
    }
    
    int count = 0;
    ramfs_node_t *child = dir->children;
    
    while (child != NULL && count < max_count) {
        strncpy(dirents[count].name, child->name, 63);
        dirents[count].name[63] = '\0';
        dirents[count].type = child->type;
        count++;
        child = child->next;
    }
    
    return count;
}

/* ==========================================
 * VFS 操作：获取文件属性
 * ========================================== */
static int ramfs_stat(const char *path, vfs_stat_t *buf) {
    ramfs_node_t *node = ramfs_find_node(root_node, path);
    if (node == NULL) {
        return -1;
    }
    
    strncpy(buf->name, node->name, 63);
    buf->name[63] = '\0';
    buf->type = node->type;
    buf->size = node->size;
    buf->inode = (uint32_t)node;  /* 用节点地址作为 inode 号 */
    
    return 0;
}

/* ==========================================
 * 初始化 RAM 文件系统
 * ========================================== */
vfs_filesystem_t *ramfs_init() {
    /* 创建根节点 */
    root_node = ramfs_create_root();
    if (root_node == NULL) {
        return NULL;
    }
    
    /* 设置操作函数表 */
    ramfs_ops.open = ramfs_open;
    ramfs_ops.close = ramfs_close;
    ramfs_ops.read = ramfs_read;
    ramfs_ops.write = ramfs_write;
    ramfs_ops.create = ramfs_create;
    ramfs_ops.unlink = ramfs_unlink;
    ramfs_ops.mkdir = ramfs_mkdir;
    ramfs_ops.rmdir = ramfs_rmdir;
    ramfs_ops.readdir = ramfs_readdir;
    ramfs_ops.stat = ramfs_stat;
    
    /* 设置文件系统结构体 */
    strncpy(ramfs_fs.name, "ramfs", 31);
    ramfs_fs.name[31] = '\0';
    ramfs_fs.ops = &ramfs_ops;
    ramfs_fs.data = root_node;
    ramfs_fs.next = NULL;
    
    return &ramfs_fs;
}
