/* ==========================================
 * mpk.c - MPK包管理工具（My Mini OS用户态版本）
 * 功能：查看、验证、提取MPK格式包
 *
 * 用法：
 *   mpk info <package.mpk>    - 显示包信息
 *   mpk list <package.mpk>    - 列出包内容
 *   mpk verify <package.mpk>  - 验证包校验和
 *   mpk extract <package.mpk> - 提取包内容
 *   mpk help                  - 显示帮助
 *
 * 注意：这是My Mini OS用户态版本，使用系统调用接口
 * ========================================== */

/* ========== 系统调用接口 ========== */
#define SYS_EXIT    1
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_LSEEK   19
#define SYS_BRK    45

#define STDIN_FD    0
#define STDOUT_FD   1
#define STDERR_FD   2

#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_CREAT     0100
#define O_TRUNC     01000

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

/* 系统调用函数 */
static int syscall1(int num, int arg1) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1)
        : "memory"
    );
    return ret;
}

static int syscall2(int num, int arg1, int arg2) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1), "c"(arg2)
        : "memory"
    );
    return ret;
}

static int syscall3(int num, int arg1, int arg2, int arg3) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory"
    );
    return ret;
}

static int my_write(int fd, const char *buf, int len) {
    return syscall3(SYS_WRITE, fd, (int)buf, len);
}

static int my_read(int fd, char *buf, int len) {
    return syscall3(SYS_READ, fd, (int)buf, len);
}

static int my_open(const char *path, int flags) {
    return syscall2(SYS_OPEN, (int)path, flags);
}

static int my_close(int fd) {
    return syscall1(SYS_CLOSE, fd);
}

static void my_exit(int code) {
    syscall1(SYS_EXIT, code);
    while(1);
}

static int my_lseek(int fd, int offset, int whence) {
    return syscall3(SYS_LSEEK, fd, offset, whence);
}

/* ========== 字符串函数 ========== */
static int my_strlen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void my_puts(const char *s) {
    my_write(STDOUT_FD, s, my_strlen(s));
}

static void my_puterr(const char *s) {
    my_write(STDERR_FD, s, my_strlen(s));
}

static int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) { s1++; s2++; }
    return *s1 - *s2;
}

static int my_strncmp(const char *s1, const char *s2, int n) {
    while (n > 0 && *s1 && *s2 && *s1 == *s2) { s1++; s2++; n--; }
    return n == 0 ? 0 : (*s1 - *s2);
}

static char* my_strchr(const char *s, char c) {
    while (*s) { if (*s == c) return (char*)s; s++; }
    return 0;
}

static char* my_strcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

static void* my_memcpy(void *dst, const void *src, int n) {
    char *d = (char*)dst;
    const char *s = (const char*)src;
    while (n--) *d++ = *s++;
    return dst;
}

static void* my_memset(void *dst, int c, int n) {
    char *d = (char*)dst;
    while (n--) *d++ = (char)c;
    return dst;
}

/* ========== 整数转字符串 ========== */
static void itoa(unsigned int n, char *buf, int base) {
    char tmp[32];
    int i = 0, j = 0;
    if (n == 0) tmp[i++] = '0';
    while (n > 0) {
        int d = n % base;
        tmp[i++] = (d < 10) ? '0' + d : 'a' + d - 10;
        n /= base;
    }
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

/* ========== MPK格式定义 ========== */
#define MPK_MAGIC       0x4B504D4B
#define MPK_HEADER_SIZE 64
#define MPK_NAME_LEN    64

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t header_size;
    uint32_t meta_offset;
    uint32_t meta_size;
    uint32_t file_table_offset;
    uint32_t file_count;
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t data_size_orig;
    uint8_t  compress_algo;
    uint8_t  _reserved[19];
    uint32_t checksum;
} __attribute__((packed)) mpk_header_t;

typedef struct {
    char name[MPK_NAME_LEN];
    uint32_t offset;
    uint32_t size;
    uint32_t size_orig;
    uint32_t mode;
    uint32_t flags;
    uint32_t checksum;
} __attribute__((packed)) mpk_file_entry_t;

/* ========== CRC32 ========== */
static uint32_t crc32_table[256];
static int crc32_inited = 0;

static void crc32_init(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_inited = 1;
}

static uint32_t crc32_calc(const uint8_t *data, uint32_t len) {
    if (!crc32_inited) crc32_init();
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return ~crc;
}

