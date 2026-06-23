/* ==========================================
 * 文件系统 - fs.c
 * 功能：
 *   1. 内存文件系统（ramfs）实现
 *   2. 文件操作：open, close, read, write等
 * ========================================== */

#include "fs.h"
#include "string.h"
#include "vga.h"
#include "types.h"

/* ==========================================
 * 全局变量
 * ========================================== */

static file_inode_t inodes[MAX_FILES];       /* inode表 */
static file_descriptor_t fds[16];            /* 文件描述符表（每个进程16个） */

/* ==========================================
 * 内部函数声明
 * ========================================== */

static int find_free_inode();
static int find_free_fd();
static int find_inode_by_name(const char *name);

/* ==========================================
 * 函数：fs_init
 * 功能：初始化文件系统
 * ========================================== */
void fs_init()
{
    vga_puts("Initializing ramfs file system...\n");

    /* 清空inode表 */
    memset(inodes, 0, sizeof(inodes));

    /* 清空文件描述符表 */
    memset(fds, 0, sizeof(fds));

    /* 创建一些初始文件 */
    fs_creat("readme.txt");
    int fd = fs_open("readme.txt", O_WRONLY);
    if (fd >= 0) {
        const char *msg = "Welcome to My Mini OS!\n\nThis is a ramfs file system.\nTry 'ls' to see all files.\n";
        fs_write(fd, msg, strlen(msg));
        fs_close(fd);
    }

    fs_creat("hello.c");
    fd = fs_open("hello.c", O_WRONLY);
    if (fd >= 0) {
        const char *msg = "/* Hello World program */\n#include <stdio.h>\n\nint main() {\n    printf(\"Hello, World!\\n\");\n    return 0;\n}\n";
        fs_write(fd, msg, strlen(msg));
        fs_close(fd);
    }

    vga_puts("  [✓] RamFS initialized\n");
}

/* ==========================================
 * 函数：fs_open
 * 功能：打开文件
 * ========================================== */
int fs_open(const char *path, int flags)
{
    int inode_idx;
    int fd;

    /* 查找文件 */
    inode_idx = find_inode_by_name(path);

    /* 如果文件不存在且指定了O_CREAT，则创建 */
    if (inode_idx < 0 && (flags & O_CREAT)) {
        inode_idx = fs_creat(path);
        if (inode_idx < 0) {
            return -1;
        }
    }

    if (inode_idx < 0) {
        return -1;  /* 文件不存在 */
    }

    /* 找一个空闲的文件描述符 */
    fd = find_free_fd();
    if (fd < 0) {
        return -1;  /* 没有空闲描述符 */
    }

    /* 初始化文件描述符 */
    fds[fd].inode_index = inode_idx;
    fds[fd].flags = flags;
    fds[fd].used = 1;

    /* 如果是追加模式，指针移到文件末尾 */
    if (flags & O_APPEND) {
        fds[fd].offset = inodes[inode_idx].size;
    } else {
        fds[fd].offset = 0;
    }

    return fd;
}

/* ==========================================
 * 函数：fs_close
 * 功能：关闭文件
 * ========================================== */
int fs_close(int fd)
{
    if (fd < 0 || fd >= 16 || !fds[fd].used) {
        return -1;
    }

    fds[fd].used = 0;
    return 0;
}

/* ==========================================
 * 函数：fs_read
 * 功能：读文件
 * ========================================== */
int fs_read(int fd, char *buf, int count)
{
    file_inode_t *inode;
    int bytes_to_read;

    if (fd < 0 || fd >= 16 || !fds[fd].used) {
        return -1;
    }

    if (!(fds[fd].flags & O_RDONLY) && !(fds[fd].flags & O_RDWR)) {
        return -1;  /* 文件不是以读模式打开的 */
    }

    inode = &inodes[fds[fd].inode_index];

    /* 计算实际能读的字节数 */
    bytes_to_read = count;
    if (fds[fd].offset + bytes_to_read > inode->size) {
        bytes_to_read = inode->size - fds[fd].offset;
    }

    if (bytes_to_read <= 0) {
        return 0;  /* 已经到文件末尾 */
    }

    /* 复制数据 */
    memcpy(buf, &inode->data[fds[fd].offset], bytes_to_read);
    fds[fd].offset += bytes_to_read;

    return bytes_to_read;
}

/* ==========================================
 * 函数：fs_write
 * 功能：写文件
 * ========================================== */
