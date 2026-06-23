/* ==========================================
 * 虚拟文件系统（VFS）实现 v0.5.0
 * 功能：
 *   1. 统一的文件操作接口
 *   2. 当前工作目录管理
 *   3. 路径解析
 * ========================================== */

#include "vfs.h"
#include "string.h"
#include "heap.h"

/* ==========================================
 * 全局变量
 * ========================================== */
static vfs_filesystem_t *root_fs = NULL;  /* 根文件系统 */
static char current_dir[256] = "/";       /* 当前工作目录 */

/* ==========================================
 * 初始化 VFS
 * ========================================== */
void vfs_init() {
    strncpy(current_dir, "/", 255);
    current_dir[255] = '\0';
}

/* ==========================================
 * 注册文件系统
 * ========================================== */
int vfs_register_filesystem(vfs_filesystem_t *fs) {
    if (fs == NULL) {
        return -1;
    }
    
    /* 简单的链表管理，这里先不做复杂处理 */
    return 0;
}

/* ==========================================
 * 挂载文件系统到根目录
 * ========================================== */
int vfs_mount_root(vfs_filesystem_t *fs) {
    if (fs == NULL) {
        return -1;
    }
    
    root_fs = fs;
    return 0;
}

/* ==========================================
 * 获取当前工作目录
 * ========================================== */
const char *vfs_getcwd() {
    return current_dir;
}

/* ==========================================
 * 解析路径（转换为绝对路径）
 * ========================================== */
static void resolve_path(const char *path, char *result, int max_len) {
    if (path == NULL || result == NULL) {
        return;
    }
    
    /* 如果是绝对路径，直接复制 */
    if (path[0] == '/') {
        strncpy(result, path, max_len - 1);
        result[max_len - 1] = '\0';
    } else {
        /* 相对路径，拼接当前目录 */
        int len = strlen(current_dir);
        int pos = 0;
        
        /* 复制当前目录 */
        strncpy(result, current_dir, max_len - 1);
        result[max_len - 1] = '\0';
        pos = len;
        
        /* 添加 / 如果需要 */
        if (pos > 0 && result[pos - 1] != '/' && pos < max_len - 1) {
            result[pos] = '/';
            pos++;
            result[pos] = '\0';
        }
        
        /* 拼接路径 */
        int path_len = strlen(path);
        for (int i = 0; i < path_len && pos < max_len - 1; i++) {
            result[pos++] = path[i];
        }
        result[pos] = '\0';
    }
    
    /* 简化路径（处理 . 和 ..） */
    /* 这里做简化处理，实际系统需要更复杂的路径规范化 */
}

/* ==========================================
 * 改变当前工作目录
 * ========================================== */
int vfs_chdir(const char *path) {
    if (path == NULL || root_fs == NULL) {
        return -1;
    }
    
    char abs_path[256];
    resolve_path(path, abs_path, sizeof(abs_path));
    
    /* 检查目录是否存在 */
    vfs_stat_t stat;
    if (root_fs->ops->stat(abs_path, &stat) != 0) {
        return -1;
    }
    
    if (stat.type != VFS_DIR) {
        return -1;
    }
    
    strncpy(current_dir, abs_path, 255);
    current_dir[255] = '\0';
    
    return 0;
}

/* ==========================================
 * 打开文件
 * ========================================== */
vfs_file_t *vfs_open(const char *path, int flags) {
    if (path == NULL || root_fs == NULL) {
        return NULL;
    }
    
    char abs_path[256];
    resolve_path(path, abs_path, sizeof(abs_path));
    
    return root_fs->ops->open(abs_path, flags);
}

/* ==========================================
 * 关闭文件
 * ========================================== */
int vfs_close(vfs_file_t *file) {
    if (file == NULL || root_fs == NULL) {
        return -1;
    }
    
    return root_fs->ops->close(file);
}

/* ==========================================
 * 读取文件
 * ========================================== */
int vfs_read(vfs_file_t *file, void *buf, uint32_t count) {
    if (file == NULL || buf == NULL || root_fs == NULL) {
        return -1;
    }
    
    return root_fs->ops->read(file, buf, count);
}

/* ==========================================
 * 写入文件
 * ========================================== */
int vfs_write(vfs_file_t *file, const void *buf, uint32_t count) {
    if (file == NULL || buf == NULL || root_fs == NULL) {
        return -1;
    }
    
    return root_fs->ops->write(file, buf, count);
}

/* ==========================================
 * 创建文件
 * ========================================== */
int vfs_create(const char *path) {
    if (path == NULL || root_fs == NULL) {
        return -1;
    }
    
    char abs_path[256];
    resolve_path(path, abs_path, sizeof(abs_path));
    
    return root_fs->ops->create(abs_path);
}

/* ==========================================
 * 删除文件
 * ========================================== */
int vfs_unlink(const char *path) {
    if (path == NULL || root_fs == NULL) {
        return -1;
    }
    
    char abs_path[256];
    resolve_path(path, abs_path, sizeof(abs_path));
    
    return root_fs->ops->unlink(abs_path);
}

/* ==========================================
 * 创建目录
 * ========================================== */
int vfs_mkdir(const char *path) {
    if (path == NULL || root_fs == NULL) {
        return -1;
    }
    
    char abs_path[256];
    resolve_path(path, abs_path, sizeof(abs_path));
    
    return root_fs->ops->mkdir(abs_path);
}

/* ==========================================
 * 删除目录
 * ========================================== */
int vfs_rmdir(const char *path) {
    if (path == NULL || root_fs == NULL) {
        return -1;
    }
    
    char abs_path[256];
    resolve_path(path, abs_path, sizeof(abs_path));
    
    return root_fs->ops->rmdir(abs_path);
}

/* ==========================================
 * 读取目录
 * ========================================== */
int vfs_readdir(const char *path, vfs_dirent_t *dirents, int max_count) {
    if (path == NULL || dirents == NULL || root_fs == NULL) {
        return -1;
    }
    
    char abs_path[256];
    resolve_path(path, abs_path, sizeof(abs_path));
    
    return root_fs->ops->readdir(abs_path, dirents, max_count);
}

/* ==========================================
 * 获取文件属性
 * ========================================== */
int vfs_stat(const char *path, vfs_stat_t *buf) {
    if (path == NULL || buf == NULL || root_fs == NULL) {
        return -1;
    }
    
    char abs_path[256];
    resolve_path(path, abs_path, sizeof(abs_path));
    
    return root_fs->ops->stat(abs_path, buf);
}
