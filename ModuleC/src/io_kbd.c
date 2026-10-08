/*
 * io_kbd.c —— 产品形态二：键盘方式的数据录入（ModuleC）
 *
 * 对应课程要求的第二种初始信息录入方式：程序运行时从键盘输入。
 * 该产品形态由编译期宏 LOAD_FROM_FILE 控制：
 *   - 定义 LOAD_FROM_FILE 时编译出 learning_platform_file，启动读数据文件；
 *   - 未定义时编译出 learning_platform_kbd，启动进入本文件的键盘录入流程。
 * 因此键盘版本的可执行文件中不包含任何读取初始数据的代码路径。
 *
 * 说明：键盘录入的用户密码同样是先计算 SHA-256 加盐哈希再入链表，
 * 内存与回写文件中都不会出现明文密码。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "link_pub.h"
#include "user_pub.h"
#include "course_pub.h"
#include "io_pub.h"
#include "sha256_pub.h"
#include "log_pub.h"

#define KBD_PWD_MAX     64
#define KBD_UID_MAX     128

/* 把 "1002,1003" 形式的学生名单解析成整型数组 */
static int parse_uid_array(const char *s, int *uids, int max, int *out_n)
{
    char buf[LINE_BUF_LEN];
    char *p;
    int   n = 0;

    if (s == NULL || uids == NULL || out_n == NULL || max <= 0) {
        return RES_ERR_PARAM;
    }
    *out_n = 0;
    snprintf(buf, sizeof(buf), "%s", s);

    p = buf;
    while (*p != '\0' && n < max) {
        char *comma = strchr(p, ',');

        if (comma != NULL) {
            *comma = '\0';
        }
        com_trim(p);
        if (*p != '\0') {
            uids[n++] = atoi(p);
        }
        if (comma == NULL) {
            break;
        }
        p = comma + 1;
    }
    *out_n = n;
    return RES_OK;
}

/* ------------------------------ 用户 ------------------------------ */

/* 读取除用户ID 之外的字段并新增用户 */
static int user_add_by_input(ListNode *head, int uid)
{
    char name[USER_NAME_LEN];
    char pwd[KBD_PWD_MAX];
    char role[16];
    User u;
    int  rc;

    if (com_read_str("  用户名: ", name, (int)sizeof(name)) != RES_OK || name[0] == '\0') {
        printf("  用户名不能为空，本用户取消录入。\n");
        return RES_ERR_PARAM;
    }
    if (com_read_str("  密码: ", pwd, (int)sizeof(pwd)) != RES_OK || pwd[0] == '\0') {
        printf("  密码不能为空，本用户取消录入。\n");
        return RES_ERR_PARAM;
    }
    if (com_read_str("  角色(student/teacher): ", role, (int)sizeof(role)) != RES_OK) {
        printf("  角色输入失败，本用户取消录入。\n");
        return RES_ERR_PARAM;
    }

    memset(&u, 0, sizeof(u));
    u.uid = uid;
    snprintf(u.username, sizeof(u.username), "%s", name);
    hash_password(u.username, pwd, u.password_hash);
    if (user_role_from_name(role, &u.role) != RES_OK) {
        u.role = ROLE_STUDENT;
        printf("  角色取值无效，按 student 处理。\n");
    }
    com_now(u.reg_time, (int)sizeof(u.reg_time));

    rc = user_add(head, &u);
    if (rc == RES_OK) {
        printf("  已新增用户 %d(%s)，密码已按 SHA-256 加盐哈希存储。\n", u.uid, u.username);
    } else if (rc == RES_ERR_EXIST) {
        printf("  新增失败：用户ID %d 已存在。\n", u.uid);
    } else if (rc == RES_ERR_DUP) {
        printf("  新增失败：用户名 %s 已存在。\n", u.username);
    } else {
        printf("  新增失败：返回码 %d。\n", rc);
    }
    return rc;
}

int io_input_user(ListNode *head)
{
    int uid;

    if (head == NULL) {
        return RES_ERR_PARAM;
    }
    printf("\n---- 新增一个用户 ----\n");
    if (com_read_int("  用户ID: ", &uid) != RES_OK) {
        printf("  用户ID 输入无效，已取消。\n");
        return RES_ERR_PARAM;
    }
    return user_add_by_input(head, uid);
}

int io_input_users(ListNode *head, int *count)
{
    int   n = 0;
    char  buf[LINE_BUF_LEN];
    int   uid;

    if (head == NULL) {
        return RES_ERR_PARAM;
    }
    if (count != NULL) {
        *count = 0;
    }

    printf("\n---- 键盘录入用户信息 ----\n");
    printf("提示：直接回车表示结束录入。\n");

    for (;;) {
        printf("\n[第 %d 条] 用户ID（直接回车结束录入）: ", n + 1);
        if (com_read_line(buf, (int)sizeof(buf)) < 0 || buf[0] == '\0') {
            break;
        }
        if (sscanf(buf, "%d", &uid) != 1) {
            printf("  用户ID 输入无效，请重新输入。\n");
            continue;
        }
        if (user_add_by_input(head, uid) == RES_OK) {
            n++;
        }
    }

    printf("用户信息录入结束，本次新增 %d 条。\n", n);
    if (count != NULL) {
        *count = n;
    }
    return RES_OK;
}