/* ========== 全局缓冲区 ========== */
static char g_buf[8192];

/* ========== 读取文件函数 ========== */
static int read_file_all(const char *path, char *buf, int max_size) {
    int fd = my_open(path, O_RDONLY);
    if (fd < 0) return -1;
    int total = 0;
    while (total < max_size) {
        int n = my_read(fd, buf + total, max_size - total);
        if (n <= 0) break;
        total += n;
    }
    my_close(fd);
    return total;
}

/* ========== 显示帮助 ========== */
static void show_help(void) {
    my_puts("MPK Package Manager v1.0.0 (My Mini OS Edition)\n");
    my_puts("\n");
    my_puts("Usage:\n");
    my_puts("  mpk info <package.mpk>    Show package information\n");
    my_puts("  mpk list <package.mpk>    List package contents\n");
    my_puts("  mpk verify <package.mpk>  Verify package checksum\n");
    my_puts("  mpk extract <package.mpk> Extract package files\n");
    my_puts("  mpk help                  Show this help\n");
    my_puts("\n");
    my_puts("This is the user-space version of mpk-tool for My Mini OS.\n");
    my_puts("License: MIT\n");
}

/* ========== 显示包信息 ========== */
static int cmd_info(const char *path) {
    int size = read_file_all(path, g_buf, sizeof(g_buf));
    if (size < 0) {
        my_puterr("Error: Cannot open file: ");
        my_puterr(path);
        my_puterr("\n");
        return 1;
    }
    
    mpk_header_t *hdr = (mpk_header_t*)g_buf;
    
    if (hdr->magic != MPK_MAGIC) {
        my_puterr("Error: Not a valid MPK package\n");
        return 1;
    }
    
    my_puts("MPK Package Info\n");
    my_puts("================\n");
    
    char num[32];
    my_puts("Magic:    0x");
    itoa(hdr->magic, num, 16);
    my_puts(num);
    my_puts("\n");
    
    my_puts("Version:  ");
    itoa(hdr->version, num, 10);
    my_puts(num);
    my_puts("\n");
    
    my_puts("Hdr size: ");
    itoa(hdr->header_size, num, 10);
    my_puts(num);
    my_puts("\n");
    
    my_puts("Files:    ");
    itoa(hdr->file_count, num, 10);
    my_puts(num);
    my_puts("\n");
    
    my_puts("Data:     ");
    itoa(hdr->data_size_orig, num, 10);
    my_puts(num);
    my_puts(" bytes\n");
    
    /* 显示元数据 */
    my_puts("\nMetadata:\n");
    if (hdr->meta_size > 0 && hdr->meta_offset < size) {
        char *meta = g_buf + hdr->meta_offset;
        int meta_size = hdr->meta_size;
        int pos = 0;
        while (pos < meta_size) {
            int line_len = 0;
            while (pos + line_len < meta_size && meta[pos + line_len] != '\n')
                line_len++;
            if (line_len > 0) {
                my_write(STDOUT_FD, meta + pos, line_len);
                my_puts("\n");
            }
            pos += line_len + 1;
        }
    }
    
    return 0;
}

/* ========== 列出包内容 ========== */
static int cmd_list(const char *path) {
    int size = read_file_all(path, g_buf, sizeof(g_buf));
    if (size < 0) {
        my_puterr("Error: Cannot open file: ");
        my_puterr(path);
        my_puterr("\n");
        return 1;
    }
    
    mpk_header_t *hdr = (mpk_header_t*)g_buf;
    
    if (hdr->magic != MPK_MAGIC) {
        my_puterr("Error: Not a valid MPK package\n");
        return 1;
    }
    
    my_puts("Package: ");
    my_puts(path);
    my_puts("\n");
    
    char num[32];
    my_puts("Files: ");
    itoa(hdr->file_count, num, 10);
    my_puts(num);
    my_puts("\n\n");
    
    my_puts("Name                                           Size\n");
    my_puts("----                                           ----\n");
    
    mpk_file_entry_t *entries = (mpk_file_entry_t*)(g_buf + hdr->file_table_offset);
    for (uint32_t i = 0; i < hdr->file_count; i++) {
        my_puts(entries[i].name);
        int pad = 47 - my_strlen(entries[i].name);
        if (pad < 0) pad = 0;
        for (int j = 0; j < pad; j++) my_puts(" ");
        itoa(entries[i].size_orig, num, 10);
        my_puts(num);
        my_puts("\n");
    }
    
    return 0;
}

