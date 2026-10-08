# tools —— 日志分析工具集

本目录存放课程要求中的 **shell 脚本**，与 C 程序配合完成
「日志记录 → 日志分析 → 成果共享」这一条链路。

| 脚本 | 作用 |
| --- | --- |
| `log_analyze.sh` | 分析 `log.log`：统计用户/课程修改次数、按学号与课程号检索、压缩归档日志、自动生成分析报告 |
| `share.sh` | 组内共享：用 wget / ftp 交换日志与压缩包 |

---

## 一、`log_analyze.sh` —— 日志分析脚本

### 用法

```bash
bash tools/log_analyze.sh [-f 日志文件] [-s 学号] [-c 课程号] [-d 归档目录] [-o 报告文件] [-q] [-h]
```

| 选项 | 说明 | 默认值 |
| --- | --- | --- |
| `-f <file>` | 指定日志文件 | `./log.log` |
| `-s <uid>` | 按学号（用户ID）检索用户的修改记录并统计次数 | 不检索 |
| `-c <cid>` | 按课程号检索课程的修改记录并统计次数 | 不检索 |
| `-d <dir>` | 日志压缩包归档目录（生成后拷贝到此目录并解压） | `~/log_archive` |
| `-o <file>` | 分析报告输出路径 | `Docs/log_report.md` |
| `-q` | 安静模式：只写文件，不把报告打到屏幕 | 关闭 |
| `-h` | 查看帮助 | — |

### 常用示例

```bash
# 1) 最简：分析默认日志，生成报告 + 归档
bash tools/log_analyze.sh

# 2) 指定学号 1006、课程号 2003 做定向检索
bash tools/log_analyze.sh -s 1006 -c 2003

# 3) 报告写到指定位置，安静模式（适合放进自动化流程）
bash tools/log_analyze.sh -o RELEASE/log_report.md -q
```

### 输出

1. **屏幕**：操作次数统计、按学号/课程号的检索明细、归档信息；
2. **报告文件**（默认 `Docs/log_report.md`）：可直接作为课程设计报告的一节；
3. **归档**：`$ARCHIVE_DIR/<日志名>_<时间戳>.tar.gz` 及其解压目录 `unpack_<时间戳>/`。

---

## 二、`share.sh` —— 日志/成果共享脚本

### 用法

```bash
bash tools/share.sh serve   [端口]                        # 本机开启共享服务（供别人 wget）
bash tools/share.sh get     <URL> [保存目录]              # 用 wget 从共享地址下载
bash tools/share.sh pack    [文件...]                     # 压缩待共享文件（默认打包 log.log）
bash tools/share.sh put-ftp <主机> <用户> <本地文件> [远端目录]
bash tools/share.sh get-ftp <主机> <用户> <远端文件> [本地目录]
bash tools/share.sh -h
```

### 方式一：wget 共享（推荐，零配置）

```bash
# 发送方（IP 假设为 192.168.1.20）
bash tools/share.sh serve 8000

# 接收方（组员 / 助教）
bash tools/share.sh get http://192.168.1.20:8000/log.log
bash tools/share.sh get http://192.168.1.20:8000/RELEASE/log_20261008_120000.tar.gz
```

### 方式二：ftp 共享（组内已有 FTP 服务器时）

```bash
# 上传
bash tools/share.sh put-ftp 192.168.1.30 student log.log /upload

# 下载
bash tools/share.sh get-ftp 192.168.1.30 student /upload/log.log RELEASE
```

> 说明：`put-ftp` / `get-ftp` 优先使用 `curl` 的 FTP 能力（无需交互，方便写进脚本）；
> 若本机没有 curl，会退化到系统 `ftp` 客户端并提示交互输入密码。

---

## 三、完整的一条链路

```bash
cd <工程根目录>
source myenv.sh file          # 定制产品形态
make clean && make all        # 编译
./learning_platform_file      # 运行程序，操作若干次（产生 log.log）

bash tools/log_analyze.sh -s 1006 -c 2003   # 分析日志 → Docs/log_report.md + 归档
bash tools/share.sh pack                    # 打包日志
bash tools/share.sh serve 8000              # 共享给组员/助教
```
