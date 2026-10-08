# 在线学习平台管理功能（Online Learning Platform）

Linux 程序开发基础 课程设计大作业。用 C 语言实现一个多模块的在线学习平台管理程序，
通过**编译期宏定制产品形态**，使同一套源代码可以编译出两种可执行程序：

- **产品形态一（文件录入）**：程序启动时从 `data/user.txt`、`data/course.txt` 读取初始信息；
- **产品形态二（键盘录入）**：程序启动时由用户从键盘逐条输入初始信息。

两种形态在初始数据载入之后的菜单与功能完全一致。

| 项目 | 内容 |
| --- | --- |
| 课程名称 | Linux 程序开发基础 |
| 专业班级 | 24 软件工程 Z 班 |
| 所属院部 | 软件工程学院 |
| 指导教师 | 孙菲艳 |

---

## 一、运行环境与依赖库

| 项目 | 要求 |
| --- | --- |
| 操作系统 | Linux（Ubuntu 20.04 及以上验证通过） |
| 编译器 | gcc，支持 C99 |
| 构建工具 | GNU make |
| 静态库工具 | ar（binutils 提供） |
| 调试工具 | gdb（用于调试记录与缺陷定位） |
| 外部依赖库 | **无**。密码哈希使用工程内自带的 SHA-256 实现，不依赖系统 `crypt` 库，也不依赖任何第三方库 |

依赖安装（Ubuntu）：

```bash
sudo apt update
sudo apt install -y build-essential gdb make
```

---

## 二、目录结构与命名映射

```
online-learning-platform/
├── Makefile              顶层 Makefile：调度各模块，按产品形态链接最终可执行文件
├── myenv.sh              产品形态定制脚本（source 后导出编译相关环境变量）
├── BUILD/                编译脚本目录
│   ├── build.sh          编译入口：定制形态 -> make clean -> make all -> 拷贝产物
│   ├── clean.sh          彻底清理脚本
│   └── Multiproduct/     各产品形态专属脚本
│       ├── product_file.sh
│       └── product_kbd.sh
├── Include/              各模块公用头文件，以及模块对外公开的接口声明
├── ModuleA/              用户管理模块（User Management）
│   ├── Makefile
│   ├── src/              模块源代码
│   └── obj/              模块编译中间产物（不纳入版本管理）
├── ModuleB/              课程管理模块（Course Management）
├── ModuleC/              数据录入（文件 / 键盘）与日志模块
├── RELEASE/              最终可执行文件与运行时产物
├── data/                 初始数据文件 user.txt、course.txt
├── tools/                日志分析等辅助脚本
├── Docs/                 课程设计报告
└── README.md
```

### 与通用工程分层命名的对应关系

本工程在满足课程要求目录框架（`BUILD/`、`RELEASE/`、`Include/`、`ModuleX/`）的同时，
遵循"源码、中间产物、头文件、编译脚本、可执行文件"相互分离的原则：

| 通用分层 | 本工程对应位置 | 说明 |
| --- | --- | --- |
| `include` | `Include/` | 所有公用头文件与跨模块接口声明 |
| `src` | `ModuleA/src/`、`ModuleB/src/`、`ModuleC/src/` | 各模块源代码，按模块分目录 |
| `obj` | `ModuleA/obj/`、`ModuleB/obj/`、`ModuleC/obj/` | 编译中间产物，已在 `.gitignore` 中忽略 |
| `bin` | `RELEASE/` | 最终可执行文件，以及库文件等编译中间结果 |
| `build` | `BUILD/`、`BUILD/Multiproduct/` | 编译脚本，含各产品形态专属脚本 |
| `Makefile` | 顶层 `Makefile` + 各模块 `Makefile` | 顶层负责调度，模块级负责生成静态库 |

各模块的 `Makefile` 负责把本模块代码编译成静态库 `libModuleX.a` 并拷贝到上级目录；
顶层 `Makefile` 负责递归调度各模块，并根据产品形态挑选库文件链接成最终可执行文件。

---

## 三、编译方法

### 方式一：一键脚本（推荐）

```bash
# 产品形态一：文件录入
bash BUILD/Multiproduct/product_file.sh

# 产品形态二：键盘录入
bash BUILD/Multiproduct/product_kbd.sh
```

### 方式二：按课程要求的标准三步

```bash
# step1: 删除 .o .a .so 等编译中间产物
make clean

# step2: 设置环境变量，定制产品形态
source myenv.sh file     # 或 source myenv.sh kbd

# step3: 编译得出可执行文件
make all
```

### 方式三：手动指定目标程序名

```bash
source myenv.sh file
make clean
make all TARGET=learning_platform_file
```

编译产物：

| 产品形态 | 编译宏 | 可执行文件 |
| --- | --- | --- |
| 文件录入 | `-DLOAD_FROM_FILE` | `RELEASE/learning_platform_file` |
| 键盘录入 | 无 | `RELEASE/learning_platform_kbd` |

> 产品形态的差异只体现在"初始信息来源"上，通过 `myenv.sh` 导出的 `CFLAGS_EXTRA`
> 宏定义在编译期决定，因此键盘录入版本的程序中不包含任何读取初始数据的代码路径。

---

## 四、清理

```bash
# 标准清理（顶层 Makefile 的伪目标）
make clean

# 彻底清理（含 RELEASE 下产物与日志文件）
bash BUILD/clean.sh
```

---

## 五、开发状态

| 阶段 | 负责人 | 内容 | 状态 |
| --- | --- | --- | --- |
| 工程框架 | CMO | 目录框架、顶层与模块 Makefile、编译脚本、README | 已完成 |
| 接口设计 | 项目经理 | `Include/` 下各模块对外接口声明 | 已完成 |
| 用户管理模块 | 开发者 A | `ModuleA` | 待开发 |
| 课程管理模块 | 开发者 B | `ModuleB` | 待开发 |
| 录入与日志模块 | 开发者 C | `ModuleC` | 待开发 |
| 集成联调 | 项目经理 | `Main.c`、样例数据、两形态联调 | 待进行 |

### 模块对外接口一览

| 头文件 | 声明内容 | 实现模块 |
| --- | --- | --- |
| `Include/common.h` | 返回码、字段长度、默认路径、内联工具函数 | 头文件（无实现单元） |
| `Include/link_pub.h` | 公共双向链表接口 | ModuleA |
| `Include/user_pub.h` | `User` 结构体、角色枚举、用户管理接口 | ModuleA |
| `Include/course_pub.h` | `Course` 结构体、状态枚举、课程管理接口 | ModuleB |
| `Include/io_pub.h` | 文件录入、键盘录入、数据回写接口 | ModuleC |
| `Include/log_pub.h` | 操作日志接口与操作类型枚举 | ModuleC |
| `Include/sha256_pub.h` | SHA-256 摘要与密码哈希接口 | ModuleC |

依赖关系（自底向上，无循环依赖）：

```
common.h (纯头文件)
   └── ModuleA  公共链表 + 用户管理
          └── ModuleB  课程管理（使用链表维护已选学生列表）
                 └── ModuleC  数据录入 + 日志 + SHA-256（使用两者构造数据对象）
```

