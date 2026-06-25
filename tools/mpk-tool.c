/* ==========================================
 * mpk-tool - MPK包管理工具
 * 用于创建、查看、提取MPK格式的包
 * 在宿主机上运行
 * ==========================================
 * 用法:
 *   mpk-tool create <output.mpk> <file1> [file2 ...]
 *   mpk-tool list <package.mpk>
 *   mpk-tool extract <package.mpk> [output_dir]
 *   mpk-tool info <package.mpk>
 *   mpk-tool convert <input_dir> <output.mpk> --name <name> --version <ver>
 * ========================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

/* MPK格式定义（与内核一致） */
#define MPK_MAGIC       0x4B504D4B
#define MPK_VERSION     1
#define MPK_HEADER_SIZE 64

#define MPK_NAME_LEN    64
#define MPK_VERSION_LEN 32
#define MPK_DESC_LEN    256
#define MPK_FILE_MAX    256
#define MPK_DEP_MAX     16

#define MPK_FLAG_COMPRESSED  0x0001
#define MPK_COMPRESS_NONE    0
#define MPK_COMPRESS_RLE     1

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
    uint8_t  _reserved[19];   /* 保留，凑64字节头部 */
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

/* CRC32实现 */
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

/* RLE压缩 */
static uint32_t rle_compress(const uint8_t *src, uint32_t src_len,
                             uint8_t *dst, uint32_t dst_size) {
    uint32_t src_i = 0, dst_i = 0;
    while (src_i < src_len && dst_i + 3 < dst_size) {
        uint8_t val = src[src_i];
        uint8_t count = 1;

        /* 查找连续相同字节 */
        while (src_i + count < src_len &&
               src[src_i + count] == val && count < 255) {
            count++;
        }

        if (count >= 3) {
            /* 可压缩 */
            dst[dst_i++] = val;
            dst[dst_i++] = val;
            dst[dst_i++] = count;
            src_i += count;
        } else {
            /* 不可压缩，直接拷贝 */
            dst[dst_i++] = val;
            src_i++;
        }
    }
    return dst_i;
}

/* 读取文件内容 */
static uint8_t *read_file(const char *path, uint32_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *buf = (uint8_t *)malloc(size);
    if (!buf) { fclose(f); return NULL; }

    fread(buf, 1, size, f);
    fclose(f);

    *out_size = (uint32_t)size;
    return buf;
}

/* 递归收集文件 */
typedef struct {
    char path[512];
    char name_in_pkg[MPK_NAME_LEN];
    uint32_t size;
    uint8_t *data;
} file_info_t;

static int collect_files(const char *base_dir, const char *rel_dir,
                         file_info_t *files, int max_files, int *count) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_dir, rel_dir);

    DIR *d = opendir(full_path);
    if (!d) return -1;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL && *count < max_files) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char rel_path[512];
        if (strlen(rel_dir) > 0) {
            snprintf(rel_path, sizeof(rel_path), "%s/%s", rel_dir, entry->d_name);
        } else {
            snprintf(rel_path, sizeof(rel_path), "%s", entry->d_name);
        }

        char file_full[1024];
        snprintf(file_full, sizeof(file_full), "%s/%s", base_dir, rel_path);

        struct stat st;
        if (stat(file_full, &st) != 0) continue;

        if (S_ISREG(st.st_mode)) {
            uint32_t size;
            uint8_t *data = read_file(file_full, &size);
            if (data) {
                strncpy(files[*count].path, file_full, 511);
                strncpy(files[*count].name_in_pkg, rel_path, MPK_NAME_LEN - 1);
                files[*count].size = size;
                files[*count].data = data;
                (*count)++;
            }
        } else if (S_ISDIR(st.st_mode)) {
            collect_files(base_dir, rel_path, files, max_files, count);
        }
    }

    closedir(d);
    return 0;
}