/* ------------------------------ 课程 ------------------------------ */

/* 读取除课程ID 之外的字段并新增课程（含可选的已选学生名单） */
static int course_add_by_input(ListNode *head, int cid)
{
    char    name[COURSE_NAME_LEN];
    char    intro[COURSE_INTRO_LEN];
    char    status[16];
    char    students[LINE_BUF_LEN];
    char    yes[16];
    int     teacher_id = 0;
    int     credit = 0;
    int     hours = 0;
    int     capacity = 0;
    Course  c;
    int     rc;

    if (com_read_str("  课程名称: ", name, (int)sizeof(name)) != RES_OK || name[0] == '\0') {
        printf("  课程名称不能为空，本课程取消录入。\n");
        return RES_ERR_PARAM;
    }
    if (com_read_int("  授课教师ID: ", &teacher_id) != RES_OK) {
        teacher_id = 0;
    }
    if (com_read_str("  课程简介: ", intro, (int)sizeof(intro)) != RES_OK) {
        intro[0] = '\0';
    }
    if (com_read_int("  学分: ", &credit) != RES_OK) {
        credit = 0;
    }
    if (com_read_int("  授课学时: ", &hours) != RES_OK) {
        hours = 0;
    }
    if (com_read_int("  课程人数上限: ", &capacity) != RES_OK) {
        capacity = 0;
    }
    if (com_read_str("  课程状态(published/unpublished，回车默认 published): ",
                     status, (int)sizeof(status)) != RES_OK) {
        status[0] = '\0';
    }

    memset(&c, 0, sizeof(c));
    c.cid = cid;
    snprintf(c.name, sizeof(c.name), "%s", name);
    c.teacher_id = teacher_id;
    snprintf(c.intro, sizeof(c.intro), "%s", intro);
    c.credit = credit;
    c.hours = hours;
    c.capacity = capacity;
    if (course_status_from_name(status, &c.status) != RES_OK) {
        c.status = COURSE_PUBLISHED;
    }

    rc = course_add(head, &c);
    if (rc == RES_OK) {
        printf("  已新增课程 %d(%s)。\n", cid, name);
    } else if (rc == RES_ERR_EXIST) {
        printf("  新增失败：课程ID %d 已存在。\n", cid);
        return rc;
    } else {
        printf("  新增失败：返回码 %d。\n", rc);
        return rc;
    }

    /* 可选：录入已选学生名单 */
    if (com_read_str("  是否录入已选学生名单？(y/N): ", yes, (int)sizeof(yes)) != RES_OK) {
        yes[0] = '\0';
    }
    if (yes[0] == 'y' || yes[0] == 'Y') {
        int uids[KBD_UID_MAX];
        int cnt = 0;

        if (com_read_str("  已选学生ID（多个用英文逗号分隔）: ",
                         students, (int)sizeof(students)) == RES_OK &&
            students[0] != '\0') {
            if (parse_uid_array(students, uids, KBD_UID_MAX, &cnt) == RES_OK && cnt > 0) {
                rc = course_set_students(head, cid, uids, cnt);
                if (rc == RES_OK) {
                    printf("  已登记 %d 名学生选课。\n", cnt);
                } else if (rc == RES_ERR_FULL) {
                    printf("  登记失败：学生人数超出课程人数上限。\n");
                } else {
                    printf("  登记失败：返回码 %d。\n", rc);
                }
            }
        }
    }
    return RES_OK;
}

int io_input_course(ListNode *head)
{
    int cid;

    if (head == NULL) {
        return RES_ERR_PARAM;
    }
    printf("\n---- 新增一门课程 ----\n");
    if (com_read_int("  课程ID: ", &cid) != RES_OK) {
        printf("  课程ID 输入无效，已取消。\n");
        return RES_ERR_PARAM;
    }
    return course_add_by_input(head, cid);
}

int io_input_courses(ListNode *head, int *count)
{
    int  n = 0;
    char buf[LINE_BUF_LEN];
    int  cid;

    if (head == NULL) {
        return RES_ERR_PARAM;
    }
    if (count != NULL) {
        *count = 0;
    }

    printf("\n---- 键盘录入课程信息 ----\n");
    printf("提示：直接回车表示结束录入。\n");

    for (;;) {
        printf("\n[第 %d 条] 课程ID（直接回车结束录入）: ", n + 1);
        if (com_read_line(buf, (int)sizeof(buf)) < 0 || buf[0] == '\0') {
            break;
        }
        if (sscanf(buf, "%d", &cid) != 1) {
            printf("  课程ID 输入无效，请重新输入。\n");
            continue;
        }
        if (course_add_by_input(head, cid) == RES_OK) {
            n++;
        }
    }

    printf("课程信息录入结束，本次新增 %d 条。\n", n);
    if (count != NULL) {
        *count = n;
    }
    return RES_OK;
}
