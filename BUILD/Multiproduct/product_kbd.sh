#!/bin/bash
#############################################################################
# BUILD/Multiproduct/product_kbd.sh
#
# 产品形态二：键盘录入
# 最终编译出的程序 learning_platform_kbd 不读取任何文件，
# 初始信息由程序运行时从键盘输入。
#############################################################################

BUILD_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
bash "$BUILD_DIR/../build.sh" kbd
