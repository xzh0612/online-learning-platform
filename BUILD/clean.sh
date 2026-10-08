#!/bin/bash
#############################################################################
# BUILD/clean.sh —— 彻底清理脚本
#
# 用法：bash BUILD/clean.sh
#
# 清理范围：各模块 obj/ 与静态库、根目录 .o/.a/.so/.out、
#           RELEASE/ 下的可执行文件、日志文件 log.log
#############################################################################
set -e

BUILD_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$BUILD_DIR")"

source "$PROJECT_ROOT/myenv.sh" file >/dev/null

cd "$PROJECT_ROOT"

echo "[clean] step1: 递归清理各模块编译产物"
make clean

echo "[clean] step2: 清理 RELEASE 与根目录的可执行文件、日志产物"
rm -f RELEASE/learning_platform_file RELEASE/learning_platform_kbd
rm -f learning_platform_file learning_platform_kbd learning_platform.out
rm -f RELEASE/*.out log.log RELEASE/log.log

echo "[clean] step3: 兜底清理散落的 .o 文件"
find . -name '*.o' -delete 2>/dev/null || true

echo "[clean] 清理完成"
