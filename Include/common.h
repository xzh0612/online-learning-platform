#ifndef _COMMON_H_
#define _COMMON_H_

/*
 * common.h —— 全工程公用的宏定义与内联工具函数
 *
 * 设计说明：本头文件只包含宏定义与 static inline 工具函数，
 * 不产生任何编译单元、不依赖任何模块，因此任何模块都可以安全地包含它，
 * 不会引入模块之间的链接依赖。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* 通用返回码                                                          */
/* ------------------------------------------------------------------ */
#define RES_OK             0    /* 成功 */
#define RES_ERR_PARAM     -1    /* 参数非法 */
#define RES_ERR_MEMORY    -2    /* 内存分配失败 */
#define RES_ERR_EXIST     -3    /* 记录已存在 */
#define RES_ERR_NOTFOUND  -4    /* 记录不存在 */
#define RES_ERR_FULL      -5    /* 课程人数已达上限 */
#define RES_ERR_IO        -6    /* 文件读写失败 */
#define RES_ERR_DUP       -7    /* 数据重复（如重复选课） */

/* ------------------------------------------------------------------ */
/* 字段长度与缓冲区长度                                                 */
/* ------------------------------------------------------------------ */
#define USER_NAME_LEN      32
#define USER_HASH_LEN      65    /* SHA-256 十六进制 64 字符 + '\0' */
#define USER_TIME_LEN      20    /* "YYYY-MM-DD HH:MM:SS" + '\0' */
#define COURSE_NAME_LEN    64
#define COURSE_INTRO_LEN   256
#define LOG_ROLE_LEN       16
#define LOG_TARGET_LEN     24
#define LOG_OPER_LEN       32
#define LOG_DETAIL_LEN     128
#define LINE_BUF_LEN       512

/* ------------------------------------------------------------------ */
/* 默认文件路径（相对工程根目录）                                        */
/* ------------------------------------------------------------------ */
#define DEFAULT_USER_FILE     "data/user.txt"
#define DEFAULT_COURSE_FILE   "data/course.txt"
#define DEFAULT_LOG_FILE      "log.log"
#define DEFAULT_RESULT_FILE   "Docs/log_report.txt"

/* ------------------------------------------------------------------ */
/* 角色名与数据文件字段分隔符                                            */
/* ------------------------------------------------------------------ */
#define ROLE_NAME_STUDENT   "student"
#define ROLE_NAME_TEACHER   "teacher"
#define ROLE_NAME_ADMIN     "admin"

/* 数据文件与日志文件均使用 '|' 作为字段分隔符，便于 awk 解析 */
#define FIELD_SEP        '|'
#define FIELD_SEP_STR    "|"

/* ------------------------------------------------------------------ */
/* 内联工具函数                                                         */
/* ------------------------------------------------------------------ */

/* 去掉字符串首尾的空白字符（空格、制表符、回车、换行），返回处理后长度 */
static inline int com_trim(char *s)
{
    char  *p;
    size_t len;

    if (s == NULL) {
        return 0;
    }
    p = s;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        p++;
    }
    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }
    len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                       s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
    return (int)len;
}

/* 取当前本地时间，格式 "YYYY-MM-DD HH:MM:SS" */
static inline void com_now(char *buf, int buflen)
{
    time_t     t;
    struct tm  tmv;

    if (buf == NULL || buflen <= 0) {
        return;
    }
    buf[0] = '\0';
    t = time(NULL);
    if (localtime_r(&t, &tmv) == NULL) {
        return;
    }
    strftime(buf, (size_t)buflen, "%Y-%m-%d %H:%M:%S", &tmv);
}

/* 从标准输入读一行，去掉行尾换行与首尾空白；成功返回长度，遇到 EOF 返回 -1 */
static inline int com_read_line(char *buf, int buflen)
{
    if (buf == NULL || buflen <= 0) {
        return -1;
    }
    if (fgets(buf, buflen, stdin) == NULL) {
        return -1;
    }
    com_trim(buf);
    return (int)strlen(buf);
}

/* 带提示符读取一个整数；成功返回 RES_OK */
static inline int com_read_int(const char *prompt, int *out)
{
    char buf[LINE_BUF_LEN];

    if (out == NULL) {
        return RES_ERR_PARAM;
    }
    if (prompt != NULL) {
        printf("%s", prompt);
        fflush(stdout);
    }
    if (com_read_line(buf, (int)sizeof(buf)) < 0) {
        return RES_ERR_PARAM;
    }
    if (sscanf(buf, "%d", out) != 1) {
        return RES_ERR_PARAM;
    }
    return RES_OK;
}

/* 带提示符读取一个字符串到 out；成功返回 RES_OK */
static inline int com_read_str(const char *prompt, char *out, int outlen)
{
    if (out == NULL || outlen <= 0) {
        return RES_ERR_PARAM;
    }
    if (prompt != NULL) {
        printf("%s", prompt);
        fflush(stdout);
    }
    if (com_read_line(out, outlen) < 0) {
        out[0] = '\0';
        return RES_ERR_PARAM;
    }
    return RES_OK;
}

/*
 * 按 delim 就地切分字符串：把每个字段起始地址写入 out[]，返回字段个数。
 * 注意：会修改传入的 line（把分隔符替换为 '\0'），line 必须是可写缓冲区。
 * 行尾正好是分隔符时，最后一个空字段不计入。
 */
static inline int com_split(char *line, char delim, char *out[], int maxout)
{
    int   n = 0;
    char *p;

    if (line == NULL || out == NULL || maxout <= 0) {
        return 0;
    }
    p = line;
    while (n < maxout) {
        out[n++] = p;
        p = strchr(p, (int)delim);
        if (p == NULL) {
            break;
        }
        *p++ = '\0';
    }
    return n;
}

/* 输出一条分隔线，用于菜单与表格美化 */
static inline void com_hr(void)
{
    printf("------------------------------------------------------------------\n");
}

/* 输出一个字段标题行 */
static inline void com_title(const char *title)
{
    printf("\n==== %s ====\n", title);
}

#endif /* _COMMON_H_ */
