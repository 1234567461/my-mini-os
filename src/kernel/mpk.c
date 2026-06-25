/* ==========================================
 * MPK - My Mini OS Package Format
 * 完全自研的包文件格式实现
 * ========================================== */

#include "mpk.h"
#include "string.h"
#include "vga.h"

/* 简单的CRC32实现 */
static uint32_t crc32_table[256];
static int crc32_table_init = 0;

static void crc32_init_table(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_init = 1;
}

static uint32_t crc32_calc(const uint8_t *data, uint32_t len) {
    if (!crc32_table_init) crc32_init_table();

    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return ~crc;
}

/* 验证MPK文件 */
int mpk_validate(const uint8_t *data, uint32_t size) {
    if (!data || size < sizeof(mpk_header_t)) {
        return -1;
    }

    mpk_header_t *header = (mpk_header_t *)data;

    /* 检查魔数 */
    if (header->magic != MPK_MAGIC) {
        vga_printf("[mpk] Invalid magic: 0x%x (expected 0x%x)\n",
                   header->magic, MPK_MAGIC);
        return -1;
    }

    /* 检查版本 */
    if (header->version != MPK_VERSION) {
        vga_printf("[mpk] Unsupported version: %d (expected %d)\n",
                   header->version, MPK_VERSION);
        return -1;
    }

    /* 检查头部大小 */
    if (header->header_size != MPK_HEADER_SIZE) {
        vga_printf("[mpk] Invalid header size: %d (expected %d)\n",
                   header->header_size, MPK_HEADER_SIZE);
        return -1;
    }

    /* 检查文件大小 */
    if (size < header->data_offset + header->data_size) {
        vga_printf("[mpk] File too small: %d (expected at least %d)\n",
                   size, header->data_offset + header->data_size);
        return -1;
    }

    /* 验证校验和 */
    uint32_t stored_checksum = header->checksum;
    header->checksum = 0;
    uint32_t calc_checksum = crc32_calc(data, size);
    header->checksum = stored_checksum;

    if (stored_checksum != calc_checksum) {
        vga_printf("[mpk] Checksum mismatch: 0x%x (expected 0x%x)\n",
                   calc_checksum, stored_checksum);
        return -1;
    }

    return 0;
}

/* 读取MPK头部 */
int mpk_read_header(const uint8_t *data, uint32_t size, mpk_header_t *header) {
    if (!data || !header || size < sizeof(mpk_header_t)) {
        return -1;
    }

    memcpy(header, data, sizeof(mpk_header_t));
    return 0;
}

/* 简单的键值对解析 */
static int parse_key_value(const char *str, const char *key, char *value, int max_len) {
    int key_len = strlen(key);
    const char *p = str;

    while (*p) {
        /* 跳过空白和换行 */
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

        /* 检查是否匹配key */
        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            p += key_len + 1;  /* 跳过key和= */
            int i = 0;
            while (*p && *p != '\n' && *p != '\r' && i < max_len - 1) {
                value[i++] = *p++;
            }
            value[i] = '\0';
            return 0;
        }

        /* 跳过这一行 */
        while (*p && *p != '\n') p++;
    }
    return -1;
}

/* 解析元数据 */
int mpk_read_metadata(const uint8_t *data, uint32_t size, mpk_metadata_t *meta) {
    if (!data || !meta || size < sizeof(mpk_header_t)) {
        return -1;
    }

    mpk_header_t *header = (mpk_header_t *)data;

    if (header->meta_offset + header->meta_size > size) {
        return -1;
    }

    const char *meta_str = (const char *)(data + header->meta_offset);
    memset(meta, 0, sizeof(mpk_metadata_t));

    char value[256];

    /* 解析基本字段 */
    if (parse_key_value(meta_str, "name", value, sizeof(value)) == 0) {
        strncpy(meta->name, value, MPK_NAME_LEN - 1);
    }
    if (parse_key_value(meta_str, "version", value, sizeof(value)) == 0) {
        strncpy(meta->version, value, MPK_VERSION_LEN - 1);
    }
    if (parse_key_value(meta_str, "description", value, sizeof(value)) == 0) {
        strncpy(meta->description, value, MPK_DESC_LEN - 1);
    }
    if (parse_key_value(meta_str, "author", value, sizeof(value)) == 0) {
        strncpy(meta->author, value, MPK_NAME_LEN - 1);
    }

    /* 解析依赖 */
    meta->dep_count = 0;
    char dep_key[32];
    for (int i = 0; i < MPK_DEP_MAX; i++) {
        itoa(i, dep_key, 10);
        strcat(dep_key, "");
        char full_key[64] = "dep";
        strcat(full_key, dep_key);
        if (parse_key_value(meta_str, full_key, value, sizeof(value)) == 0) {
            strncpy(meta->dependencies[meta->dep_count], value, MPK_NAME_LEN - 1);
            meta->dep_count++;
        } else {
            break;
        }
    }

    return 0;
}

