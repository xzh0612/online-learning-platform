/*
 * course_mgr.c —— 课程管理模块核心实现（ModuleB）
 *
 * 按 course_pub.h 的约定实现课程信息的增、删、改、查，
 * 以及「已选学生列表」的维护（选课 / 退课 / 整表设置）。
 *
 * 依赖说明：本模块使用 ModuleA 提供的公共双向链表（link_pub.h）来维护
 * 已选学生列表，不直接依赖用户管理模块的内部实现；课程中只保存
 * 授课教师ID 与学生 uid，不使用对方的结构体指针，避免循环依赖。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "link_pub.h"
#include "course_pub.h"

/* ------------------------- 内部比较函数 ------------------------- */

/* 按课程ID比较：命中返回 0 */
static int course_cmp_id(const void *data, const void *key)
{
    const Course *c  = (const Course *)data;
    const int    *id = (const int *)key;

    if (c == NULL || id == NULL) {
        return -1;
    }
    return (c->cid == *id) ? 0 : 1;
}

/* 按学生 uid 比较（用于已选学生列表）：命中返回 0 */
static int course_cmp_uid(const void *data, const void *key)
{
    const int *a = (const int *)data;
    const int *b = (const int *)key;

    if (a == NULL || b == NULL) {
        return -1;
    }
    return (*a == *b) ? 0 : 1;
}

/* ------------------------------ 查 ------------------------------ */

Course *course_find_by_id(ListNode *head, int cid)
{
    ListNode *node = list_find(head, course_cmp_id, &cid);

    return (node == NULL) ? NULL : (Course *)node->data;
}

/* ------------------------------ 增 ------------------------------ */

int course_add(ListNode *head, const Course *c)
{
    Course *copy;

    if (head == NULL || c == NULL) {
        return RES_ERR_PARAM;
    }
    if (c->name[0] == '\0') {
        return RES_ERR_PARAM;
    }
    if (course_find_by_id(head, c->cid) != NULL) {
        return RES_ERR_EXIST;
    }

    copy = (Course *)malloc(sizeof(Course));
    if (copy == NULL) {
        return RES_ERR_MEMORY;
    }
    *copy = *c;

    /* 新课程从零开始：已选人数为 0，已选学生列表为空链表 */
    copy->student_list = list_create();
    if (copy->student_list == NULL) {
        free(copy);
        return RES_ERR_MEMORY;
    }
    copy->enrolled = 0;

    if (list_append(head, copy) != RES_OK) {
        list_destroy(&copy->student_list, free);
        free(copy);
        return RES_ERR_MEMORY;
    }
    return RES_OK;
}

/* ------------------------------ 删 ------------------------------ */

int course_delete(ListNode *head, int cid)
{
    ListNode *node;
    Course   *c;

    if (head == NULL) {
        return RES_ERR_PARAM;
    }
    node = list_find(head, course_cmp_id, &cid);
    if (node == NULL) {
        return RES_ERR_NOTFOUND;
    }

    /* 先释放课程及其已选学生列表，再由链表删除并释放节点本身 */
    c = (Course *)node->data;
    course_free(c);
    return list_remove(head, node);
}

/* ------------------------------ 改 ------------------------------ */

int course_modify_info(ListNode *head, int cid, const Course *newinfo)
{
    ListNode *node;
    Course   *c;

    if (head == NULL || newinfo == NULL) {
        return RES_ERR_PARAM;
    }
    node = list_find(head, course_cmp_id, &cid);
    if (node == NULL) {
        return RES_ERR_NOTFOUND;
    }
    if (newinfo->capacity > 0 && newinfo->capacity < course_student_count((Course *)node->data)) {
        return RES_ERR_PARAM;   /* 人数上限不能小于当前已选人数 */
    }

    c = (Course *)node->data;
    c->cid = cid;               /* 课程ID 不允许修改 */

    if (newinfo->name[0] != '\0') {
        snprintf(c->name, sizeof(c->name), "%s", newinfo->name);
    }
    if (newinfo->intro[0] != '\0') {
        snprintf(c->intro, sizeof(c->intro), "%s", newinfo->intro);
    }
    if (newinfo->teacher_id != 0) {
        c->teacher_id = newinfo->teacher_id;
    }
    if (newinfo->credit >= 0) {
        c->credit = newinfo->credit;
    }
    if (newinfo->hours >= 0) {
        c->hours = newinfo->hours;
    }
    if (newinfo->capacity > 0) {
        c->capacity = newinfo->capacity;
    }
    if (newinfo->status == COURSE_PUBLISHED || newinfo->status == COURSE_UNPUBLISHED) {
        c->status = newinfo->status;
    }

    /* 已选学生列表不在此处覆盖，统一由 course_set_students 处理 */
    c->enrolled = course_student_count(c);
    return RES_OK;
}

