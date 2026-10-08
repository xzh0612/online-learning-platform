/*
 * course_util.c —— 课程管理模块的展示与释放工具（ModuleB）
 *
 * 集中实现课程状态枚举与字符串互转、课程信息的格式化输出
 * （列表视图与详情视图），以及供 list_clear / list_destroy 使用的释放回调。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "link_pub.h"
#include "course_pub.h"

/* 数据文件与界面中使用的状态名 */
const char *course_status_name(CourseStatus st)
{
    return (st == COURSE_PUBLISHED) ? "published" : "unpublished";
}

/* 由字符串还原课程状态枚举；成功返回 RES_OK */
int course_status_from_name(const char *name, CourseStatus *out)
{
    if (name == NULL || out == NULL) {
        return RES_ERR_PARAM;
    }
    if (strcmp(name, "published") == 0) {
        *out = COURSE_PUBLISHED;
        return RES_OK;
    }
    if (strcmp(name, "unpublished") == 0) {
        *out = COURSE_UNPUBLISHED;
        return RES_OK;
    }
    return RES_ERR_PARAM;
}

/* 列表视图的分隔线 */
static void course_print_sep(void)
{
    printf("--------  --------------------  ---------  ------  ------  ---------  -----------\n");
}

void course_print_header(void)
{
    printf("%-8s  %-20s  %-9s  %-6s  %-6s  %-9s  %-11s\n",
           "CourseID", "CourseName", "TeacherID", "Credit", "Hours", "Enrolled", "Status");
    course_print_sep();
}

void course_print(const Course *c)
{
    char enroll_buf[16];

    snprintf(enroll_buf, sizeof(enroll_buf), "%d/%d",
             course_student_count(c), c->capacity);

    printf("%-8d  %-20s  %-9d  %-6d  %-6d  %-9s  %-11s\n",
           c->cid,
           c->name,
           c->teacher_id,
           c->credit,
           c->hours,
           enroll_buf,
           course_status_name(c->status));
}

void course_print_list(ListNode *head)
{
    ListNode *p;

    if (head == NULL) {
        printf("[课程列表] 链表未初始化\n");
        return;
    }

    course_print_header();
    for (p = head->next; p != NULL; p = p->next) {
        course_print((const Course *)p->data);
    }
    course_print_sep();
    printf("共 %d 门课程\n", list_count(head));
}

/* 详情视图：学分、课时、授课教师、已选学生列表等 */
void course_print_detail(const Course *c)
{
    ListNode *p;
    int       first = 1;

    if (c == NULL) {
        printf("[课程详情] 课程不存在\n");
        return;
    }

    printf("课程ID     : %d\n", c->cid);
    printf("课程名称   : %s\n", c->name);
    printf("授课教师ID : %d\n", c->teacher_id);
    printf("课程简介   : %s\n", (c->intro[0] == '\0') ? "(无)" : c->intro);
    printf("学分/学时  : %d 学分 / %d 学时\n", c->credit, c->hours);
    printf("已选/上限  : %d / %d\n", course_student_count(c), c->capacity);
    printf("课程状态   : %s\n", course_status_name(c->status));

    printf("已选学生   : ");
    for (p = c->student_list == NULL ? NULL : c->student_list->next;
         p != NULL; p = p->next) {
        printf("%s%d", first ? "" : ", ", *(int *)p->data);
        first = 0;
    }
    if (first) {
        printf("(暂无学生选课)");
    }
    printf("\n");
}

/* 释放一门课程：先释放已选学生列表，再释放课程结构体本身 */
void course_free(void *data)
{
    Course *c = (Course *)data;

    if (c == NULL) {
        return;
    }
    if (c->student_list != NULL) {
        list_destroy(&c->student_list, free);
    }
    free(c);
}
