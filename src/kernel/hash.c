/* ==========================================
 * 哈希计算实现 - Hash Functions
 * 完全自研的SHA256和MD5实现
 * ========================================== */

#include "hash.h"
#include "string.h"
#include "types.h"

/* ==========================================
 * MD5 实现
 * RFC 1321 标准
 * ========================================== */

/* MD5 辅助宏 */
#define MD5_F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | ~(z)))

#define MD5_ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/* MD5 F, G, H, I 变换宏 */
#define MD5_FF(a, b, c, d, x, s, ac) { \
    (a) += MD5_F((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = MD5_ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define MD5_GG(a, b, c, d, x, s, ac) { \
    (a) += MD5_G((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = MD5_ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define MD5_HH(a, b, c, d, x, s, ac) { \
    (a) += MD5_H((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = MD5_ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define MD5_II(a, b, c, d, x, s, ac) { \
    (a) += MD5_I((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = MD5_ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}

/* MD5 K表 - 预计算的常数 */
static const uint32_t MD5_K_TABLE[64] = {
    0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
    0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
    0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
    0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
    0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
    0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
    0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
    0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
    0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
    0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
    0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
    0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
    0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
    0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
    0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
    0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
};

/* 获取MD5 K值 */
static uint32_t md5_k(int i) {
    return MD5_K_TABLE[i & 63];
}

/* MD5初始化 */
void md5_init(md5_context_t *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->count = 0;
}

/* MD5 块更新操作 */
static void md5_process_block(const uint8_t *block, md5_context_t *ctx) {
    uint32_t a = ctx->state[0];
    uint32_t b = ctx->state[1];
    uint32_t c = ctx->state[2];
    uint32_t d = ctx->state[3];
    uint32_t x[16];

    /* 将块转换为16个32位字 */
    for (int i = 0; i < 16; i++) {
        x[i] = (uint32_t)block[i * 4] |
               ((uint32_t)block[i * 4 + 1] << 8) |
               ((uint32_t)block[i * 4 + 2] << 16) |
               ((uint32_t)block[i * 4 + 3] << 24);
    }

    /* 第一轮：16次操作 */
    MD5_FF(a, b, c, d, x[0],  7,  0xD76AA478);
    MD5_FF(d, a, b, c, x[1],  12, 0xE8C7B756);
    MD5_FF(c, d, a, b, x[2],  17, 0x242070DB);
    MD5_FF(b, c, d, a, x[3],  22, 0xC1BDCEEE);
    MD5_FF(a, b, c, d, x[4],  7,  0xF57C0FAF);
    MD5_FF(d, a, b, c, x[5],  12, 0x4787C62A);
    MD5_FF(c, d, a, b, x[6],  17, 0xA8304613);
    MD5_FF(b, c, d, a, x[7],  22, 0xFD469501);
    MD5_FF(a, b, c, d, x[8],  7,  0x698098D8);
    MD5_FF(d, a, b, c, x[9],  12, 0x8B44F7AF);
    MD5_FF(c, d, a, b, x[10], 17, 0xFFFF5BB1);
    MD5_FF(b, c, d, a, x[11], 22, 0x895CD7BE);
    MD5_FF(a, b, c, d, x[12], 7,  0x6B901122);
    MD5_FF(d, a, b, c, x[13], 12, 0xFD987193);
    MD5_FF(c, d, a, b, x[14], 17, 0xA679438E);
    MD5_FF(b, c, d, a, x[15], 22, 0x49B40821);

    /* 第二轮：16次操作 */
    MD5_GG(a, b, c, d, x[1],  5,  0xF61E2562);
    MD5_GG(d, a, b, c, x[6],  9,  0xC040B340);
    MD5_GG(c, d, a, b, x[11], 14, 0x265E5A51);
    MD5_GG(b, c, d, a, x[0],  20, 0xE9B6C7AA);
    MD5_GG(a, b, c, d, x[5],  5,  0xD62F105D);
    MD5_GG(d, a, b, c, x[10], 9,  0x02441453);
    MD5_GG(c, d, a, b, x[15], 14, 0xD8A1E681);
    MD5_GG(b, c, d, a, x[4],  20, 0xE7D3FBC8);
    MD5_GG(a, b, c, d, x[9],  5,  0x21E1CDE6);
    MD5_GG(d, a, b, c, x[14], 9,  0xC33707D6);
    MD5_GG(c, d, a, b, x[3],  14, 0xF4D50D87);
    MD5_GG(b, c, d, a, x[8],  20, 0x455A14ED);
    MD5_GG(a, b, c, d, x[13], 5,  0xA9E3E905);
    MD5_GG(d, a, b, c, x[2],  9,  0xFCEFA3F8);
    MD5_GG(c, d, a, b, x[7],  14, 0x676F02D9);
    MD5_GG(b, c, d, a, x[12], 20, 0x8D2A4C8A);

    /* 第三轮：16次操作 */
    MD5_HH(a, b, c, d, x[5],  4,  0xFFFA3942);
    MD5_HH(d, a, b, c, x[8],  11, 0x8771F681);
    MD5_HH(c, d, a, b, x[11], 16, 0x6D9D6122);
    MD5_HH(b, c, d, a, x[14], 23, 0xFDE5380C);
    MD5_HH(a, b, c, d, x[1],  4,  0xA4BEEA44);
    MD5_HH(d, a, b, c, x[4],  11, 0x4BDECFA9);
    MD5_HH(c, d, a, b, x[7],  16, 0xF6BB4B60);
    MD5_HH(b, c, d, a, x[10], 23, 0xBEBFBC70);
    MD5_HH(a, b, c, d, x[13], 4,  0x289B7EC6);
    MD5_HH(d, a, b, c, x[0],  11, 0xEAA127FA);
    MD5_HH(c, d, a, b, x[3],  16, 0xD4EF3085);
    MD5_HH(b, c, d, a, x[6],  23, 0x04881D05);
    MD5_HH(a, b, c, d, x[9],  4,  0xD9D4D039);
    MD5_HH(d, a, b, c, x[12], 11, 0xE6DB99E5);
    MD5_HH(c, d, a, b, x[15], 16, 0x1FA27CF8);
    MD5_HH(b, c, d, a, x[2],  23, 0xC4AC5665);

    /* 第四轮：16次操作 */
    MD5_II(a, b, c, d, x[0],  6,  0xF4292244);
    MD5_II(d, a, b, c, x[7],  10, 0x432AFF97);
    MD5_II(c, d, a, b, x[14], 15, 0xAB9423A7);
    MD5_II(b, c, d, a, x[5],  21, 0xFC93A039);
    MD5_II(a, b, c, d, x[12], 6,  0x655B59C3);
    MD5_II(d, a, b, c, x[3],  10, 0x8F0CCC92);
    MD5_II(c, d, a, b, x[10], 15, 0xFFEFF47D);
    MD5_II(b, c, d, a, x[1],  21, 0x85845DD1);
    MD5_II(a, b, c, d, x[8],  6,  0x6FA87E4F);
    MD5_II(d, a, b, c, x[15], 10, 0xFE2CE6E0);
    MD5_II(c, d, a, b, x[6],  15, 0xA3014314);
    MD5_II(b, c, d, a, x[13], 21, 0x4E0811A1);
    MD5_II(a, b, c, d, x[4],  6,  0xF7537E82);
    MD5_II(d, a, b, c, x[11], 10, 0xBD3AF235);
    MD5_II(c, d, a, b, x[2],  15, 0x2AD7D2BB);
    MD5_II(b, c, d, a, x[9],  21, 0xEB86D391);

    /* 更新状态 */
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
}

/* MD5 更新 */
void md5_update(md5_context_t *ctx, const uint8_t *input, uint32_t len) {
    uint32_t index = (uint32_t)((ctx->count >> 3) & 0x3F);
    uint32_t partlen = 64 - index;

    ctx->count += (uint64_t)len << 3;

    if (len >= partlen) {
        memcpy(ctx->buffer + index, input, partlen);
        md5_process_block(ctx->buffer, ctx);

        for (uint32_t i = partlen; i + 63 < len; i += 64) {
            md5_process_block(input + i, ctx);
        }
        index = 0;
    } else {
        index = 0;
    }

    memcpy(ctx->buffer + index, input, len);
}

/* MD5 完成 */
void md5_final(md5_context_t *ctx, uint8_t digest[MD5_HASH_SIZE]) {
    uint32_t index = (uint32_t)((ctx->count >> 3) & 0x3F);
    uint32_t padlen = (index < 56) ? (56 - index) : (120 - index);

    /* 填充：1位 + 0位 */
    uint8_t padding[64];
    padding[0] = 0x80;
    memset(padding + 1, 0, padlen - 1);

    md5_update(ctx, padding, padlen);

    /* 追加长度（64位） */
    uint8_t length[8];
    uint64_t bits = ctx->count;
    for (int i = 0; i < 8; i++) {
        length[i] = (uint8_t)(bits >> (i * 8));
    }
    md5_update(ctx, length, 8);

    /* 输出摘要 */
    for (int i = 0; i < 4; i++) {
        digest[i * 4]     = (uint8_t)(ctx->state[i]);
        digest[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 8);
        digest[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 16);
        digest[i * 4 + 3] = (uint8_t)(ctx->state[i] >> 24);
    }
}

/* 直接计算字符串的MD5 */
void md5_string(const char *str, uint8_t digest[MD5_HASH_SIZE]) {
    md5_context_t ctx;
    md5_init(&ctx);
    md5_update(&ctx, (const uint8_t *)str, strlen(str));
    md5_final(&ctx, digest);
}

/* 直接计算数据的MD5 */
void md5_data(const void *data, uint32_t len, uint8_t digest[MD5_HASH_SIZE]) {
    md5_context_t ctx;
    md5_init(&ctx);
    md5_update(&ctx, (const uint8_t *)data, len);
    md5_final(&ctx, digest);
}

/* ==========================================
 * SHA-512/256 实现（防碰撞）
 * FIPS 180-4 标准，截断至256位
 * 完全自研
 * ========================================== */

/* SHA-512/256 初始哈希值（SHA-512前8个质数平方根的小数部分） */
static const uint64_t SHA512_256_INIT[8] = {
    0x22312194FC2BF72C, 0x9F555FA3C84C64C2,
    0x2393B86B6F53B151, 0x963877195940EABD,
    0x96283EE2A88EFFE3, 0xBE5E1E2553863992,
    0x2B0199FC2C85B8AA, 0x0EB72DDC81C52CA2
};

/* SHA-512 K常数（64个质数立方根的小数部分） */
static const uint64_t SHA512_K[80] = {
    0x428A2F98D728AE22, 0x7137449123EF65CD, 0xB5C0FBCFEC4D3B2F,
    0xE9B5DBA58189DBBC, 0x3956C25BF348B538, 0x59F111F1B605D019,
    0x923F82A4AF194F9B, 0xAB1C5ED5DA6D8118, 0xD807AA98A3030242,
    0x12835B0145706FBE, 0x243185BE4EE4B28C, 0x550C7DC3D5FFB4E2,
    0x72BE5D74F27B896F, 0x80DEB1FE3B1696B1, 0x9BDC06A725C71235,
    0xC19BF174CF692694, 0xE49B69C19EF14AD2, 0xEFBE4786384F25E3,
    0x0FC19DC68B8CD5B5, 0x240CA1CC77AC9C65, 0x2DE92C6F592B0275,
    0x4A7484AA6EA6E483, 0x5CB0A9DCBD41FBD4, 0x76F988DA831153B5,
    0x983E5152EE66DFAB, 0xA831C66D2DB43210, 0xB00327C898FB213F,
    0xBF597FC7BEEF0EE4, 0xC6E00BF33DA88FC2, 0xD5A79147930AA725,
    0x06CA6351E003826F, 0x142929670A0E6E70, 0x27B70A8546D22FFC,
    0x2E1B21385C26C926, 0x4D2C6DFC5AC42AED, 0x53380D139D95B3DF,
    0x650A73548BAF63DE, 0x766A0ABB3C77B2A8, 0x81C2C92E47CBEEA8,
    0x92722C851482353B, 0xA2BFE8A14CF10364, 0xA81A664BBC423001,
    0xC24B8B70D0F89791, 0xC76C51A30654BE30, 0xD192E819D6EF5218,
    0xD69906245565A910, 0xF40E35855771202A, 0x106AA07032BBD1B8,
    0x19A4C116B8D2D0C8, 0x1E376C085141AB53, 0x2748774CDF8EEB99,
    0x34B0BCB5E19B48A8, 0x391C0CB3C5C95A63, 0x4ED8AA4AE3418ACB,
    0x5B9CCA4F7763E373, 0x682E6FF3D6B2B8A3, 0x748F82EE5DEFB2FC,
    0x78A5636F43172F60, 0x84C87814A1F0AB72, 0x8CC702081A6439EC,
    0x90BEFFFA23631E28, 0xA4506CEBDE82BDE9, 0xBEF9A3F7B2C67915,
    0xC67178F2E372532B, 0xCA273ECEEA26619C, 0xD186B8C721C0C207,
    0xEADA7DD6CDE0EB1E, 0xF57D4F7FEE6ED178, 0x06F067AA72176FBA,
    0x0A637DC5A2C898A6, 0x113F9804BEF90DAE, 0x1B710B35131C471B,
    0x28DB77F523047D84, 0x32CAAB7B40C72493, 0x3C9EBE0A15C9BEBC,
    0x431D67C49C100D4C, 0x4CC5D4BECB3E42B6, 0x597F299CFC657E2A,
    0x5FCB6FAB3AD6FAEC, 0x6C44198C4A475817
};

/* SHA-512 循环右移64位 */
#define SHA512_ROTR(x, n) (((x) >> (n)) | ((x) << (64 - (n))))
#define SHA512_SHR(x, n) ((x) >> (n))

/* SHA-512 Σ0, Σ1, σ0, σ1 */
#define SHA512_SIGMA0(x) (SHA512_ROTR(x, 28) ^ SHA512_ROTR(x, 34) ^ SHA512_ROTR(x, 39))
#define SHA512_SIGMA1(x) (SHA512_ROTR(x, 14) ^ SHA512_ROTR(x, 18) ^ SHA512_ROTR(x, 41))
#define SHA512_GAMMA0(x) (SHA512_ROTR(x, 1) ^ SHA512_ROTR(x, 8) ^ SHA512_SHR(x, 7))
#define SHA512_GAMMA1(x) (SHA512_ROTR(x, 19) ^ SHA512_ROTR(x, 61) ^ SHA512_SHR(x, 6))

/* SHA-512 Ch 和 Maj */
#define SHA512_CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define SHA512_MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

/* 初始化SHA512/256 */
void sha512_256_init(sha512_256_context_t *ctx) {
    memcpy(ctx->state, SHA512_256_INIT, sizeof(SHA512_256_INIT));
    ctx->count[0] = 0;
    ctx->count[1] = 0;
}

/* SHA512/256 块处理 */
static void sha512_256_process_block(const uint8_t *block, sha512_256_context_t *ctx) {
    uint64_t w[80];
    uint64_t a, b, c, d, e, f, g, h;
    uint64_t t1, t2;

    /* 展开消息到80个64位字 */
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint64_t)block[i * 8] << 56) |
               ((uint64_t)block[i * 8 + 1] << 48) |
               ((uint64_t)block[i * 8 + 2] << 40) |
               ((uint64_t)block[i * 8 + 3] << 32) |
               ((uint64_t)block[i * 8 + 4] << 24) |
               ((uint64_t)block[i * 8 + 5] << 16) |
               ((uint64_t)block[i * 8 + 6] << 8) |
               ((uint64_t)block[i * 8 + 7]);
    }
    for (int i = 16; i < 80; i++) {
        w[i] = SHA512_GAMMA1(w[i-2]) + w[i-7] + SHA512_GAMMA0(w[i-15]) + w[i-16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    /* 80轮压缩 */
    for (int i = 0; i < 80; i++) {
        t1 = h + SHA512_SIGMA1(e) + SHA512_CH(e, f, g) + SHA512_K[i] + w[i];
        t2 = SHA512_SIGMA0(a) + SHA512_MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    /* 更新状态 */
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

/* SHA512/256 更新 */
void sha512_256_update(sha512_256_context_t *ctx, const uint8_t *input, uint32_t len) {
    uint32_t index = (uint32_t)((ctx->count[0] >> 3) & 0x7F);
    uint32_t partlen = 128 - index;

    ctx->count[0] += (uint64_t)len << 3;
    if (ctx->count[0] < ((uint64_t)len << 3)) {
        ctx->count[1]++;
    }
    ctx->count[1] += (uint64_t)len >> 29;

    if (len >= partlen) {
        memcpy(ctx->buffer + index, input, partlen);
        sha512_256_process_block(ctx->buffer, ctx);

        for (uint32_t i = partlen; i + 127 < len; i += 128) {
            sha512_256_process_block(input + i, ctx);
        }
        index = 0;
    } else {
        index = 0;
    }

    memcpy(ctx->buffer + index, input, len);
}

/* SHA512/256 完成 */
void sha512_256_final(sha512_256_context_t *ctx, uint8_t digest[SHA512_HASH_SIZE]) {
    uint32_t index = (uint32_t)((ctx->count[0] >> 3) & 0x7F);
    uint32_t padlen = (index < 112) ? (112 - index) : (240 - index);

    /* 填充：1位 + 0位 */
    uint8_t padding[128];
    padding[0] = 0x80;
    memset(padding + 1, 0, padlen - 1);

    sha512_256_update(ctx, padding, padlen);

    /* 追加长度（128位） */
    uint8_t length[16];
    for (int i = 0; i < 8; i++) {
        length[i] = (uint8_t)(ctx->count[1] >> (56 - i * 8));
    }
    for (int i = 0; i < 8; i++) {
        length[i + 8] = (uint8_t)(ctx->count[0] >> (56 - i * 8));
    }
    sha512_256_update(ctx, length, 16);

    /* 输出摘要（只取前32字节） */
    for (int i = 0; i < 4; i++) {
        digest[i * 8]     = (uint8_t)(ctx->state[i] >> 56);
        digest[i * 8 + 1] = (uint8_t)(ctx->state[i] >> 48);
        digest[i * 8 + 2] = (uint8_t)(ctx->state[i] >> 40);
        digest[i * 8 + 3] = (uint8_t)(ctx->state[i] >> 32);
        digest[i * 8 + 4] = (uint8_t)(ctx->state[i] >> 24);
        digest[i * 8 + 5] = (uint8_t)(ctx->state[i] >> 16);
        digest[i * 8 + 6] = (uint8_t)(ctx->state[i] >> 8);
        digest[i * 8 + 7] = (uint8_t)(ctx->state[i]);
    }
}

/* 直接计算字符串的SHA512/256 */
void sha512_256_string(const char *str, uint8_t digest[SHA512_HASH_SIZE]) {
    sha512_256_context_t ctx;
    sha512_256_init(&ctx);
    sha512_256_update(&ctx, (const uint8_t *)str, strlen(str));
    sha512_256_final(&ctx, digest);
}

/* 直接计算数据的SHA512/256 */
void sha512_256_data(const void *data, uint32_t len, uint8_t digest[SHA512_HASH_SIZE]) {
    sha512_256_context_t ctx;
    sha512_256_init(&ctx);
    sha512_256_update(&ctx, (const uint8_t *)data, len);
    sha512_256_final(&ctx, digest);
}

/* SHA256 初始哈希值（前8个质数的平方根的小数部分） */
static const uint32_t SHA256_INIT[8] = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

/* SHA256 K常数（前64个质数的立方根的小数部分） */
static const uint32_t SHA256_K[64] = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};

/* SHA256 循环右移 */
#define SHA256_ROT_RIGHT(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define SHA256_CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define SHA256_MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SHA256_SIGMA0(x) (SHA256_ROT_RIGHT(x, 2) ^ SHA256_ROT_RIGHT(x, 13) ^ SHA256_ROT_RIGHT(x, 22))
#define SHA256_SIGMA1(x) (SHA256_ROT_RIGHT(x, 6) ^ SHA256_ROT_RIGHT(x, 11) ^ SHA256_ROT_RIGHT(x, 25))
#define SHA256_GAMMA0(x) (SHA256_ROT_RIGHT(x, 7) ^ SHA256_ROT_RIGHT(x, 18) ^ ((x) >> 3))
#define SHA256_GAMMA1(x) (SHA256_ROT_RIGHT(x, 17) ^ SHA256_ROT_RIGHT(x, 19) ^ ((x) >> 10))

/* SHA256 初始化 */
void sha256_init(sha256_context_t *ctx) {
    memcpy(ctx->state, SHA256_INIT, sizeof(SHA256_INIT));
    ctx->count = 0;
}

/* SHA256 块处理 */
static void sha256_process_block(const uint8_t *block, sha256_context_t *ctx) {
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t t1, t2;

    /* 展开消息到64个32位字 */
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               ((uint32_t)block[i * 4 + 3]);
    }
    for (int i = 16; i < 64; i++) {
        w[i] = SHA256_GAMMA1(w[i-2]) + w[i-7] + SHA256_GAMMA0(w[i-15]) + w[i-16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    /* 64轮压缩 */
    for (int i = 0; i < 64; i++) {
        t1 = h + SHA256_SIGMA1(e) + SHA256_CH(e, f, g) + SHA256_K[i] + w[i];
        t2 = SHA256_SIGMA0(a) + SHA256_MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    /* 更新状态 */
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

/* SHA256 更新 */
void sha256_update(sha256_context_t *ctx, const uint8_t *input, uint32_t len) {
    uint32_t index = (uint32_t)((ctx->count >> 3) & 0x3F);
    uint32_t partlen = 64 - index;

    ctx->count += (uint64_t)len << 3;

    if (len >= partlen) {
        memcpy(ctx->buffer + index, input, partlen);
        sha256_process_block(ctx->buffer, ctx);

        for (uint32_t i = partlen; i + 63 < len; i += 64) {
            sha256_process_block(input + i, ctx);
        }
        index = 0;
    } else {
        index = 0;
    }

    memcpy(ctx->buffer + index, input, len);
}

/* SHA256 完成 */
void sha256_final(sha256_context_t *ctx, uint8_t digest[SHA256_HASH_SIZE]) {
    uint32_t index = (uint32_t)((ctx->count >> 3) & 0x3F);
    uint32_t padlen = (index < 56) ? (56 - index) : (120 - index);

    /* 填充：1位 + 0位 */
    uint8_t padding[64];
    padding[0] = 0x80;
    memset(padding + 1, 0, padlen - 1);

    sha256_update(ctx, padding, padlen);

    /* 追加长度（64位） */
    uint8_t length[8];
    uint64_t bits = ctx->count;
    for (int i = 0; i < 8; i++) {
        length[i] = (uint8_t)(bits >> (56 - i * 8));
    }
    sha256_update(ctx, length, 8);

    /* 输出摘要 */
    for (int i = 0; i < 8; i++) {
        digest[i * 4]     = (uint8_t)(ctx->state[i] >> 24);
        digest[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        digest[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        digest[i * 4 + 3] = (uint8_t)(ctx->state[i]);
    }
}

/* 直接计算字符串的SHA256 */
void sha256_string(const char *str, uint8_t digest[SHA256_HASH_SIZE]) {
    sha256_context_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)str, strlen(str));
    sha256_final(&ctx, digest);
}

/* 直接计算数据的SHA256 */
void sha256_data(const void *data, uint32_t len, uint8_t digest[SHA256_HASH_SIZE]) {
    sha256_context_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)data, len);
    sha256_final(&ctx, digest);
}

/* ==========================================
 * 工具函数
 * ========================================== */

/* 将哈希值转换为十六进制字符串 */
void hash_to_string(const uint8_t *hash, int hash_len, char *output) {
    static const char hex_chars[] = "0123456789abcdef";

    for (int i = 0; i < hash_len; i++) {
        output[i * 2] = hex_chars[hash[i] >> 4];
        output[i * 2 + 1] = hex_chars[hash[i] & 0x0F];
    }
    output[hash_len * 2] = '\0';
}

/* 验证哈希值 */
int hash_verify(const uint8_t *hash, int hash_len, const char *expected_hex) {
    char actual_hex[65];
    hash_to_string(hash, hash_len, actual_hex);

    /* 简单比较 */
    for (int i = 0; i < hash_len * 2; i++) {
        if (actual_hex[i] != expected_hex[i]) {
            return 0;  /* 不匹配 */
        }
    }
    return 1;  /* 匹配 */
}
