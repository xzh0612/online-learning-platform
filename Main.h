#ifndef _MAIN_H_
#define _MAIN_H_

/*
 * Main.h —— 在线学习平台管理程序入口的头文件
 *
 * 只声明程序级常量与子菜单入口，具体的业务逻辑分别位于
 * ModuleA（用户管理）、ModuleB（课程管理）、ModuleC（录入与日志）中。
 */

#include "common.h"
#include "link_pub.h"

#define APP_NAME        "在线学习平台管理程序"
#define APP_VERSION     "v0.2.0"

#define APP_USER_FILE   DEFAULT_USER_FILE
#define APP_COURSE_FILE DEFAULT_COURSE_FILE
#define APP_LOG_FILE    DEFAULT_LOG_FILE

/* 二级菜单入口 */
void menu_user(ListNode *users, ListNode *courses);
void menu_course(ListNode *users, ListNode *courses);

/* 数据文件同步：把内存中的用户/课程信息整表回写 */
int  app_save_all(ListNode *users, ListNode *courses);

#endif /* _MAIN_H_ */