/* 创建MPK包 */
static int cmd_create(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: mpk-tool create <output.mpk> <file1> [file2 ...]\n");
        return 1;
    }

    const char *output_path = argv[2];

    /* 收集文件 */
    file_info_t files[MPK_FILE_MAX];
    int file_count = 0;

    for (int i = 3; i < argc && file_count < MPK_FILE_MAX; i++) {
        struct stat st;
        if (stat(argv[i], &st) != 0) {
            fprintf(stderr, "Warning: cannot stat %s\n", argv[i]);
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            uint32_t size;
            uint8_t *data = read_file(argv[i], &size);
            if (data) {
                strncpy(files[file_count].path, argv[i], 511);
                /* 只取文件名 */
                const char *basename = strrchr(argv[i], '/');
                if (basename) basename++;
                else basename = argv[i];
                strncpy(files[file_count].name_in_pkg, basename, MPK_NAME_LEN - 1);
                files[file_count].size = size;
                files[file_count].data = data;
                file_count++;
            }
        } else if (S_ISDIR(st.st_mode)) {
            collect_files(argv[i], "", files, MPK_FILE_MAX, &file_count);
        }
    }

    if (file_count == 0) {
        fprintf(stderr, "No files to package\n");
        return 1;
    }

    printf("Creating %s with %d files...\n", output_path, file_count);

    /* 计算元数据 */
    char metadata[1024];
    int meta_len = snprintf(metadata, sizeof(metadata),
        "name=custom-pkg\n"
        "version=1.0.0\n"
        "description=Custom MPK package\n"
        "author=mpk-tool\n");

    /* 计算文件表和数据区大小 */
    uint32_t file_table_size = file_count * sizeof(mpk_file_entry_t);
    uint32_t data_offset = MPK_HEADER_SIZE + meta_len + file_table_size;

    /* 压缩数据 */
    uint8_t *data_buf = (uint8_t *)malloc(4 * 1024 * 1024); /* 4MB */
    mpk_file_entry_t *entries = (mpk_file_entry_t *)calloc(file_count, sizeof(mpk_file_entry_t));
    uint32_t data_pos = 0;
    uint32_t total_orig = 0;

    for (int i = 0; i < file_count; i++) {
        strncpy(entries[i].name, files[i].name_in_pkg, MPK_NAME_LEN - 1);
        entries[i].offset = data_pos;
        entries[i].size_orig = files[i].size;
        entries[i].mode = 0644;
        entries[i].checksum = crc32_calc(files[i].data, files[i].size);
        total_orig += files[i].size;

        /* 尝试RLE压缩 */
        uint32_t comp_size = rle_compress(files[i].data, files[i].size,
                                          data_buf + data_pos,
                                          4 * 1024 * 1024 - data_pos);

        /* 如果压缩比不好，用原始数据 */
        if (comp_size > 0 && comp_size < files[i].size) {
            entries[i].size = comp_size;
            data_pos += comp_size;
        } else {
            memcpy(data_buf + data_pos, files[i].data, files[i].size);
            entries[i].size = files[i].size;
            data_pos += files[i].size;
        }

        free(files[i].data);
    }

    /* 构建头部 */
    mpk_header_t header;
    memset(&header, 0, sizeof(header));
    header.magic = MPK_MAGIC;
    header.version = MPK_VERSION;
    header.flags = 0;
    header.header_size = MPK_HEADER_SIZE;
    header.meta_offset = MPK_HEADER_SIZE;
    header.meta_size = meta_len;
    header.file_table_offset = MPK_HEADER_SIZE + meta_len;
    header.file_count = file_count;
    header.data_offset = data_offset;
    header.data_size = data_pos;
    header.data_size_orig = total_orig;
    header.compress_algo = MPK_COMPRESS_NONE; /* 简化：标记为未压缩 */
    header.checksum = 0;

    /* 写入文件 */
    FILE *f = fopen(output_path, "w+b");
    if (!f) {
        fprintf(stderr, "Cannot create %s\n", output_path);
        free(data_buf);
        free(entries);
        return 1;
    }

    fwrite(&header, sizeof(header), 1, f);
    fwrite(metadata, 1, meta_len, f);
    fwrite(entries, sizeof(mpk_file_entry_t), file_count, f);
    fwrite(data_buf, 1, data_pos, f);
    fflush(f);

    /* 计算校验和 */
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *full_buf = (uint8_t *)malloc(file_size);
    fread(full_buf, 1, file_size, f);

    uint32_t checksum = crc32_calc(full_buf, file_size);
    ((mpk_header_t *)full_buf)->checksum = checksum;

    fseek(f, 0, SEEK_SET);
    fwrite(full_buf, 1, file_size, f);
    fclose(f);

    printf("Created: %s (%ld bytes)\n", output_path, file_size);
    printf("  Files: %d\n", file_count);
    printf("  Original size: %d bytes\n", total_orig);
    printf("  Compressed size: %d bytes\n", data_pos);
    printf("  Checksum: 0x%08x\n", checksum);

    free(data_buf);
    free(entries);
    free(full_buf);
    return 0;
}

