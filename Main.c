/*
 * Main.c —— 在线学习平台管理程序入口
 *
 * 职责：
 *   1. 程序启动时按产品形态准备初始数据：
 *      - 定义 LOAD_FROM_FILE（文件录入形态）：从 user.txt / course.txt 载入；
 *      - 未定义（键盘录入形态）：由用户从键盘录入。
 *   2. 提供用户管理与课程管理的二级菜单，调度 ModuleA / ModuleB 的对外接口；
 *   3. 每次成功的写操作后：把数据整表回写到文件，并写入一条操作日志。
 *
 * 说明：日志由本层统一记录，模块之间不互相调用日志接口，
 *       这样可以避免模块间出现循环依赖（ModuleA/ModuleB 不依赖 ModuleC）。
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
#include "sha256_pub.h"
#include "Main.h"

/* ------------------------------ 全局状态 ------------------------------ */

static ListNode *g_users   = NULL;
static ListNode *g_courses = NULL;

/* 当前操作者，写入日志的 OPERATOR / ROLE 字段 */
static const char *g_operator = ROLE_NAME_ADMIN;
static const char *g_role     = ROLE_NAME_ADMIN;

/* ------------------------------ 内部工具 ------------------------------ */

static void log_user(LogOpType op, int uid, const char *detail)
{
    char target[LOG_TARGET_LEN];

    snprintf(target, sizeof(target), "uid:%d", uid);
    log_record(op, g_role, target, g_operator, detail);
}

static void log_course(LogOpType op, int cid, const char *detail)
{
    char target[LOG_TARGET_LEN];

    snprintf(target, sizeof(target), "cid:%d", cid);
    log_record(op, g_role, target, g_operator, detail);
}

/* 把内存中的用户与课程信息整表回写到数据文件 */
int app_save_all(ListNode *users, ListNode *courses)
{
    int rc1 = io_save_users(APP_USER_FILE, users);
    int rc2 = io_save_courses(APP_COURSE_FILE, courses);

    if (rc1 != RES_OK || rc2 != RES_OK) {
        printf("[警告] 数据回写失败（用户 %d，课程 %d）。\n", rc1, rc2);
        return RES_ERR_IO;
    }
    return RES_OK;
}

static int save_all(void)
{
    return app_save_all(g_users, g_courses);
}

static void pause_enter(void)
{
    char buf[LINE_BUF_LEN];

    printf("\n按回车键继续...");
    fflush(stdout);
    (void)com_read_line(buf, (int)sizeof(buf));
}

/* 统一的结果提示 */
static void print_result(int rc, const char *okmsg)
{
    switch (rc) {
    case RES_OK:         printf("[成功] %s\n", okmsg); break;
    case RES_ERR_EXIST:  printf("[失败] 该 ID 的记录已存在。\n"); break;
    case RES_ERR_DUP:    printf("[失败] 存在重复数据（用户名重复 / 重复选课）。\n"); break;
    case RES_ERR_NOTFOUND: printf("[失败] 记录不存在。\n"); break;
    case RES_ERR_FULL:   printf("[失败] 课程人数已达上限。\n"); break;
    case RES_ERR_PARAM:  printf("[失败] 输入参数不合法。\n"); break;
    case RES_ERR_MEMORY: printf("[失败] 内存分配失败。\n"); break;
    case RES_ERR_IO:     printf("[失败] 文件读写失败。\n"); break;
    default:             printf("[失败] 未知错误(%d)。\n", rc); break;
    }
}

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

/* ============================ 用户管理菜单 ============================ */

