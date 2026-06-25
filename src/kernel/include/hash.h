/* ==========================================
 * 哈希计算 - Hash Functions
 * 完全自研的SHA256和MD5实现
 * ========================================== */

#ifndef HASH_H
#define HASH_H

#include "types.h"

/* MD5 上下文 */
typedef struct {
    uint32_t state[4];     /* 四个32位状态变量 */
    uint64_t count;        /* 位计数器 */
    uint8_t buffer[64];    /* 输入缓冲区 */
} md5_context_t;

/* SHA256 上下文 */
typedef struct {
    uint32_t state[8];     /* 八个32位状态变量 */
    uint64_t count;        /* 位计数器 */
    uint8_t buffer[64];    /* 输入缓冲区 */
} sha256_context_t;

/* 哈希结果 */
#define MD5_HASH_SIZE     16
#define SHA256_HASH_SIZE  32
#define SHA512_HASH_SIZE  64

/* ==========================================
 * SHA-512/256 函数（防碰撞）
 * ========================================== */

/* SHA512/256 上下文 */
typedef struct {
    uint64_t state[8];     /* 八个64位状态变量 */
    uint64_t count[2];     /* 位计数器 */
    uint8_t buffer[128];   /* 输入缓冲区 */
} sha512_256_context_t;

/* 初始化SHA512/256上下文 */
void sha512_256_init(sha512_256_context_t *ctx);

/* 更新SHA512/256 */
void sha512_256_update(sha512_256_context_t *ctx, const uint8_t *input, uint32_t len);

/* 完成SHA512/256计算，输出32字节哈希 */
void sha512_256_final(sha512_256_context_t *ctx, uint8_t digest[SHA512_HASH_SIZE]);

/* 直接计算字符串的SHA512/256 */
void sha512_256_string(const char *str, uint8_t digest[SHA512_HASH_SIZE]);

/* 直接计算数据的SHA512/256 */
void sha512_256_data(const void *data, uint32_t len, uint8_t digest[SHA512_HASH_SIZE]);

/* ==========================================
 * MD5 函数
 * ========================================== */

/* 初始化MD5上下文 */
void md5_init(md5_context_t *ctx);

/* 更新MD5（处理更多数据） */
void md5_update(md5_context_t *ctx, const uint8_t *input, uint32_t len);

/* 完成MD5计算，输出16字节哈希 */
void md5_final(md5_context_t *ctx, uint8_t digest[MD5_HASH_SIZE]);

/* 直接计算字符串的MD5 */
void md5_string(const char *str, uint8_t digest[MD5_HASH_SIZE]);

/* 直接计算数据的MD5 */
void md5_data(const void *data, uint32_t len, uint8_t digest[MD5_HASH_SIZE]);

/* ==========================================
 * SHA256 函数
 * ========================================== */

/* 初始化SHA256上下文 */
void sha256_init(sha256_context_t *ctx);

/* 更新SHA256（处理更多数据） */
void sha256_update(sha256_context_t *ctx, const uint8_t *input, uint32_t len);

/* 完成SHA256计算，输出32字节哈希 */
void sha256_final(sha256_context_t *ctx, uint8_t digest[SHA256_HASH_SIZE]);

/* 直接计算字符串的SHA256 */
void sha256_string(const char *str, uint8_t digest[SHA256_HASH_SIZE]);

/* 直接计算数据的SHA256 */
void sha256_data(const void *data, uint32_t len, uint8_t digest[SHA256_HASH_SIZE]);

/* ==========================================
 * 工具函数
 * ========================================== */

/* 将哈希值转换为十六进制字符串 */
void hash_to_string(const uint8_t *hash, int hash_len, char *output);

/* 验证哈希值（用于比对下载文件） */
int hash_verify(const uint8_t *hash, int hash_len, const char *expected_hex);

#endif /* HASH_H */
