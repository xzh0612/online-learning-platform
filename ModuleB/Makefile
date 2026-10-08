#############################################################################
# ModuleB —— 课程管理模块 Makefile
#
# 职责：把本模块源代码编译成静态库 libModuleB.a，并拷贝到上级目录
#       供顶层 Makefile 链接使用。
#############################################################################

AR = ar
ARFLAGS = cr
RM = rm -rf

INCLUDE_DIR = ../Include
CFLAGS = -I"$(INCLUDE_DIR)" -Wall -g $(CFLAGS_EXTRA)
CC = gcc

LIB = libModuleB.a
SRC_DIR = src
OBJ_DIR = obj

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

all: $(LIB)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(LIB): $(OBJS)
	@if [ -n "$(OBJS)" ]; then \
		$(AR) $(ARFLAGS) $@ $(OBJS); \
		cp $(LIB) ../; \
		echo "[ModuleB] 生成 $@ 并拷贝到上级目录"; \
	else \
		echo "[ModuleB] 暂无源文件，跳过"; \
	fi

clean:
	$(RM) $(OBJ_DIR) $(LIB) ../$(LIB)
	@echo "[ModuleB] 清理完成"

.PHONY: all clean