static void do_user_add(void)
{
    char name[USER_NAME_LEN];
    char pwd[64];
    char role[16];
    char detail[LOG_DETAIL_LEN];
    User u;
    int  uid;
    int  rc;

    com_title("新增用户");
    if (com_read_int("用户ID（回车取消）: ", &uid) != RES_OK) {
        printf("已取消。\n");
        return;
    }
    if (com_read_str("用户名: ", name, (int)sizeof(name)) != RES_OK || name[0] == '\0') {
        printf("用户名不能为空，已取消。\n");
        return;
    }
    if (com_read_str("密码: ", pwd, (int)sizeof(pwd)) != RES_OK || pwd[0] == '\0') {
        printf("密码不能为空，已取消。\n");
        return;
    }
    if (com_read_str("角色(student/teacher，回车默认 student): ", role, (int)sizeof(role)) != RES_OK) {
        role[0] = '\0';
    }

    memset(&u, 0, sizeof(u));
    u.uid = uid;
    snprintf(u.username, sizeof(u.username), "%s", name);
    /* 密码在进入内存前先做 SHA-256 加盐哈希，内存与文件中都不出现明文 */
    hash_password(u.username, pwd, u.password_hash);
    if (user_role_from_name(role, &u.role) != RES_OK) {
        u.role = ROLE_STUDENT;
    }
    com_now(u.reg_time, (int)sizeof(u.reg_time));

    rc = user_add(g_users, &u);
    if (rc == RES_OK) {
        snprintf(detail, sizeof(detail), "新增用户 %s，角色 %s",
                 u.username, user_role_name(u.role));
        log_user(LOG_USER_ADD, u.uid, detail);
        save_all();
    }
    print_result(rc, "用户已新增，数据已同步到文件。");
}

static void do_user_modify(void)
{
    char  buf[USER_NAME_LEN];
    char  pwd[64];
    char  role[16];
    char  detail[LOG_DETAIL_LEN];
    User *cur;
    User  nw;
    int   uid;
    int   rc;

    com_title("修改用户信息");
    if (com_read_int("要修改的用户ID（回车取消）: ", &uid) != RES_OK) {
        printf("已取消。\n");
        return;
    }
    cur = user_find_by_id(g_users, uid);
    if (cur == NULL) {
        printf("[失败] 用户ID %d 不存在。\n", uid);
        return;
    }
    printf("当前信息：\n");
    user_print_header();
    user_print(cur);

    /* 以当前值为基础，用户只修改需要变更的字段 */
    nw = *cur;

    if (com_read_str("新用户名（回车保持不变）: ", buf, (int)sizeof(buf)) == RES_OK && buf[0] != '\0') {
        snprintf(nw.username, sizeof(nw.username), "%s", buf);
    }
    if (com_read_str("新密码（回车保持不变）: ", pwd, (int)sizeof(pwd)) == RES_OK && pwd[0] != '\0') {
        hash_password(nw.username, pwd, nw.password_hash);
    }
    if (com_read_str("新角色(student/teacher，回车保持不变): ", role, (int)sizeof(role)) == RES_OK &&
        role[0] != '\0') {
        RoleType r;

        if (user_role_from_name(role, &r) == RES_OK) {
            nw.role = r;
        }
    }

    rc = user_modify(g_users, uid, &nw);
    if (rc == RES_OK) {
        /* 详情中带上"被操作对象的角色"，便于日志分析脚本区分学生/教师信息修改 */
        snprintf(detail, sizeof(detail), "修改用户 %s（ID=%d，角色 %s）",
                 nw.username, uid, user_role_name(nw.role));
        log_user(LOG_USER_MOD, uid, detail);
        save_all();
    }
    print_result(rc, "用户信息已修改，数据已同步到文件。");
}

static void do_user_delete(void)
{
    char detail[LOG_DETAIL_LEN];
    User *cur;
    int   uid;
    int   rc;

    com_title("删除用户");
    if (com_read_int("要删除的用户ID（回车取消）: ", &uid) != RES_OK) {
        printf("已取消。\n");
        return;
    }
    cur = user_find_by_id(g_users, uid);
    if (cur == NULL) {
        printf("[失败] 用户ID %d 不存在。\n", uid);
        return;
    }
    snprintf(detail, sizeof(detail), "删除用户 %s（ID=%d，角色 %s）",
             cur->username, uid, user_role_name(cur->role));

    rc = user_delete(g_users, uid);
    if (rc == RES_OK) {
        log_user(LOG_USER_DEL, uid, detail);
        save_all();
    }
    print_result(rc, "用户已删除，数据已同步到文件。");
}

static void do_user_find_by_id(void)
{
    User *u;
    int   uid;

    com_title("按用户ID 查看用户详细信息");
    if (com_read_int("用户ID: ", &uid) != RES_OK) {
        printf("输入无效。\n");
        return;
    }
    u = user_find_by_id(g_users, uid);
    if (u == NULL) {
        printf("[失败] 用户ID %d 不存在。\n", uid);
        return;
    }
    user_print_header();
    user_print(u);
}

