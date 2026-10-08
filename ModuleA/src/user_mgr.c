/*
 * user_mgr.c —— 用户管理模块核心实现（ModuleA）
 *
 * 按 user_pub.h 的约定实现用户信息的增、删、改、查：
 *   - 查：按用户ID、按用户名查询，按角色筛选列表
 *   - 密码：本模块只做哈希值的保存与比对，哈希计算由调用方先行完成
 *     （接口见 sha256_pub.h 的 hash_password），因此本模块不依赖 ModuleC。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "link_pub.h"
#include "user_pub.h"

/* ------------------------- 内部比较函数 ------------------------- */

/* 按用户ID比较：命中返回 0 */
static int user_cmp_id(const void *data, const void *key)
{
    const User *u  = (const User *)data;
    const int  *id = (const int *)key;

    if (u == NULL || id == NULL) {
        return -1;
    }
    return (u->uid == *id) ? 0 : 1;
}

/* 按用户名比较：命中返回 0 */
static int user_cmp_name(const void *data, const void *key)
{
    const User        *u    = (const User *)data;
    const char *const *name = (const char *const *)key;

    if (u == NULL || name == NULL || *name == NULL) {
        return -1;
    }
    return (strcmp(u->username, *name) == 0) ? 0 : 1;
}

/* 按角色比较：命中返回 0 */
static int user_cmp_role(const void *data, const void *key)
{
    const User     *u = (const User *)data;
    const RoleType *r = (const RoleType *)key;

    if (u == NULL || r == NULL) {
        return -1;
    }
    return (u->role == *r) ? 0 : 1;
}

/* ------------------------------ 查 ------------------------------ */

User *user_find_by_id(ListNode *head, int uid)
{
    ListNode *node = list_find(head, user_cmp_id, &uid);

    return (node == NULL) ? NULL : (User *)node->data;
}

User *user_find_by_name(ListNode *head, const char *name)
{
    ListNode *node;

    if (name == NULL) {
        return NULL;
    }
    node = list_find(head, user_cmp_name, &name);
    return (node == NULL) ? NULL : (User *)node->data;
}

/* 统计某角色的用户数量 */
int user_count_by_role(ListNode *head, RoleType role)
{
    ListNode *p;
    int       n = 0;

    if (head == NULL) {
        return 0;
    }
    for (p = head->next; p != NULL; p = p->next) {
        if (user_cmp_role(p->data, &role) == 0) {
            n++;
        }
    }
    return n;
}

/*
 * 按角色筛选用户列表。
 * 返回一条新建的链表（元素为 User 的副本），调用方用 list_destroy(&list, user_free) 释放。
 */
ListNode *user_filter_by_role(ListNode *head, RoleType role)
{
    ListNode *result;
    ListNode *p;

    if (head == NULL) {
        return NULL;
    }
    result = list_create();
    if (result == NULL) {
        return NULL;
    }

    for (p = head->next; p != NULL; p = p->next) {
        const User *u = (const User *)p->data;
        User       *copy;

        if (user_cmp_role(p->data, &role) != 0) {
            continue;
        }
        copy = (User *)malloc(sizeof(User));
        if (copy == NULL) {
            list_destroy(&result, user_free);
            return NULL;
        }
        *copy = *u;
        if (list_append(result, copy) != RES_OK) {
            free(copy);
            list_destroy(&result, user_free);
            return NULL;
        }
    }
    return result;
}

/* 校验密码：password_hash 为调用方用 hash_password() 计算好的哈希值 */
int user_verify_password(ListNode *head, int uid, const char *password_hash)
{
    User *u = user_find_by_id(head, uid);

    if (u == NULL || password_hash == NULL) {
        return 0;
    }
    return (strcmp(u->password_hash, password_hash) == 0) ? 1 : 0;
}
