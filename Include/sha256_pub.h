#ifndef _SHA256_PUB_H_
#define _SHA256_PUB_H_

/*
 * sha256_pub.h —— SHA-256 摘要与密码哈希接口（ModuleC 实现）
 *
 * 课程要求：用户密码采用哈希存储。
 * 本工程自带一份纯 C 实现的 SHA-256，不依赖系统 crypt 库，
 * 以保证在 macOS（clang）与 Linux（gcc）上行为一致、编译方式统一。
 *
 * 加盐规则：hash_password() 对「用户名 + ':' + 明文密码」求摘要，
 * 使得同一明文在不同用户名下得到不同哈希，避免相同口令在数据文件中同文。
 */

#include "common.h"

/* 计算 SHA-256 摘要，结果写入 64 位小写十六进制字符串（out 至少 65 字节） */
void sha256_hex(const unsigned char *data, size_t len, char *out);

/* 密码哈希：加盐后求摘要，结果写入 out（至少 USER_HASH_LEN 字节）；成功返回 RES_OK */
int hash_password(const char *username, const char *plain, char *out);

/* 校验明文密码与给定哈希是否匹配：匹配返回 1，不匹配返回 0 */
int hash_password_match(const char *username, const char *plain, const char *expect_hash);

#endif /* _SHA256_PUB_H_ */
