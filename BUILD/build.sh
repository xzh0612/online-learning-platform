#!/bin/bash
#############################################################################
# BUILD/build.sh —— 工程编译入口
#
# 用法：bash BUILD/build.sh [file|kbd]
#
# 流程：定制产品形态 -> make clean -> make all -> 拷贝产物到 RELEASE/
#############################################################################
set -e

BUILD_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$BUILD_DIR")"

# step 1: 设置环境变量，定制产品形态
source "$PROJECT_ROOT/myenv.sh" "${1:-file}"

cd "$PROJECT_ROOT"

# step 2: 清理上一次的编译产物
echo "[build] step1: make clean"
make clean

# step 3: 编译整个工程
echo "[build] step2: make all"
make all TARGET="$TARGET_NAME"

# step 4: 把可执行文件集中到 RELEASE 目录
echo "[build] step3: 拷贝产物到 RELEASE/"
mkdir -p RELEASE
cp "$TARGET_NAME" RELEASE/

echo "[build] 编译完成 -> RELEASE/$TARGET_NAME"
