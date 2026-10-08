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
