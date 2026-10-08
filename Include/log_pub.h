#ifndef _LOG_PUB_H_
#define _LOG_PUB_H_

/*
 * log_pub.h —— 日志服务（ModuleC）对外接口
 *
 * 课程要求：通过命令行修改信息时，需将修改同步到文件中，
 *           并记录操作日志到日志文件 log.log。
 *
 * 日志行格式（字段之间以 '|' 分隔，便于 awk 稳定解析）：
 *   YYYY-MM-DD HH:MM:SS | OP | ROLE | TARGET | OPERATOR | DETAIL
 * 例如：
 *   2026-10-09 14:23:01 | USER_MOD   | teacher | uid:1001 | admin | 修改用户名
 */

#include "common.h"

/* 操作类型：同时用于日志写入与后续日志分析统计 */
typedef enum {
    LOG_USER_ADD = 0,    /* 新增用户 */
    LOG_USER_MOD,        /* 修改用户信息 */
    LOG_USER_DEL,        /* 删除用户 */
    LOG_COURSE_ADD,      /* 新增课程 */
    LOG_COURSE_MOD,      /* 修改课程信息（含已选学生列表变更） */
    LOG_COURSE_DEL,      /* 删除课程 */
    LOG_DATA_LOAD,       /* 载入初始数据 */
    LOG_DATA_SAVE        /* 回写数据文件 */
} LogOpType;

/* 初始化日志服务，指定日志文件路径；成功返回 RES_OK */
int log_init(const char *path);

/* 关闭日志文件 */
void log_close(void);

/*
 * 记录一条操作日志。
 *   op            操作类型
 *   role          操作者角色字符串：student / teacher / admin
 *   target_id     目标标识字符串，格式 "uid:1001" 或 "cid:2001"
 *   operator_name 操作人（用户名，系统初始录入用 "admin"）
 *   detail        可读的操作说明
 * 成功返回 RES_OK；日志文件未初始化时直接返回 RES_ERR_IO。
 */
int log_record(LogOpType op, const char *role, const char *target_id,
               const char *operator_name, const char *detail);

/* 返回操作类型对应的短字符串（USER_ADD / USER_MOD / ...），供日志写入与脚本统计复用 */
const char *log_op_name(LogOpType op);

#endif /* _LOG_PUB_H_ */