static void do_user_find_by_name(void)
{
    char  name[USER_NAME_LEN];
    User *u;

    com_title("按用户名查看用户详细信息");
    if (com_read_str("用户名: ", name, (int)sizeof(name)) != RES_OK || name[0] == '\0') {
        printf("输入无效。\n");
        return;
    }
    u = user_find_by_name(g_users, name);
    if (u == NULL) {
        printf("[失败] 用户名 %s 不存在。\n", name);
        return;
    }
    user_print_header();
    user_print(u);
}

static void do_user_filter_role(void)
{
    char      role[16];
    RoleType  r;
    ListNode *lst;
    ListNode *p;

    com_title("按角色筛选用户列表");
    if (com_read_str("角色(student/teacher): ", role, (int)sizeof(role)) != RES_OK) {
        printf("输入无效。\n");
        return;
    }
    if (user_role_from_name(role, &r) != RES_OK) {
        printf("[失败] 角色取值无效，只能输入 student 或 teacher。\n");
        return;
    }

    lst = user_filter_by_role(g_users, r);
    if (lst == NULL) {
        printf("[失败] 筛选失败。\n");
        return;
    }
    printf("角色为 %s 的用户共 %d 人：\n", user_role_name(r), list_count(lst));
    user_print_header();
    for (p = lst->next; p != NULL; p = p->next) {
        user_print((const User *)p->data);
    }
    list_destroy(&lst, user_free);
}

void menu_user(ListNode *users, ListNode *courses)
{
    int choice;

    (void)users;
    (void)courses;

    for (;;) {
        printf("\n");
        com_hr();
        printf(" 用户管理（User Management）        [用户总数 %d]\n", list_count(g_users));
        com_hr();
        printf("  1. 新增用户信息\n");
        printf("  2. 修改用户信息\n");
        printf("  3. 删除用户信息\n");
        printf("  4. 按用户ID 查看用户详细信息\n");
        printf("  5. 按用户名查看用户详细信息\n");
        printf("  6. 按角色筛选用户列表\n");
        printf("  7. 查看全部用户\n");
        printf("  0. 返回主菜单\n");
        com_hr();
        if (com_read_int("请选择: ", &choice) != RES_OK) {
            printf("输入无效，请重新选择。\n");
            continue;
        }
        switch (choice) {
        case 1: do_user_add();          pause_enter(); break;
        case 2: do_user_modify();       pause_enter(); break;
        case 3: do_user_delete();       pause_enter(); break;
        case 4: do_user_find_by_id();   pause_enter(); break;
        case 5: do_user_find_by_name(); pause_enter(); break;
        case 6: do_user_filter_role();  pause_enter(); break;
        case 7: com_title("全部用户"); user_print_list(g_users); pause_enter(); break;
        case 0: return;
        default: printf("无效选项，请重新选择。\n");
        }
    }
}

/* ============================ 课程管理菜单 ============================ */

static void do_course_add(void)
{
    char    name[COURSE_NAME_LEN];
    char    intro[COURSE_INTRO_LEN];
    char    status[16];
    char    detail[LOG_DETAIL_LEN];
    Course  c;
    int     cid;
    int     rc;

    com_title("新增课程");
    if (com_read_int("课程ID（回车取消）: ", &cid) != RES_OK) {
        printf("已取消。\n");
        return;
    }
    if (com_read_str("课程名称: ", name, (int)sizeof(name)) != RES_OK || name[0] == '\0') {
        printf("课程名称不能为空，已取消。\n");
        return;
    }

    memset(&c, 0, sizeof(c));
    c.cid = cid;
    snprintf(c.name, sizeof(c.name), "%s", name);
    if (com_read_int("授课教师ID: ", &c.teacher_id) != RES_OK) {
        c.teacher_id = 0;
    }
    if (com_read_str("课程简介: ", intro, (int)sizeof(intro)) == RES_OK) {
        snprintf(c.intro, sizeof(c.intro), "%s", intro);
    }
    if (com_read_int("学分: ", &c.credit) != RES_OK) {
        c.credit = 0;
    }
    if (com_read_int("授课学时: ", &c.hours) != RES_OK) {
        c.hours = 0;
    }
    if (com_read_int("课程人数上限: ", &c.capacity) != RES_OK) {
        c.capacity = 0;
    }
    if (com_read_str("课程状态(published/unpublished，回车默认 published): ",
                     status, (int)sizeof(status)) != RES_OK ||
        course_status_from_name(status, &c.status) != RES_OK) {
        c.status = COURSE_PUBLISHED;
    }

    rc = course_add(g_courses, &c);
    if (rc == RES_OK) {
        snprintf(detail, sizeof(detail), "新增课程 %s，学分 %d，学时 %d",
                 c.name, c.credit, c.hours);
        log_course(LOG_COURSE_ADD, c.cid, detail);
        save_all();
    }
    print_result(rc, "课程已新增，数据已同步到文件。");
}