/* 获取文件数量 */
int mpk_get_file_count(const uint8_t *data, uint32_t size) {
    if (!data || size < sizeof(mpk_header_t)) {
        return -1;
    }

    mpk_header_t *header = (mpk_header_t *)data;
    return header->file_count;
}

/* 获取文件条目 */
int mpk_get_file_entry(const uint8_t *data, uint32_t size,
                       int index, mpk_file_entry_t *entry) {
    if (!data || !entry || size < sizeof(mpk_header_t)) {
        return -1;
    }

    mpk_header_t *header = (mpk_header_t *)data;

    if (index < 0 || index >= (int)header->file_count) {
        return -1;
    }

    uint32_t offset = header->file_table_offset + index * sizeof(mpk_file_entry_t);
    if (offset + sizeof(mpk_file_entry_t) > size) {
        return -1;
    }

    memcpy(entry, data + offset, sizeof(mpk_file_entry_t));
    return 0;
}

/* 按名称查找文件 */
int mpk_find_file(const uint8_t *data, uint32_t size,
                  const char *filename, mpk_file_entry_t *entry) {
    if (!data || !filename || !entry) {
        return -1;
    }

    mpk_header_t *header = (mpk_header_t *)data;

    for (uint32_t i = 0; i < header->file_count; i++) {
        mpk_file_entry_t e;
        if (mpk_get_file_entry(data, size, i, &e) == 0) {
            if (strcmp(e.name, filename) == 0) {
                memcpy(entry, &e, sizeof(mpk_file_entry_t));
                return 0;
            }
        }
    }
    return -1;
}

/* 读取文件内容 */
int mpk_read_file(const uint8_t *data, uint32_t size,
                  const char *filename, uint8_t *buf, uint32_t buf_size) {
    mpk_file_entry_t entry;

    if (mpk_find_file(data, size, filename, &entry) != 0) {
        return -1;
    }

    if (entry.size_orig > buf_size) {
        return -1;
    }

    mpk_header_t *header = (mpk_header_t *)data;
    uint32_t file_offset = header->data_offset + entry.offset;

    if (file_offset + entry.size > size) {
        return -1;
    }

    /* 如果未压缩，直接复制 */
    if (header->compress_algo == MPK_COMPRESS_NONE) {
        memcpy(buf, data + file_offset, entry.size_orig);
        return entry.size_orig;
    }

    /* 简单的RLE解压 */
    if (header->compress_algo == MPK_COMPRESS_RLE) {
        uint8_t *src = (uint8_t *)(data + file_offset);
        uint32_t src_len = entry.size;
        uint32_t dst_len = 0;
        uint32_t i = 0;

        while (i < src_len && dst_len < entry.size_orig && dst_len < buf_size) {
            if (i + 1 < src_len && src[i] == src[i + 1] && i + 2 < src_len) {
                uint8_t val = src[i];
                uint8_t count = src[i + 2];
                for (int j = 0; j < count && dst_len < buf_size; j++) {
                    buf[dst_len++] = val;
                }
                i += 3;
            } else {
                buf[dst_len++] = src[i++];
            }
        }
        return dst_len;
    }

    return -1;
}

/* 解压整个包（简化版） */
int mpk_extract_all(const uint8_t *data, uint32_t size, const char *dest_path) {
    mpk_header_t *header = (mpk_header_t *)data;
    int count = 0;

    vga_printf("[mpk] Extracting to %s (%d files)\n", dest_path, header->file_count);

    for (uint32_t i = 0; i < header->file_count; i++) {
        mpk_file_entry_t entry;
        if (mpk_get_file_entry(data, size, i, &entry) == 0) {
            vga_printf("  %s (%d bytes)\n", entry.name, entry.size_orig);
            count++;
        }
    }

    vga_printf("[mpk] Extracted %d files\n", count);
    return count;
}

/* 计算包校验和 */
uint32_t mpk_calculate_checksum(const uint8_t *data, uint32_t size) {
    if (!data || size < sizeof(mpk_header_t)) {
        return 0;
    }

    /* 临时清零头部校验和字段 */
    mpk_header_t *header = (mpk_header_t *)data;
    uint32_t saved = header->checksum;
    header->checksum = 0;

    uint32_t result = crc32_calc(data, size);

    header->checksum = saved;
    return result;
}
