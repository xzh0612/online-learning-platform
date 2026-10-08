#ifndef _COURSE_PUB_H_
#define _COURSE_PUB_H_

/*
 * course_pub.h —— 课程管理模块（ModuleB）对外接口
 *
 * 覆盖课程要求中的课程管理功能：
 *   新增课程、删除课程、查看某课程详细信息（学分、课时、授课教师、
 *   已选学生列表等）、修改某课程的信息（学分、课时、课程简介、已选学生列表等）。
 *
 * 依赖：link_pub.h（公共双向链表，由 ModuleA 提供）、common.h。
 * 设计约定：Course 中的授课教师只保存 teacher_id，已选学生列表只保存学生 uid，
 *           不使用对方结构体指针，从而避免用户模块与课程模块之间出现循环依赖。
 */

#include "common.h"
#include "link_pub.h"

/* 课程状态 */
typedef enum {
    COURSE_UNPUBLISHED = 0,   /* 未发布 */
    COURSE_PUBLISHED   = 1    /* 已发布 */
} CourseStatus;

/* 课程信息 */
typedef struct {
    int          cid;                       /* 课程ID */
    char         name[COURSE_NAME_LEN];     /* 课程名称 */
    int          teacher_id;                /* 授课教师ID */
    char         intro[COURSE_INTRO_LEN];   /* 课程简介 */
    int          credit;                    /* 学分 */
    int          hours;                     /* 授课学时 */
    int          enrolled;                  /* 课程已选人数（与 student_list 保持一致） */
    int          capacity;                  /* 课程人数上限 */
    CourseStatus status;                    /* 课程状态：已发布 / 未发布 */
    ListNode    *student_list;              /* 已选学生列表，节点 data 为 int*（学生 uid） */
} Course;

/* ------------------------------ 增 ------------------------------ */
/* 新增课程：cid 重复返回 RES_ERR_EXIST；内部会把 enrolled 置 0、student_list 置空 */
int course_add(ListNode *head, const Course *c);

/* ------------------------------ 删 ------------------------------ */
/* 删除课程：同时释放其已选学生列表；不存在返回 RES_ERR_NOTFOUND */
int course_delete(ListNode *head, int cid);

/* ------------------------------ 改 ------------------------------ */
/* 修改课程信息（学分、课时、课程简介、状态等；cid 不可改） */
int course_modify_info(ListNode *head, int cid, const Course *newinfo);

/* -------------------------- 查 -------------------------- */
Course *course_find_by_id(ListNode *head, int cid);

/* ---------------------- 已选学生列表维护 ---------------------- */
/* 学生选课：人数已达上限返回 RES_ERR_FULL，重复选课返回 RES_ERR_DUP */
int course_enroll(ListNode *head, int cid, int uid);

/* 学生退课：未选该课返回 RES_ERR_NOTFOUND */
int course_drop(ListNode *head, int cid, int uid);

/* 整表设置已选学生列表：先清空原有列表，再按 uids 重建 */
int course_set_students(ListNode *head, int cid, const int *uids, int n);

/* 判断某学生是否已选该课程，是返回 1，否返回 0 */
int course_has_student(const Course *c, int uid);

/* 返回课程当前已选人数（以 student_list 为准） */
int course_student_count(const Course *c);

/* -------------------------- 展示与释放 -------------------------- */
const char *course_status_name(CourseStatus st);
int course_status_from_name(const char *name, CourseStatus *out);

void course_print_header(void);
void course_print(const Course *c);
void course_print_list(ListNode *head);
void course_print_detail(const Course *c);   /* 含已选学生列表 */

/* 供 list_clear / list_destroy 使用的释放回调 */
void course_free(void *data);

#endif /* _COURSE_PUB_H_ */
