#ifndef _USER_PUB_H_
#define _USER_PUB_H_

/*
 * user_pub.h —— 用户管理模块（ModuleA）对外接口
 *
 * 覆盖课程要求中的用户管理功能：
 *   新增用户信息、修改用户信息、删除用户信息、
 *   按用户ID/用户名查看并修改某用户详细信息、按角色筛选用户列表。
 *
 * 依赖：link_pub.h（公共双向链表）、common.h（通用宏与工具）。
 * 注意：本模块只负责数据的保存与校验，密码哈希由调用方通过
 *       sha256_pub.h 的 hash_password() 先行计算后再传入。
 */

#include "common.h"
#include "link_pub.h"

/* 用户角色 */
typedef enum {
    ROLE_STUDENT = 0,   /* 学生 */
    ROLE_TEACHER = 1    /* 教师 */
} RoleType;

/* 用户信息 */
typedef struct {
    int      uid;                          /* 用户ID */
    char     username[USER_NAME_LEN];      /* 用户名 */
    char     password_hash[USER_HASH_LEN]; /* 密码（SHA-256 哈希，十六进制字符串） */
    RoleType role;                         /* 角色：学生 / 教师 */
    char     reg_time[USER_TIME_LEN];      /* 注册时间 "YYYY-MM-DD HH:MM:SS" */
} User;

/* ------------------------------ 增 ------------------------------ */
/* 新增用户：uid 重复返回 RES_ERR_EXIST，用户名重复返回 RES_ERR_DUP */
int user_add(ListNode *head, const User *u);

/* ------------------------------ 删 ------------------------------ */
/* 按用户ID删除用户；不存在返回 RES_ERR_NOTFOUND */
int user_delete(ListNode *head, int uid);

/* ------------------------------ 改 ------------------------------ */
/* 按用户ID修改用户信息（uid 不可改，其余字段以 newinfo 覆盖） */
int user_modify(ListNode *head, int uid, const User *newinfo);

/* ------------------------------ 查 ------------------------------ */
User *user_find_by_id(ListNode *head, int uid);
User *user_find_by_name(ListNode *head, const char *name);

/* 按角色筛选用户列表，返回新建的链表（调用方用 list_destroy 释放） */
ListNode *user_filter_by_role(ListNode *head, RoleType role);

/* 统计某角色的用户数 */
int user_count_by_role(ListNode *head, RoleType role);

/* 校验密码：password_hash 为调用方先用 hash_password() 算好的哈希 */
int user_verify_password(ListNode *head, int uid, const char *password_hash);

/* -------------------------- 展示与释放 -------------------------- */
/* 角色与字符串互转（用于数据文件与界面展示） */
const char *user_role_name(RoleType role);
int user_role_from_name(const char *name, RoleType *out);

void user_print_header(void);
void user_print(const User *u);
void user_print_list(ListNode *head);

/* 供 list_clear / list_destroy 使用的释放回调 */
void user_free(void *data);

#endif /* _USER_PUB_H_ */
