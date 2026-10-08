/*
 * user_util.c —— 用户管理模块的展示与释放工具（ModuleA）
 *
 * 集中实现角色枚举与字符串的互转、用户信息的格式化输出，
 * 以及供 list_clear / list_destroy 使用的释放回调。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "link_pub.h"
#include "user_pub.h"

/* 数据文件与日志中使用的角色名（与 course 模块、日志脚本约定一致） */
const char *user_role_name(RoleType role)
{
    return (role == ROLE_TEACHER) ? ROLE_NAME_TEACHER : ROLE_NAME_STUDENT;
}

/* 由字符串还原角色枚举；成功返回 RES_OK */
int user_role_from_name(const char *name, RoleType *out)
{
    if (name == NULL || out == NULL) {
        return RES_ERR_PARAM;
    }
    if (strcmp(name, ROLE_NAME_TEACHER) == 0) {
        *out = ROLE_TEACHER;
        return RES_OK;
    }
    if (strcmp(name, ROLE_NAME_STUDENT) == 0) {
        *out = ROLE_STUDENT;
        return RES_OK;
    }
    return RES_ERR_PARAM;
}

/* 表头分隔线 */
static void user_print_sep(void)
{
    printf("------  ------------------  --------  -------------------  ------------------\n");
}

void user_print_header(void)
{
    printf("%-6s  %-18s  %-8s  %-19s  %-18s\n",
           "UserID", "UserName", "Role", "RegTime", "PasswordHash");
    user_print_sep();
}

void user_print(const User *u)
{
    char hash[20];

    /* 只展示哈希前 16 位，避免表格过宽，同时体现密码为哈希存储 */
    memset(hash, 0, sizeof(hash));
    strncpy(hash, u->password_hash, sizeof(hash) - 1);

    printf("%-6d  %-18s  %-8s  %-19s  %s...\n",
           u->uid,
           u->username,
           user_role_name(u->role),
           u->reg_time,
           hash);
}

void user_print_list(ListNode *head)
{
    ListNode *p;

    if (head == NULL) {
        printf("[用户列表] 链表未初始化\n");
        return;
    }

    user_print_header();
    for (p = head->next; p != NULL; p = p->next) {
        user_print((const User *)p->data);
    }
    user_print_sep();
    printf("共 %d 条用户记录\n", list_count(head));
}

/* 释放一个用户结构体，供 list_clear / list_destroy 使用 */
void user_free(void *data)
{
    if (data != NULL) {
        free(data);
    }
}