int fs_write(int fd, const char *buf, int count)
{
    file_inode_t *inode;
    int bytes_to_write;

    if (fd < 0 || fd >= 16 || !fds[fd].used) {
        return -1;
    }

    if (!(fds[fd].flags & O_WRONLY) && !(fds[fd].flags & O_RDWR)) {
        return -1;  /* 文件不是以写模式打开的 */
    }

    inode = &inodes[fds[fd].inode_index];

    /* 计算实际能写的字节数 */
    bytes_to_write = count;
    if (fds[fd].offset + bytes_to_write > MAX_FILE_SIZE) {
        bytes_to_write = MAX_FILE_SIZE - fds[fd].offset;
    }

    if (bytes_to_write <= 0) {
        return 0;  /* 文件已满 */
    }

    /* 复制数据 */
    memcpy(&inode->data[fds[fd].offset], buf, bytes_to_write);
    fds[fd].offset += bytes_to_write;

    /* 更新文件大小 */
    if (fds[fd].offset > inode->size) {
        inode->size = fds[fd].offset;
    }

    return bytes_to_write;
}

/* ==========================================
 * 函数：fs_lseek
 * 功能：移动文件指针
 * ========================================== */
int fs_lseek(int fd, int offset, int whence)
{
    file_inode_t *inode;
    int new_offset;

    if (fd < 0 || fd >= 16 || !fds[fd].used) {
        return -1;
    }

    inode = &inodes[fds[fd].inode_index];

    switch (whence) {
        case 0:  /* SEEK_SET */
            new_offset = offset;
            break;
        case 1:  /* SEEK_CUR */
            new_offset = fds[fd].offset + offset;
            break;
        case 2:  /* SEEK_END */
            new_offset = inode->size + offset;
            break;
        default:
            return -1;
    }

    if (new_offset < 0 || new_offset > MAX_FILE_SIZE) {
        return -1;
    }

    fds[fd].offset = new_offset;
    return new_offset;
}

/* ==========================================
 * 函数：fs_creat
 * 功能：创建文件
 * ========================================== */
int fs_creat(const char *path)
{
    int inode_idx;

    /* 检查文件是否已存在 */
    if (find_inode_by_name(path) >= 0) {
        return find_inode_by_name(path);  /* 已存在，返回索引 */
    }

    /* 找一个空闲的inode */
    inode_idx = find_free_inode();
    if (inode_idx < 0) {
        return -1;  /* 没有空闲inode */
    }

    /* 初始化inode */
    memset(&inodes[inode_idx], 0, sizeof(file_inode_t));
    strncpy(inodes[inode_idx].name, path, MAX_FILENAME - 1);
    inodes[inode_idx].type = FILE_TYPE_REGULAR;
    inodes[inode_idx].size = 0;
    inodes[inode_idx].magic = FS_MAGIC;

    return inode_idx;
}

/* ==========================================
 * 函数：fs_unlink
 * 功能：删除文件
 * ========================================== */
int fs_unlink(const char *path)
{
    int inode_idx = find_inode_by_name(path);
    if (inode_idx < 0) {
        return -1;
    }

    /* 清空inode */
    memset(&inodes[inode_idx], 0, sizeof(file_inode_t));
    return 0;
}

/* ==========================================
 * 函数：fs_filesize
 * 功能：获取文件大小
 * ========================================== */
int fs_filesize(int fd)
{
    if (fd < 0 || fd >= 16 || !fds[fd].used) {
        return -1;
    }

    return inodes[fds[fd].inode_index].size;
}

/* ==========================================
 * 函数：fs_listdir
 * 功能：列出目录内容
 * ========================================== */
int fs_listdir(const char *path, char *buf, int bufsize)
{
    int i;
    int offset = 0;

    /* 简化：只有根目录 */
    for (i = 0; i < MAX_FILES; i++) {
        if (inodes[i].magic == FS_MAGIC) {
            int len = strlen(inodes[i].name);
            if (offset + len + 2 < bufsize) {
                memcpy(&buf[offset], inodes[i].name, len);
                offset += len;
                buf[offset++] = '\n';
            }
        }
    }

    buf[offset] = '\0';
    return offset;
}

/* ==========================================
 * 函数：fs_exists
 * 功能：检查文件是否存在
 * ========================================== */
int fs_exists(const char *path)
{
    return find_inode_by_name(path) >= 0;
}

/* ==========================================
 * 内部函数：find_free_inode
 * 功能：找一个空闲的inode
 * ========================================== */
static int find_free_inode()
{
    int i;
    for (i = 0; i < MAX_FILES; i++) {
        if (inodes[i].magic != FS_MAGIC) {
            return i;
        }
    }
    return -1;
}

/* ==========================================
 * 内部函数：find_free_fd
 * 功能：找一个空闲的文件描述符
 * ========================================== */
static int find_free_fd()
{
    int i;
    for (i = 0; i < 16; i++) {
        if (!fds[i].used) {
            return i;
        }
    }
    return -1;
}

/* ==========================================
 * 内部函数：find_inode_by_name
 * 功能：根据文件名查找inode
 * ========================================== */
static int find_inode_by_name(const char *name)
{
    int i;
    for (i = 0; i < MAX_FILES; i++) {
        if (inodes[i].magic == FS_MAGIC &&
            strcmp(inodes[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}