/* ---------------------- 已选学生列表维护 ---------------------- */

/* 课程已选人数：以 student_list 的实际节点数为准 */
int course_student_count(const Course *c)
{
    if (c == NULL || c->student_list == NULL) {
        return 0;
    }
    return list_count(c->student_list);
}

/* 判断某学生是否已选该课程 */
int course_has_student(const Course *c, int uid)
{
    if (c == NULL || c->student_list == NULL) {
        return 0;
    }
    return (list_find(c->student_list, course_cmp_uid, &uid) != NULL) ? 1 : 0;
}

/* 学生选课 */
int course_enroll(ListNode *head, int cid, int uid)
{
    ListNode *node;
    Course   *c;
    int      *puid;

    if (head == NULL) {
        return RES_ERR_PARAM;
    }
    node = list_find(head, course_cmp_id, &cid);
    if (node == NULL) {
        return RES_ERR_NOTFOUND;
    }

    c = (Course *)node->data;
    if (course_has_student(c, uid)) {
        return RES_ERR_DUP;
    }
    if (c->capacity > 0 && c->enrolled >= c->capacity) {
        return RES_ERR_FULL;
    }

    puid = (int *)malloc(sizeof(int));
    if (puid == NULL) {
        return RES_ERR_MEMORY;
    }
    *puid = uid;
    if (list_append(c->student_list, puid) != RES_OK) {
        free(puid);
        return RES_ERR_MEMORY;
    }
    c->enrolled = course_student_count(c);
    return RES_OK;
}

/* 学生退课 */
int course_drop(ListNode *head, int cid, int uid)
{
    ListNode *node;
    ListNode *snode;
    Course   *c;

    if (head == NULL) {
        return RES_ERR_PARAM;
    }
    node = list_find(head, course_cmp_id, &cid);
    if (node == NULL) {
        return RES_ERR_NOTFOUND;
    }

    c = (Course *)node->data;
    snode = list_find(c->student_list, course_cmp_uid, &uid);
    if (snode == NULL) {
        return RES_ERR_NOTFOUND;
    }

    free(snode->data);
    list_remove(c->student_list, snode);
    c->enrolled = course_student_count(c);
    return RES_OK;
}

/* 整表设置已选学生列表 */
int course_set_students(ListNode *head, int cid, const int *uids, int n)
{
    ListNode *node;
    Course   *c;
    int       i;

    if (head == NULL || n < 0) {
        return RES_ERR_PARAM;
    }
    if (n > 0 && uids == NULL) {
        return RES_ERR_PARAM;
    }
    node = list_find(head, course_cmp_id, &cid);
    if (node == NULL) {
        return RES_ERR_NOTFOUND;
    }

    c = (Course *)node->data;
    if (c->capacity > 0 && n > c->capacity) {
        return RES_ERR_FULL;
    }

    /* 先清空原有列表，再按新名单重建，避免残留旧数据 */
    list_clear(c->student_list, free);

    for (i = 0; i < n; i++) {
        int *puid;

        if (course_has_student(c, uids[i])) {
            continue;   /* 名单内重复项只记一次 */
        }
        puid = (int *)malloc(sizeof(int));
        if (puid == NULL) {
            c->enrolled = course_student_count(c);
            return RES_ERR_MEMORY;
        }
        *puid = uids[i];
        if (list_append(c->student_list, puid) != RES_OK) {
            free(puid);
            c->enrolled = course_student_count(c);
            return RES_ERR_MEMORY;
        }
    }

    c->enrolled = course_student_count(c);
    return RES_OK;
}
