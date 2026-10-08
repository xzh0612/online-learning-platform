/*
 * sha256.c —— SHA-256 摘要与密码哈希实现（ModuleC）
 *
 * 本工程自带一份标准 SHA-256 实现，不依赖系统 crypt 库或任何第三方库，
 * 以保证在 macOS（clang）与 Linux（gcc）上行为一致、编译方式统一。
 *
 * 密码哈希规则：sha256(用户名 + ":" + 明文密码)，
 * 同一明文在不同用户名下得到不同摘要，避免相同口令在数据文件中同文。
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "sha256_pub.h"

/* SHA-256 轮常量 */
static const unsigned int sha256_k[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

/* 初始哈希值 */
static const unsigned int sha256_h0[8] = {
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
};

#define ROTR(x, n)  (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)       (ROTR(x, 2)  ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x)       (ROTR(x, 6)  ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SG0(x)       (ROTR(x, 7)  ^ ROTR(x, 18) ^ ((x) >> 3))
#define SG1(x)       (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

/* SHA-256 上下文 */
typedef struct {
    unsigned int       state[8];
    unsigned long long bitlen;
    unsigned char      data[64];
    unsigned int       datalen;
} Sha256Ctx;

/* 处理一个 64 字节数据块 */
static void sha256_transform(Sha256Ctx *ctx, const unsigned char *block)
{
    unsigned int i;
    unsigned int m[64];
    unsigned int a, b, c, d, e, f, g, h;
    unsigned int t1, t2;

    /* 填充分组 */
    for (i = 0; i < 16; i++) {
        m[i] = ((unsigned int)block[i * 4]     << 24) |
               ((unsigned int)block[i * 4 + 1] << 16) |
               ((unsigned int)block[i * 4 + 2] << 8)  |
               ((unsigned int)block[i * 4 + 3]);
    }
    for (i = 16; i < 64; i++) {
        m[i] = SG1(m[i - 2]) + m[i - 7] + SG0(m[i - 15]) + m[i - 16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + sha256_k[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

/* 初始化上下文 */
static void sha256_init(Sha256Ctx *ctx)
{
    memcpy(ctx->state, sha256_h0, sizeof(ctx->state));
    ctx->bitlen = 0;
    ctx->datalen = 0;
}

/* 追加数据 */
static void sha256_update(Sha256Ctx *ctx, const unsigned char *data, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++) {
        ctx->data[ctx->datalen++] = data[i];
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

/* 收尾并输出 32 字节摘要 */
static void sha256_final(Sha256Ctx *ctx, unsigned char *digest)
{
    unsigned int i;

    i = ctx->datalen;

    /* 补 0x80，后面补 0 到 56 字节，最后 8 字节为长度 */
    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56) {
            ctx->data[i++] = 0x00;
        }
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64) {
            ctx->data[i++] = 0x00;
        }
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }

    ctx->bitlen += (unsigned long long)ctx->datalen * 8;
    ctx->data[56] = (unsigned char)(ctx->bitlen >> 56);
    ctx->data[57] = (unsigned char)(ctx->bitlen >> 48);
    ctx->data[58] = (unsigned char)(ctx->bitlen >> 40);
    ctx->data[59] = (unsigned char)(ctx->bitlen >> 32);
    ctx->data[60] = (unsigned char)(ctx->bitlen >> 24);
    ctx->data[61] = (unsigned char)(ctx->bitlen >> 16);
    ctx->data[62] = (unsigned char)(ctx->bitlen >> 8);
    ctx->data[63] = (unsigned char)(ctx->bitlen);
    sha256_transform(ctx, ctx->data);

    for (i = 0; i < 4; i++) {
        digest[i]      = (unsigned char)(ctx->state[0] >> (24 - i * 8));
        digest[i + 4]  = (unsigned char)(ctx->state[1] >> (24 - i * 8));
        digest[i + 8]  = (unsigned char)(ctx->state[2] >> (24 - i * 8));
        digest[i + 12] = (unsigned char)(ctx->state[3] >> (24 - i * 8));
        digest[i + 16] = (unsigned char)(ctx->state[4] >> (24 - i * 8));
        digest[i + 20] = (unsigned char)(ctx->state[5] >> (24 - i * 8));
        digest[i + 24] = (unsigned char)(ctx->state[6] >> (24 - i * 8));
        digest[i + 28] = (unsigned char)(ctx->state[7] >> (24 - i * 8));
    }
}

/* 计算 SHA-256 并输出 64 位小写十六进制字符串 */
void sha256_hex(const unsigned char *data, size_t len, char *out)
{
    static const char hex[] = "0123456789abcdef";
    Sha256Ctx     ctx;
    unsigned char digest[32];
    int           i;

    if (out == NULL) {
        return;
    }
    sha256_init(&ctx);
    if (data != NULL && len > 0) {
        sha256_update(&ctx, data, len);
    }
    sha256_final(&ctx, digest);

    for (i = 0; i < 32; i++) {
        out[i * 2]     = hex[(digest[i] >> 4) & 0x0F];
        out[i * 2 + 1] = hex[digest[i] & 0x0F];
    }
    out[64] = '\0';
}

/* 密码哈希：加盐后求摘要 */
int hash_password(const char *username, const char *plain, char *out)
{
    char salted[USER_NAME_LEN + 128];

    if (username == NULL || plain == NULL || out == NULL) {
        return RES_ERR_PARAM;
    }
    snprintf(salted, sizeof(salted), "%s:%s", username, plain);
    sha256_hex((const unsigned char *)salted, strlen(salted), out);
    return RES_OK;
}

/* 校验明文密码与给定哈希是否匹配 */
int hash_password_match(const char *username, const char *plain, const char *expect_hash)
{
    char calc[USER_HASH_LEN];

    if (username == NULL || plain == NULL || expect_hash == NULL) {
        return 0;
    }
    if (hash_password(username, plain, calc) != RES_OK) {
        return 0;
    }
    return (strcmp(calc, expect_hash) == 0) ? 1 : 0;
}