/* 列出包内容 */
static int cmd_list(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: mpk-tool list <package.mpk>\n");
        return 1;
    }

    uint32_t size;
    uint8_t *data = read_file(argv[2], &size);
    if (!data) {
        fprintf(stderr, "Cannot read %s\n", argv[2]);
        return 1;
    }

    mpk_header_t *header = (mpk_header_t *)data;
    if (header->magic != MPK_MAGIC) {
        fprintf(stderr, "Not a valid MPK file\n");
        free(data);
        return 1;
    }

    printf("Package: %s\n", argv[2]);
    printf("Version: %d\n", header->version);
    printf("Files: %d\n", header->file_count);
    printf("Size: %d / %d bytes\n", header->data_size, header->data_size_orig);
    printf("\n");
    printf("%-40s %10s\n", "Name", "Size");
    printf("%-40s %10s\n", "----", "----");

    mpk_file_entry_t *entries = (mpk_file_entry_t *)(data + header->file_table_offset);
    for (uint32_t i = 0; i < header->file_count; i++) {
        printf("%-40s %10d\n", entries[i].name, entries[i].size_orig);
    }

    free(data);
    return 0;
}

/* 显示包信息 */
static int cmd_info(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: mpk-tool info <package.mpk>\n");
        return 1;
    }

    uint32_t size;
    uint8_t *data = read_file(argv[2], &size);
    if (!data) {
        fprintf(stderr, "Cannot read %s\n", argv[2]);
        return 1;
    }

    mpk_header_t *header = (mpk_header_t *)data;
    if (header->magic != MPK_MAGIC) {
        fprintf(stderr, "Not a valid MPK file (bad magic)\n");
        free(data);
        return 1;
    }

    printf("MPK Package Info\n");
    printf("================\n");
    printf("Magic:    0x%08x\n", header->magic);
    printf("Version:  %d\n", header->version);
    printf("Flags:    0x%04x\n", header->flags);
    printf("Hdr size: %d\n", header->header_size);
    printf("Meta off: %d (size: %d)\n", header->meta_offset, header->meta_size);
    printf("Files:    %d\n", header->file_count);
    printf("Data off: %d\n", header->data_offset);
    printf("Data:     %d / %d bytes (compressed/original)\n",
           header->data_size, header->data_size_orig);
    printf("Checksum: 0x%08x\n", header->checksum);

    /* 验证校验和 */
    uint32_t saved = header->checksum;
    header->checksum = 0;
    uint32_t calc = crc32_calc(data, size);
    header->checksum = saved;
    printf("Verify:   %s\n", saved == calc ? "OK" : "FAILED");

    /* 显示元数据 */
    if (header->meta_size > 0 && header->meta_offset < size) {
        printf("\nMetadata:\n");
        fwrite(data + header->meta_offset, 1, header->meta_size, stdout);
    }

    free(data);
    return 0;
}

/* 提取包 */
static int cmd_extract(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: mpk-tool extract <package.mpk> [output_dir]\n");
        return 1;
    }

    const char *out_dir = (argc >= 4) ? argv[3] : ".";

    uint32_t size;
    uint8_t *data = read_file(argv[2], &size);
    if (!data) {
        fprintf(stderr, "Cannot read %s\n", argv[2]);
        return 1;
    }

    mpk_header_t *header = (mpk_header_t *)data;
    if (header->magic != MPK_MAGIC) {
        fprintf(stderr, "Not a valid MPK file\n");
        free(data);
        return 1;
    }

    printf("Extracting %s to %s/ (%d files)\n", argv[2], out_dir, header->file_count);

    mpk_file_entry_t *entries = (mpk_file_entry_t *)(data + header->file_table_offset);

    for (uint32_t i = 0; i < header->file_count; i++) {
        char out_path[1024];
        snprintf(out_path, sizeof(out_path), "%s/%s", out_dir, entries[i].name);

        /* 创建目录 */
        char *slash = out_path;
        while ((slash = strchr(slash + 1, '/')) != NULL) {
            *slash = '\0';
            mkdir(out_path, 0755);
            *slash = '/';
        }

        FILE *f = fopen(out_path, "wb");
        if (f) {
            fwrite(data + header->data_offset + entries[i].offset,
                   1, entries[i].size, f);
            fclose(f);
            printf("  %s (%d bytes)\n", entries[i].name, entries[i].size_orig);
        } else {
            fprintf(stderr, "  Failed: %s\n", entries[i].name);
        }
    }

    printf("Done.\n");
    free(data);
    return 0;
}

