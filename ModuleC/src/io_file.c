/*
 * io_file.c —— 产品形态一：文件方式的数据载入与回写（ModuleC）
 *
 * 数据文件格式（每行一条记录，字段以 '|' 分隔，以 '#' 开头的行为注释）：
 *
 *   user.txt    uid|username|password_hash|role|reg_time
 *   course.txt  cid|name|teacher_id|intro|credit|hours|capacity|status|student_uids
 *
 * 说明：
 *   1. 用户密码在数据文件中即为 SHA-256 哈希值，不含明文；
 *   2. 课程已选人数 enrolled 不落盘，由 student_uids 字段推导，
 *      保证「人数」与「已选学生列表」只有一处数据源；
 *   3. 程序运行中通过命令行修改信息后，由 Main.c 调用 io_save_* 整表回写，
 *      并把回写动作记入日志。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "link_pub.h"
#include "user_pub.h"
#include "course_pub.h"
#include "io_pub.h"
#include "log_pub.h"

/* 把 "1002,1003" 这样的字段解析成 int* 链表；s 为空表示没有学生选课 */
static int parse_uid_list(const char *s, ListNode **out_list)
{
    ListNode *lst;
    char      buf[LINE_BUF_LEN];
    char     *p;

    if (out_list == NULL) {
        return RES_ERR_PARAM;
    }
    *out_list = NULL;

    lst = list_create();
    if (lst == NULL) {
        return RES_ERR_MEMORY;
    }
    if (s == NULL || s[0] == '\0') {
        *out_list = lst;
        return RES_OK;
    }

    snprintf(buf, sizeof(buf), "%s", s);
    p = buf;
    while (*p != '\0') {
        char *comma = strchr(p, ',');
        int  *puid;

        if (comma != NULL) {
            *comma = '\0';
        }
        com_trim(p);
        if (*p != '\0') {
            puid = (int *)malloc(sizeof(int));
            if (puid == NULL) {
                list_destroy(&lst, free);
                return RES_ERR_MEMORY;
            }
            *puid = atoi(p);
            if (list_append(lst, puid) != RES_OK) {
                free(puid);
                list_destroy(&lst, free);
                return RES_ERR_MEMORY;
            }
        }
        if (comma == NULL) {
            break;
        }
        p = comma + 1;
    }

    *out_list = lst;
    return RES_OK;
}

/* 单行解析：用户 */
int io_parse_user_line(const char *line, User *out)
{
    char  buf[LINE_BUF_LEN];
    char *f[5];
    int   n;

    if (line == NULL || out == NULL) {
        return RES_ERR_PARAM;
    }
    snprintf(buf, sizeof(buf), "%s", line);
    n = com_split(buf, FIELD_SEP, f, 5);
    if (n < 5) {
        return RES_ERR_PARAM;
    }

    memset(out, 0, sizeof(*out));
    com_trim(f[0]);
    com_trim(f[1]);
    com_trim(f[2]);
    com_trim(f[3]);
    com_trim(f[4]);

    out->uid = atoi(f[0]);
    snprintf(out->username, sizeof(out->username), "%s", f[1]);
    snprintf(out->password_hash, sizeof(out->password_hash), "%s", f[2]);
    if (user_role_from_name(f[3], &out->role) != RES_OK) {
        return RES_ERR_PARAM;
    }
    snprintf(out->reg_time, sizeof(out->reg_time), "%s", f[4]);
    return RES_OK;
}

/* 单行解析：课程（含已选学生列表） */
int io_parse_course_line(const char *line, Course *out)
{
    char  buf[LINE_BUF_LEN];
    char *f[9];
    int   n;

    if (line == NULL || out == NULL) {
        return RES_ERR_PARAM;
    }
    snprintf(buf, sizeof(buf), "%s", line);
    n = com_split(buf, FIELD_SEP, f, 9);
    if (n < 9) {
        return RES_ERR_PARAM;
    }

    memset(out, 0, sizeof(*out));
    com_trim(f[0]);
    com_trim(f[1]);
    com_trim(f[2]);
    com_trim(f[3]);
    com_trim(f[4]);
    com_trim(f[5]);
    com_trim(f[6]);
    com_trim(f[7]);
    com_trim(f[8]);

    out->cid        = atoi(f[0]);
    snprintf(out->name, sizeof(out->name), "%s", f[1]);
    out->teacher_id = atoi(f[2]);
    snprintf(out->intro, sizeof(out->intro), "%s", f[3]);
    out->credit     = atoi(f[4]);
    out->hours      = atoi(f[5]);
    out->capacity   = atoi(f[6]);
    if (course_status_from_name(f[7], &out->status) != RES_OK) {
        return RES_ERR_PARAM;
    }

    if (parse_uid_list(f[8], &out->student_list) != RES_OK) {
        return RES_ERR_MEMORY;
    }
    out->enrolled = list_count(out->student_list);
    return RES_OK;
}