static void do_course_modify(void)
{
    char    buf[LINE_BUF_LEN];
    char    detail[LOG_DETAIL_LEN];
    Course *cur;
    Course  nw;
    int     cid;
    int     rc;

    com_title("修改课程信息");
    if (com_read_int("要修改的课程ID（回车取消）: ", &cid) != RES_OK) {
        printf("已取消。\n");
        return;
    }
    cur = course_find_by_id(g_courses, cid);
    if (cur == NULL) {
        printf("[失败] 课程ID %d 不存在。\n", cid);
        return;
    }
    printf("当前信息：\n");
    course_print_detail(cur);

    /* 以当前值为基础，只覆盖用户确实修改了的字段 */
    nw = *cur;
    nw.student_list = cur->student_list;

    if (com_read_str("新学分（回车保持不变）: ", buf, (int)sizeof(buf)) == RES_OK && buf[0] != '\0') {
        nw.credit = atoi(buf);
    }
    if (com_read_str("新授课学时（回车保持不变）: ", buf, (int)sizeof(buf)) == RES_OK && buf[0] != '\0') {
        nw.hours = atoi(buf);
    }
    if (com_read_str("新课程简介（回车保持不变）: ", buf, (int)sizeof(buf)) == RES_OK && buf[0] != '\0') {
        snprintf(nw.intro, sizeof(nw.intro), "%s", buf);
    }
    if (com_read_str("新课程人数上限（回车保持不变）: ", buf, (int)sizeof(buf)) == RES_OK && buf[0] != '\0') {
        nw.capacity = atoi(buf);
    }
    if (com_read_str("新课程状态(published/unpublished，回车保持不变): ",
                     buf, (int)sizeof(buf)) == RES_OK && buf[0] != '\0') {
        CourseStatus st;

        if (course_status_from_name(buf, &st) == RES_OK) {
            nw.status = st;
        }
    }
    if (com_read_str("新授课教师ID（回车保持不变）: ", buf, (int)sizeof(buf)) == RES_OK && buf[0] != '\0') {
        nw.teacher_id = atoi(buf);
    }

    rc = course_modify_info(g_courses, cid, &nw);
    if (rc == RES_OK) {
        snprintf(detail, sizeof(detail), "修改课程信息：学分=%d 学时=%d 上限=%d 状态=%s",
                 nw.credit, nw.hours, nw.capacity, course_status_name(nw.status));
        log_course(LOG_COURSE_MOD, cid, detail);
        save_all();
    }
    print_result(rc, "课程信息已修改，数据已同步到文件。");
}

static void do_course_delete(void)
{
    char    detail[LOG_DETAIL_LEN];
    Course *cur;
    int     cid;
    int     rc;

    com_title("删除课程");
    if (com_read_int("要删除的课程ID（回车取消）: ", &cid) != RES_OK) {
        printf("已取消。\n");
        return;
    }
    cur = course_find_by_id(g_courses, cid);
    if (cur == NULL) {
        printf("[失败] 课程ID %d 不存在。\n", cid);
        return;
    }
    snprintf(detail, sizeof(detail), "删除课程 %s（ID=%d）", cur->name, cid);

    rc = course_delete(g_courses, cid);
    if (rc == RES_OK) {
        log_course(LOG_COURSE_DEL, cid, detail);
        save_all();
    }
    print_result(rc, "课程已删除，数据已同步到文件。");
}