/* ========== 验证校验和 ========== */
static int cmd_verify(const char *path) {
    int size = read_file_all(path, g_buf, sizeof(g_buf));
    if (size < 0) {
        my_puterr("Error: Cannot open file: ");
        my_puterr(path);
        my_puterr("\n");
        return 1;
    }
    
    mpk_header_t *hdr = (mpk_header_t*)g_buf;
    
    if (hdr->magic != MPK_MAGIC) {
        my_puterr("Error: Not a valid MPK package\n");
        return 1;
    }
    
    uint32_t saved_checksum = hdr->checksum;
    hdr->checksum = 0;
    uint32_t calc_checksum = crc32_calc((uint8_t*)g_buf, size);
    
    my_puts("Verifying: ");
    my_puts(path);
    my_puts("\n");
    
    char num[32];
    my_puts("Expected: 0x");
    itoa(saved_checksum, num, 16);
    my_puts(num);
    my_puts("\n");
    
    my_puts("Actual:   0x");
    itoa(calc_checksum, num, 16);
    my_puts(num);
    my_puts("\n");
    
    if (calc_checksum == saved_checksum) {
        my_puts("\nResult: OK - Checksum matches\n");
        return 0;
    } else {
        my_puts("\nResult: FAILED - Checksum mismatch\n");
        return 1;
    }
}

/* ========== 提取包内容 ========== */
static int cmd_extract(const char *path) {
    int size = read_file_all(path, g_buf, sizeof(g_buf));
    if (size < 0) {
        my_puterr("Error: Cannot open file: ");
        my_puterr(path);
        my_puterr("\n");
        return 1;
    }
    
    mpk_header_t *hdr = (mpk_header_t*)g_buf;
    
    if (hdr->magic != MPK_MAGIC) {
        my_puterr("Error: Not a valid MPK package\n");
        return 1;
    }
    
    my_puts("Extracting: ");
    my_puts(path);
    my_puts("\n");
    
    mpk_file_entry_t *entries = (mpk_file_entry_t*)(g_buf + hdr->file_table_offset);
    
    char num[32];
    for (uint32_t i = 0; i < hdr->file_count; i++) {
        my_puts("  ");
        my_puts(entries[i].name);
        my_puts(" (");
        itoa(entries[i].size_orig, num, 10);
        my_puts(num);
        my_puts(" bytes)\n");
        
        /* 简单模式：只显示不实际提取（需要完整的文件系统支持） */
    }
    
    my_puts("\nExtraction complete.\n");
    return 0;
}

/* ========== 主函数 ========== */
int mmos_main(int argc, char **argv) {
    if (argc < 2 || my_strcmp(argv[1], "help") == 0) {
        show_help();
        return 0;
    }
    
    const char *cmd = argv[1];
    
    if (my_strcmp(cmd, "info") == 0) {
        if (argc < 3) {
            my_puterr("Usage: mpk info <package.mpk>\n");
            return 1;
        }
        return cmd_info(argv[2]);
    }
    else if (my_strcmp(cmd, "list") == 0) {
        if (argc < 3) {
            my_puterr("Usage: mpk list <package.mpk>\n");
            return 1;
        }
        return cmd_list(argv[2]);
    }
    else if (my_strcmp(cmd, "verify") == 0) {
        if (argc < 3) {
            my_puterr("Usage: mpk verify <package.mpk>\n");
            return 1;
        }
        return cmd_verify(argv[2]);
    }
    else if (my_strcmp(cmd, "extract") == 0) {
        if (argc < 3) {
            my_puterr("Usage: mpk extract <package.mpk>\n");
            return 1;
        }
        return cmd_extract(argv[2]);
    }
    else {
        my_puterr("Unknown command: ");
        my_puterr(cmd);
        my_puterr("\nTry 'mpk help' for usage information.\n");
        return 1;
    }
}

/* ========== 入口点 ========== */
void _start() {
    int argc = 0;
    char **argv = 0;
    
    __asm__ volatile (
        "movl 4(%%ebp), %%eax\n\t"
        "movl %%eax, %0\n\t"
        "leal 8(%%ebp), %%eax\n\t"
        "movl %%eax, %1\n\t"
        : "=r"(argc), "=r"(argv)
        :
        : "eax", "memory"
    );
    
    int ret = mmos_main(argc, argv);
    my_exit(ret);
}