/* ---------------------- 载入 ---------------------- */

int io_load_users(const char *path, ListNode *head)
{
    FILE *fp;
    char  line[LINE_BUF_LEN];
    char  detail[LOG_DETAIL_LEN];
    int   n = 0;

    if (path == NULL || head == NULL) {
        return RES_ERR_PARAM;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        return RES_ERR_IO;
    }

    while (fgets(line, (int)sizeof(line), fp) != NULL) {
        User u;

        com_trim(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        if (io_parse_user_line(line, &u) != RES_OK) {
            continue;
        }
        if (user_add(head, &u) == RES_OK) {
            n++;
        }
    }
    fclose(fp);

    snprintf(detail, sizeof(detail), "从 %s 载入 %d 条用户信息", path, n);
    log_record(LOG_DATA_LOAD, ROLE_NAME_ADMIN, "all", ROLE_NAME_ADMIN, detail);
    return n;
}

int io_load_courses(const char *path, ListNode *head)
{
    FILE *fp;
    char  line[LINE_BUF_LEN];
    char  detail[LOG_DETAIL_LEN];
    int   n = 0;

    if (path == NULL || head == NULL) {
        return RES_ERR_PARAM;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        return RES_ERR_IO;
    }

    while (fgets(line, (int)sizeof(line), fp) != NULL) {
        Course *copy;
        Course  c;

        com_trim(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        if (io_parse_course_line(line, &c) != RES_OK) {
            continue;
        }

        /* 课程号重复的行直接丢弃，避免链表内出现两条同号课程 */
        if (course_find_by_id(head, c.cid) != NULL) {
            list_destroy(&c.student_list, free);
            continue;
        }

        copy = (Course *)malloc(sizeof(Course));
        if (copy == NULL) {
            list_destroy(&c.student_list, free);
            break;
        }
        *copy = c;
        if (list_append(head, copy) != RES_OK) {
            course_free(copy);
            break;
        }
        n++;
    }
    fclose(fp);

    snprintf(detail, sizeof(detail), "从 %s 载入 %d 条课程信息", path, n);
    log_record(LOG_DATA_LOAD, ROLE_NAME_ADMIN, "all", ROLE_NAME_ADMIN, detail);
    return n;
}

/* ---------------------- 回写 ---------------------- */

int io_save_users(const char *path, ListNode *head)
{
    FILE     *fp;
    ListNode *p;
    char      detail[LOG_DETAIL_LEN];
    int       n = 0;

    if (path == NULL || head == NULL) {
        return RES_ERR_PARAM;
    }
    fp = fopen(path, "w");
    if (fp == NULL) {
        return RES_ERR_IO;
    }

    fprintf(fp, "# 用户信息：uid|username|password_hash|role|reg_time\n");
    for (p = head->next; p != NULL; p = p->next) {
        const User *u = (const User *)p->data;

        fprintf(fp, "%d|%s|%s|%s|%s\n",
                u->uid, u->username, u->password_hash,
                user_role_name(u->role), u->reg_time);
        n++;
    }
    fclose(fp);

    snprintf(detail, sizeof(detail), "回写 %d 条用户信息到 %s", n, path);
    log_record(LOG_DATA_SAVE, ROLE_NAME_ADMIN, "all", ROLE_NAME_ADMIN, detail);
    return RES_OK;
}

int io_save_courses(const char *path, ListNode *head)
{
    FILE     *fp;
    ListNode *p;
    char      detail[LOG_DETAIL_LEN];
    int       n = 0;

    if (path == NULL || head == NULL) {
        return RES_ERR_PARAM;
    }
    fp = fopen(path, "w");
    if (fp == NULL) {
        return RES_ERR_IO;
    }

    fprintf(fp, "# 课程信息：cid|name|teacher_id|intro|credit|hours|capacity|status|student_uids\n");
    for (p = head->next; p != NULL; p = p->next) {
        const Course *c = (const Course *)p->data;
        ListNode     *q;
        int           first = 1;

        fprintf(fp, "%d|%s|%d|%s|%d|%d|%d|%s|",
                c->cid, c->name, c->teacher_id, c->intro,
                c->credit, c->hours, c->capacity,
                course_status_name(c->status));

        for (q = (c->student_list == NULL) ? NULL : c->student_list->next;
             q != NULL; q = q->next) {
            fprintf(fp, "%s%d", first ? "" : ",", *(int *)q->data);
            first = 0;
        }
        fprintf(fp, "\n");
        n++;
    }
    fclose(fp);

    snprintf(detail, sizeof(detail), "回写 %d 条课程信息到 %s", n, path);
    log_record(LOG_DATA_SAVE, ROLE_NAME_ADMIN, "all", ROLE_NAME_ADMIN, detail);
    return RES_OK;
}