static void do_course_detail(void)
{
    Course   *c;
    ListNode *p;
    User     *t;
    int       cid;

    com_title("查看课程详细信息");
    if (com_read_int("课程ID: ", &cid) != RES_OK) {
        printf("输入无效。\n");
        return;
    }
    c = course_find_by_id(g_courses, cid);
    if (c == NULL) {
        printf("[失败] 课程ID %d 不存在。\n", cid);
        return;
    }

    course_print_detail(c);

    /* 课程模块只保存教师ID 与学生 uid，这里用用户模块的接口解析出姓名，
       体现两个模块通过公共接口协作，而不是互相持有对方的结构体指针 */
    t = user_find_by_id(g_users, c->teacher_id);
    printf("授课教师   : %s\n", (t != NULL) ? t->username : "(用户表中无该教师记录)");

    if (c->student_list != NULL && c->student_list->next != NULL) {
        printf("学生名单（%d 人）:\n", list_count(c->student_list));
        user_print_header();
        for (p = c->student_list->next; p != NULL; p = p->next) {
            User *s = user_find_by_id(g_users, *(int *)p->data);

            if (s != NULL) {
                user_print(s);
            } else {
                printf("用户表中不存在 uid=%d 的学生记录\n", *(int *)p->data);
            }
        }
    } else {
        printf("学生名单（0 人）: 暂无学生选课\n");
    }
}

static void do_course_enroll(void)
{
    char detail[LOG_DETAIL_LEN];
    int  cid;
    int  uid;
    int  rc;

    com_title("学生选课");
    if (com_read_int("课程ID: ", &cid) != RES_OK || com_read_int("学生用户ID: ", &uid) != RES_OK) {
        printf("输入无效。\n");
        return;
    }
    if (user_find_by_id(g_users, uid) == NULL) {
        printf("[失败] 用户ID %d 不存在，无法选课。\n", uid);
        return;
    }

    rc = course_enroll(g_courses, cid, uid);
    if (rc == RES_OK) {
        snprintf(detail, sizeof(detail), "学生 uid:%d 选课", uid);
        log_course(LOG_COURSE_MOD, cid, detail);
        save_all();
    }
    print_result(rc, "选课成功，数据已同步到文件。");
}

static void do_course_drop(void)
{
    char detail[LOG_DETAIL_LEN];
    int  cid;
    int  uid;
    int  rc;

    com_title("学生退课");
    if (com_read_int("课程ID: ", &cid) != RES_OK || com_read_int("学生用户ID: ", &uid) != RES_OK) {
        printf("输入无效。\n");
        return;
    }

    rc = course_drop(g_courses, cid, uid);
    if (rc == RES_OK) {
        snprintf(detail, sizeof(detail), "学生 uid:%d 退课", uid);
        log_course(LOG_COURSE_MOD, cid, detail);
        save_all();
    }
    print_result(rc, "退课成功，数据已同步到文件。");
}

static void do_course_set_students(void)
{
    char detail[LOG_DETAIL_LEN];
    char buf[LINE_BUF_LEN];
    int  uids[128];
    int  cnt = 0;
    int  cid;
    int  rc;

    com_title("修改课程的已选学生列表");
    if (com_read_int("课程ID: ", &cid) != RES_OK) {
        printf("输入无效。\n");
        return;
    }
    if (course_find_by_id(g_courses, cid) == NULL) {
        printf("[失败] 课程ID %d 不存在。\n", cid);
        return;
    }
    if (com_read_str("新的已选学生ID（英文逗号分隔，留空表示清空名单）: ",
                     buf, (int)sizeof(buf)) != RES_OK) {
        printf("输入无效。\n");
        return;
    }
    if (parse_uid_array(buf, uids, (int)(sizeof(uids) / sizeof(uids[0])), &cnt) != RES_OK) {
        printf("[失败] 名单解析失败。\n");
        return;
    }

    rc = course_set_students(g_courses, cid, uids, cnt);
    if (rc == RES_OK) {
        snprintf(detail, sizeof(detail), "重设已选学生列表，共 %d 人", cnt);
        log_course(LOG_COURSE_MOD, cid, detail);
        save_all();
    }
    print_result(rc, "已选学生列表已更新，数据已同步到文件。");
}

