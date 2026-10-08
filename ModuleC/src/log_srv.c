/*
 * log_srv.c —— 操作日志服务实现（ModuleC）
 *
 * 课程要求：通过命令行修改信息时，需将修改同步到文件中，
 *           并记录操作日志到日志文件 log.log。
 *
 * 日志行格式（字段之间以 " | " 分隔，便于 awk 稳定解析）：
 *   YYYY-MM-DD HH:MM:SS | OP | ROLE | TARGET | OPERATOR | DETAIL
 *
 * 其中 OP 取值为 USER_ADD / USER_MOD / USER_DEL / COURSE_ADD /
 * COURSE_MOD / COURSE_DEL / DATA_LOAD / DATA_SAVE，
 * 后续的日志分析脚本正是按 OP 与 TARGET 两个字段完成统计与检索。
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "log_pub.h"

/* 已打开的日志文件，NULL 表示日志服务尚未初始化 */
static FILE *g_log_fp = NULL;

/* 返回操作类型对应的短字符串 */
const char *log_op_name(LogOpType op)
{
    switch (op) {
    case LOG_USER_ADD:   return "USER_ADD";
    case LOG_USER_MOD:   return "USER_MOD";
    case LOG_USER_DEL:   return "USER_DEL";
    case LOG_COURSE_ADD: return "COURSE_ADD";
    case LOG_COURSE_MOD: return "COURSE_MOD";
    case LOG_COURSE_DEL: return "COURSE_DEL";
    case LOG_DATA_LOAD:  return "DATA_LOAD";
    case LOG_DATA_SAVE:  return "DATA_SAVE";
    default:             return "UNKNOWN";
    }
}

/* 初始化日志：以追加方式打开，重复调用会先关闭上一个文件 */
int log_init(const char *path)
{
    if (g_log_fp != NULL) {
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
    if (path == NULL) {
        path = DEFAULT_LOG_FILE;
    }

    g_log_fp = fopen(path, "a");
    if (g_log_fp == NULL) {
        return RES_ERR_IO;
    }
    return RES_OK;
}

/* 关闭日志文件 */
void log_close(void)
{
    if (g_log_fp != NULL) {
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
}

/*
 * 记录一条操作日志。
 * 写入后立即 fflush，保证程序异常退出时日志不丢失。
 */
int log_record(LogOpType op, const char *role, const char *target_id,
               const char *operator_name, const char *detail)
{
    char now[USER_TIME_LEN];

    if (g_log_fp == NULL) {
        return RES_ERR_IO;
    }

    com_now(now, (int)sizeof(now));
    fprintf(g_log_fp, "%s | %s | %s | %s | %s | %s\n",
            now,
            log_op_name(op),
            (role != NULL) ? role : "-",
            (target_id != NULL) ? target_id : "-",
            (operator_name != NULL) ? operator_name : "-",
            (detail != NULL) ? detail : "-");
    fflush(g_log_fp);
    return RES_OK;
}
