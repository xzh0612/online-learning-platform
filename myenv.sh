#!/bin/bash
#############################################################################
# myenv.sh —— 产品形态定制脚本
#
# 用法：source myenv.sh [file|kbd]
#   file —— 产品形态一：程序启动时从 user.txt / course.txt 读取初始信息
#   kbd  —— 产品形态二：程序启动时从键盘输入初始信息
#
# 说明：本脚本只负责"设置环境变量"，不负责编译；编译由顶层 Makefile 完成。
#############################################################################

# 1. 产品形态（缺省为 file）
export PRODUCT_TYPE="${1:-file}"

# 2. 工程根目录：按本脚本自身所在位置动态推导，不写死绝对路径
export ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export INCLUDE_DIR="$ROOT/Include"

# 3. 产品形态相关的库文件列表（顶层 Makefile 通过 LIBS_ENV 链接）
export LIBS_ENV="libModuleA.a libModuleB.a libModuleC.a"

# 4. 编译期宏与目标程序名
if [ "$PRODUCT_TYPE" = "file" ]; then
    export CFLAGS_EXTRA="-DLOAD_FROM_FILE"
    export TARGET_NAME="learning_platform_file"
else
    export CFLAGS_EXTRA=""
    export TARGET_NAME="learning_platform_kbd"
fi

echo "------------------------------------------------------------"
echo " 工程根目录 : $ROOT"
echo " 产品形态   : $PRODUCT_TYPE"
echo " 链接库     : $LIBS_ENV"
echo " 编译宏     : ${CFLAGS_EXTRA:-（无）}"
echo " 目标程序   : $TARGET_NAME"
echo "------------------------------------------------------------"