void menu_course(ListNode *users, ListNode *courses)
{
    int choice;

    (void)users;
    (void)courses;

    for (;;) {
        printf("\n");
        com_hr();
        printf(" 课程管理（Course Management）      [课程总数 %d]\n", list_count(g_courses));
        com_hr();
        printf("  1. 新增课程\n");
        printf("  2. 修改课程信息（学分/课时/简介/上限/状态）\n");
        printf("  3. 删除课程\n");
        printf("  4. 查看课程详细信息（含已选学生列表）\n");
        printf("  5. 学生选课\n");
        printf("  6. 学生退课\n");
        printf("  7. 修改课程的已选学生列表\n");
        printf("  8. 查看全部课程\n");
        printf("  0. 返回主菜单\n");
        com_hr();
        if (com_read_int("请选择: ", &choice) != RES_OK) {
            printf("输入无效，请重新选择。\n");
            continue;
        }
        switch (choice) {
        case 1: do_course_add();          pause_enter(); break;
        case 2: do_course_modify();       pause_enter(); break;
        case 3: do_course_delete();       pause_enter(); break;
        case 4: do_course_detail();       pause_enter(); break;
        case 5: do_course_enroll();       pause_enter(); break;
        case 6: do_course_drop();         pause_enter(); break;
        case 7: do_course_set_students(); pause_enter(); break;
        case 8: com_title("全部课程"); course_print_list(g_courses); pause_enter(); break;
        case 0: return;
        default: printf("无效选项，请重新选择。\n");
        }
    }
}

/* ============================ 程序入口 ============================ */

static int app_init(void)
{
    if (log_init(APP_LOG_FILE) != RES_OK) {
        printf("[警告] 无法打开日志文件 %s，操作日志将不可用。\n", APP_LOG_FILE);
    }

    g_users = list_create();
    g_courses = list_create();
    if (g_users == NULL || g_courses == NULL) {
        printf("[错误] 内存分配失败，程序退出。\n");
        return RES_ERR_MEMORY;
    }

#ifdef LOAD_FROM_FILE
    {
        int nu = io_load_users(APP_USER_FILE, g_users);
        int nc = io_load_courses(APP_COURSE_FILE, g_courses);

        if (nu < 0 || nc < 0) {
            printf("[警告] 初始数据文件读取失败（用户返回 %d，课程返回 %d）。\n", nu, nc);
            printf("       请在工程根目录下运行本程序，并确认 %s 与 %s 存在。\n",
                   APP_USER_FILE, APP_COURSE_FILE);
        } else {
            printf("已从文件载入初始信息：用户 %d 条，课程 %d 条。\n", nu, nc);
        }
    }
#else
    {
        int nu = 0;
        int nc = 0;

        printf("键盘录入形态：请先录入初始信息，直接回车可跳过。\n");
        io_input_users(g_users, &nu);
        io_input_courses(g_courses, &nc);
        if (nu > 0 || nc > 0) {
            save_all();
        }
    }
#endif
    return RES_OK;
}

static void app_exit(void)
{
    printf("\n正在把内存数据回写到文件...\n");
    save_all();
    log_close();
    list_destroy(&g_users, user_free);
    list_destroy(&g_courses, course_free);
    printf("数据已保存，程序退出。\n");
}

static void show_all(void)
{
    com_title("全部用户");
    user_print_list(g_users);
    com_title("全部课程");
    course_print_list(g_courses);
}

int main(void)
{
    int choice;

    printf("=======================================================\n");
    printf("  %s  %s\n", APP_NAME, APP_VERSION);
#ifdef LOAD_FROM_FILE
    printf("  产品形态：文件录入（编译宏 LOAD_FROM_FILE）\n");
    printf("  数据文件：%s / %s\n", APP_USER_FILE, APP_COURSE_FILE);
#else
    printf("  产品形态：键盘录入（未定义 LOAD_FROM_FILE）\n");
#endif
    printf("  日志文件：%s\n", APP_LOG_FILE);
    printf("=======================================================\n");

    if (app_init() != RES_OK) {
        return 1;
    }

    for (;;) {
        printf("\n");
        com_hr();
        printf(" %s\n", APP_NAME);
        com_hr();
        printf("  1. 用户管理\n");
        printf("  2. 课程管理\n");
        printf("  3. 查看全部数据（用户 + 课程）\n");
        printf("  4. 保存数据到文件\n");
        printf("  0. 退出程序\n");
        com_hr();
        if (com_read_int("请选择: ", &choice) != RES_OK) {
            printf("输入无效，请重新选择。\n");
            continue;
        }

        switch (choice) {
        case 1: menu_user(g_users, g_courses); break;
        case 2: menu_course(g_users, g_courses); break;
        case 3: show_all(); pause_enter(); break;
        case 4:
            if (save_all() == RES_OK) {
                printf("[成功] 数据已回写到 %s 与 %s。\n", APP_USER_FILE, APP_COURSE_FILE);
            }
            pause_enter();
            break;
        case 0:
            app_exit();
            return 0;
        default:
            printf("无效选项，请重新选择。\n");
            break;
        }
    }
}
