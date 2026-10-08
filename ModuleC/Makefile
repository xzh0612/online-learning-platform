#############################################################################
# ModuleC —— 数据录入与日志模块 Makefile
#
# 职责：把本模块源代码（文件录入、键盘录入、日志服务、SHA-256）
#       编译成静态库 libModuleC.a，并拷贝到上级目录供链接使用。
#############################################################################

AR = ar
ARFLAGS = cr
RM = rm -rf

INCLUDE_DIR = ../Include
CFLAGS = -I"$(INCLUDE_DIR)" -Wall -g $(CFLAGS_EXTRA)
CC = gcc

LIB = libModuleC.a
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
		echo "[ModuleC] 生成 $@ 并拷贝到上级目录"; \
	else \
		echo "[ModuleC] 暂无源文件，跳过"; \
	fi

clean:
	$(RM) $(OBJ_DIR) $(LIB) ../$(LIB)
	@echo "[ModuleC] 清理完成"

.PHONY: all clean