/* 转换目录为MPK包（带元数据） */
static int cmd_convert(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: mpk-tool convert <input_dir> <output.mpk> "
               "[--name <name>] [--version <ver>] [--desc <desc>]\n");
        return 1;
    }

    const char *input_dir = argv[2];
    const char *output_path = argv[3];
    const char *pkg_name = "package";
    const char *pkg_version = "1.0.0";
    const char *pkg_desc = "Converted package";

    /* 解析额外参数 */
    for (int i = 4; i < argc - 1; i++) {
        if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
            pkg_name = argv[++i];
        } else if (strcmp(argv[i], "--version") == 0 && i + 1 < argc) {
            pkg_version = argv[++i];
        } else if (strcmp(argv[i], "--desc") == 0 && i + 1 < argc) {
            pkg_desc = argv[++i];
        }
    }

    /* 收集文件 */
    file_info_t files[MPK_FILE_MAX];
    int file_count = 0;
    collect_files(input_dir, "", files, MPK_FILE_MAX, &file_count);

    if (file_count == 0) {
        fprintf(stderr, "No files found in %s\n", input_dir);
        return 1;
    }

    printf("Converting %s -> %s (%d files)\n", input_dir, output_path, file_count);
    printf("  Name: %s\n", pkg_name);
    printf("  Version: %s\n", pkg_version);

    /* 构建元数据 */
    char metadata[2048];
    int meta_len = snprintf(metadata, sizeof(metadata),
        "name=%s\n"
        "version=%s\n"
        "description=%s\n"
        "author=mpk-tool\n",
        pkg_name, pkg_version, pkg_desc);

    /* 分配缓冲区 */
    uint8_t *data_buf = (uint8_t *)malloc(8 * 1024 * 1024); /* 8MB */
    mpk_file_entry_t *entries = (mpk_file_entry_t *)calloc(file_count, sizeof(mpk_file_entry_t));
    uint32_t data_pos = 0;
    uint32_t total_orig = 0;

    for (int i = 0; i < file_count; i++) {
        strncpy(entries[i].name, files[i].name_in_pkg, MPK_NAME_LEN - 1);
        entries[i].offset = data_pos;
        entries[i].size_orig = files[i].size;
        entries[i].size = files[i].size;
        entries[i].mode = 0755;
        entries[i].checksum = crc32_calc(files[i].data, files[i].size);

        memcpy(data_buf + data_pos, files[i].data, files[i].size);
        data_pos += files[i].size;
        total_orig += files[i].size;

        free(files[i].data);
    }

    uint32_t file_table_size = file_count * sizeof(mpk_file_entry_t);
    uint32_t data_offset = MPK_HEADER_SIZE + meta_len + file_table_size;

    /* 构建头部 */
    mpk_header_t header;
    memset(&header, 0, sizeof(header));
    header.magic = MPK_MAGIC;
    header.version = MPK_VERSION;
    header.flags = 0;
    header.header_size = MPK_HEADER_SIZE;
    header.meta_offset = MPK_HEADER_SIZE;
    header.meta_size = meta_len;
    header.file_table_offset = MPK_HEADER_SIZE + meta_len;
    header.file_count = file_count;
    header.data_offset = data_offset;
    header.data_size = data_pos;
    header.data_size_orig = total_orig;
    header.compress_algo = MPK_COMPRESS_NONE;
    header.checksum = 0;

    /* 写入文件 */
    FILE *f = fopen(output_path, "w+b");
    if (!f) {
        fprintf(stderr, "Cannot create %s\n", output_path);
        free(data_buf);
        free(entries);
        return 1;
    }

    fwrite(&header, sizeof(header), 1, f);
    fwrite(metadata, 1, meta_len, f);
    fwrite(entries, sizeof(mpk_file_entry_t), file_count, f);
    fwrite(data_buf, 1, data_pos, f);
    fflush(f);

    /* 计算校验和 */
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *full_buf = (uint8_t *)malloc(file_size);
    fread(full_buf, 1, file_size, f);

    uint32_t checksum = crc32_calc(full_buf, file_size);
    ((mpk_header_t *)full_buf)->checksum = checksum;

    fseek(f, 0, SEEK_SET);
    fwrite(full_buf, 1, file_size, f);
    fclose(f);

    printf("Created: %s (%ld bytes)\n", output_path, file_size);
    printf("  Files: %d, total: %d bytes\n", file_count, total_orig);
    printf("  Checksum: 0x%08x\n", checksum);

    free(data_buf);
    free(entries);
    free(full_buf);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("MPK Tool - My Mini OS Package Manager\n");
        printf("Usage:\n");
        printf("  mpk-tool create <output.mpk> <file1> [file2 ...]\n");
        printf("  mpk-tool list <package.mpk>\n");
        printf("  mpk-tool extract <package.mpk> [output_dir]\n");
        printf("  mpk-tool info <package.mpk>\n");
        printf("  mpk-tool convert <dir> <output.mpk> [--name name] [--version ver]\n");
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "create") == 0) {
        return cmd_create(argc, argv);
    } else if (strcmp(cmd, "list") == 0) {
        return cmd_list(argc, argv);
    } else if (strcmp(cmd, "extract") == 0) {
        return cmd_extract(argc, argv);
    } else if (strcmp(cmd, "info") == 0) {
        return cmd_info(argc, argv);
    } else if (strcmp(cmd, "convert") == 0) {
        return cmd_convert(argc, argv);
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        return 1;
    }
}
