#############################################################################
# 在线学习平台管理功能 —— 顶层 Makefile
#
# 职责：总体调度各模块的 makefile，并根据产品形态挑选库文件
#       链接成最终的可执行文件。
#
# 用法：
#   source myenv.sh file     # 定制产品形态（file / kbd）
#   make clean               # 删除 .o .a .so .out
#   make all                 # 编译出最终可执行文件
#############################################################################

ROOT ?= .
INCLUDE_DIR = $(ROOT)/Include
export INCLUDE_DIR

# 库文件列表随产品形态变化，由 myenv.sh 导出（继承教学工程的 LIBS_ENV 机制）
LIBS := $(LIBS_ENV)

CC = gcc
CFLAGS = -I"$(INCLUDE_DIR)" -Wall -g $(CFLAGS_EXTRA)
# CFLAGS_EXTRA 由 myenv.sh 导出（-DLOAD_FROM_FILE 等），必须 export 才能传入子模块
export CFLAGS_EXTRA

# 用相对路径列出子模块，避免工程路径含空格时被 shell 拆词
SUBDIRS := ModuleA ModuleB ModuleC

SOURCES  := $(wildcard *.c)
CHEADERS := $(wildcard *.h)
objects  := $(patsubst %.c,%.o,$(SOURCES))

RM = rm -rf
MAKE = make

# 默认目标名，由 myenv.sh 的 TARGET_NAME 覆盖
TARGET ?= learning_platform.out

all: $(TARGET)

# 根目录下的源文件（Main.c）编译成目标文件
$(objects): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

# 先递归编译各模块生成静态库，再链接成本产品形态的可执行文件
$(TARGET): $(objects)
	@for dir in $(SUBDIRS); do $(MAKE) -C "$$dir" all || exit 1; done
	@if [ -n "$(objects)" ]; then \
		$(CC) $(CFLAGS) $(objects) -o $(TARGET) $(LIBS); \
		echo "[make] 链接完成: $(TARGET)"; \
	else \
		echo "[make] 根目录暂无源文件，仅完成各模块库编译，跳过链接"; \
	fi

# 伪目标：清理所有编译中间产物
clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C "$$dir" clean || exit 1; done
	$(RM) $(objects) $(LIBS) $(TARGET) $(wildcard *.a) $(wildcard *.so) $(wildcard *.out)
	@echo "[make] 清理完成"

.PHONY: all clean
