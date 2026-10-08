#!/bin/bash
#############################################################################
# BUILD/Multiproduct/product_file.sh
#
# 产品形态一：文件录入
# 最终编译出的程序 learning_platform_file 从 data/user.txt、
# data/course.txt 读取初始信息后进入内存链表处理。
#############################################################################

BUILD_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
bash "$BUILD_DIR/../build.sh" file
