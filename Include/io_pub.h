#ifndef _IO_PUB_H_
#define _IO_PUB_H_

/*
 * io_pub.h —— 数据录入模块（ModuleC）对外接口
 *
 * 实现课程要求中"初始信息录入的两种方式"：
 *   方式一：文件方式 —— 从 user.txt / course.txt 读取初始信息记入内存；
 *   方式二：键盘方式 —— 程序运行时从键盘逐条输入初始信息。
 * 两种方式编译期由宏 LOAD_FROM_FILE 选择（见 myenv.sh 与 Main.c）。
 *
 * 另外提供修改后的数据回写与单行解析能力，供文件方式与键盘方式共用。
 */

#include "common.h"
#include "link_pub.h"
#include "user_pub.h"
#include "course_pub.h"

/* ---------------------- 产品形态一：文件方式 ---------------------- */
/* 从文件载入用户/课程信息到链表，返回实际载入条数；文件不存在返回 RES_ERR_IO */
int io_load_users(const char *path, ListNode *head);
int io_load_courses(const char *path, ListNode *head);

/* 把内存链表中全部记录回写（覆盖）到文件，成功返回 RES_OK */
int io_save_users(const char *path, ListNode *head);
int io_save_courses(const char *path, ListNode *head);

/* 单行解析：把数据文件中的一行解析为 User / Course，成功返回 RES_OK */
int io_parse_user_line(const char *line, User *out);
int io_parse_course_line(const char *line, Course *out);

/* ---------------------- 产品形态二：键盘方式 ---------------------- */
/* 键盘录入一个用户/课程，成功返回 RES_OK，用户放弃输入返回 RES_ERR_PARAM */
int io_input_user(ListNode *head);
int io_input_course(ListNode *head);

/*
 * 键盘批量录入：反复调用单条录入，直到用户选择结束。
 * *count 用于返回本次录入的条数（可为 NULL）。
 */
int io_input_users(ListNode *head, int *count);
int io_input_courses(ListNode *head, int *count);

#endif /* _IO_PUB_H_ */
