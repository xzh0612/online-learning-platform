#!/bin/bash
#############################################################################
# log_analyze.sh —— 操作日志分析脚本（工具集 / tools）
#
# 对应课程要求：
#   1. 编写 shell 脚本对日志文件 log.log 进行分析；
#   2. 统计"用户信息修改次数"与"课程信息修改次数"；
#   3. 按"学号（用户ID）"查询该用户的修改记录与次数；
#   4. 按"课程号"查询该课程的修改记录与次数；
#   5. 把日志文件压缩后拷贝到个人目录并解压；
#   6. 自动生成一份日志分析报告。
#
# 用法：
#   bash tools/log_analyze.sh [-f 日志文件] [-s 学号] [-c 课程号]
#                             [-d 归档目录] [-o 报告文件] [-q] [-h]
#
# 选项说明：
#   -f <file>  指定日志文件，默认 ./log.log
#   -s <uid>   按学号（用户ID）检索用户信息修改记录并统计次数
#   -c <cid>   按课程号检索课程信息修改记录并统计次数
#   -d <dir>   归档目录：把日志文件压缩后拷贝到该目录并解压，默认 ~/log_archive
#   -o <file>  分析报告输出路径，默认 Docs/log_report.md
#   -q         安静模式：不把报告同时打印到屏幕
#   -h         显示帮助
#
# 依赖：awk、grep、tar、date（Linux 系统自带，无需额外安装）
#############################################################################

set -u

# ------------------------- 默认参数 -------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

LOG_FILE="$PROJECT_ROOT/log.log"
SID=""                       # 学号 / 用户ID
CID=""                       # 课程号
ARCHIVE_DIR="$HOME/log_archive"
REPORT_FILE="$PROJECT_ROOT/Docs/log_report.md"
QUIET=0

# ------------------------- 帮助 -------------------------
usage()
{
    sed -n '2,30p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
    exit 0
}

# ------------------------- 解析命令行 -------------------------
while getopts ":f:s:c:d:o:qh" opt; do
    case "$opt" in
        f) LOG_FILE="$OPTARG" ;;
        s) SID="$OPTARG" ;;
        c) CID="$OPTARG" ;;
        d) ARCHIVE_DIR="$OPTARG" ;;
        o) REPORT_FILE="$OPTARG" ;;
        q) QUIET=1 ;;
        h) usage ;;
        \?) echo "未知选项: -$OPTARG（用 -h 查看帮助）" >&2; exit 1 ;;
        :)  echo "选项 -$OPTARG 缺少参数（用 -h 查看帮助）" >&2; exit 1 ;;
    esac
done

# 日志文件不存在则直接退出
if [ ! -f "$LOG_FILE" ]; then
    echo "[错误] 日志文件不存在: $LOG_FILE" >&2
    echo "       请先在工程根目录运行本程序产生操作日志，或用 -f 指定日志文件。" >&2
    exit 1
fi

# ------------------------- 公共 awk 片段 -------------------------
# 日志行格式（见 Include/log_pub.h）：
#   时间 | OP | ROLE | TARGET | OPERATOR | DETAIL
# 以 '|' 为分隔符，$1..$6 分别对应上述 6 个字段，取值时顺手去掉两端空格。
AWK_TRIM='
function trim(s) { gsub(/^[ \t]+|[ \t]+$/, "", s); return s }
'

# =========================================================================
# 1. 基础信息
# =========================================================================
TOTAL_LINES=$(awk 'NF' "$LOG_FILE" | wc -l | tr -d ' ')
# 报告里若能用相对工程根目录的路径，会更简洁、换台机器也能读懂
DISPLAY_LOG="${LOG_FILE#"$PROJECT_ROOT"/}"
FIRST_TIME=$(awk -F'|' 'NF>=6 { t=trim($1); if (t != "") { print t; exit } }
                       function trim(s){ gsub(/^[ \t]+|[ \t]+$/,"",s); return s }' "$LOG_FILE")
LAST_TIME=$(awk -F'|' 'NF>=6 { t=trim($1); if (t != "") last = t }
                      END { print last }
                      function trim(s){ gsub(/^[ \t]+|[ \t]+$/,"",s); return s }' "$LOG_FILE")

echo "=============================================================="
echo " 在线学习平台 —— 操作日志分析"
echo "=============================================================="
echo " 日志文件 : $DISPLAY_LOG"
echo " 日志行数 : $TOTAL_LINES"
echo " 时间范围 : ${FIRST_TIME:-无}  ~  ${LAST_TIME:-无}"
echo " 分析时间 : $(date '+%Y-%m-%d %H:%M:%S')"
echo "--------------------------------------------------------------"

# =========================================================================
# 2. 统计各类操作次数
#    需求中的"用户信息修改次数""课程信息修改次数"即分别对应
#    USER_MOD、COURSE_MOD（增删单独统计，便于区分口径）。
# =========================================================================
STATS=$(awk -F'|' "$AWK_TRIM"'
NF >= 6 {
    op = trim($2)
    if (op == "") next
    cnt[op]++
}
END {
    for (k in cnt) printf "%s %d\n", k, cnt[k]
}' "$LOG_FILE")

get_count()
{
    local op="$1"
    local n
    n=$(printf '%s\n' "$STATS" | awk -v o="$op" '$1 == o { print $2 }')
    echo "${n:-0}"
}

N_USER_ADD=$(get_count USER_ADD)
N_USER_MOD=$(get_count USER_MOD)
N_USER_DEL=$(get_count USER_DEL)
N_COURSE_ADD=$(get_count COURSE_ADD)
N_COURSE_MOD=$(get_count COURSE_MOD)
N_COURSE_DEL=$(get_count COURSE_DEL)
N_DATA_LOAD=$(get_count DATA_LOAD)
N_DATA_SAVE=$(get_count DATA_SAVE)

USER_TOTAL=$((N_USER_ADD + N_USER_MOD + N_USER_DEL))
COURSE_TOTAL=$((N_COURSE_ADD + N_COURSE_MOD + N_COURSE_DEL))

echo "【1】操作次数统计"
printf "   %-14s %s\n" "USER_ADD"   "$N_USER_ADD"
printf "   %-14s %s\n" "USER_MOD"   "$N_USER_MOD"
printf "   %-14s %s\n" "USER_DEL"   "$N_USER_DEL"
printf "   %-14s %s\n" "COURSE_ADD" "$N_COURSE_ADD"
printf "   %-14s %s\n" "COURSE_MOD" "$N_COURSE_MOD"
printf "   %-14s %s\n" "COURSE_DEL" "$N_COURSE_DEL"
printf "   %-14s %s\n" "DATA_LOAD"  "$N_DATA_LOAD"
printf "   %-14s %s\n" "DATA_SAVE"  "$N_DATA_SAVE"
echo "   ----------------------------------------------------------"
echo "   用户信息修改次数（USER_MOD）  : $N_USER_MOD"
echo "   课程信息修改次数（COURSE_MOD）: $N_COURSE_MOD"
echo "   用户类操作合计                : $USER_TOTAL"
echo "   课程类操作合计                : $COURSE_TOTAL"

# =========================================================================
# 3. 按学号（用户ID）查询
# =========================================================================
SID_REPORT=""
if [ -n "$SID" ]; then
    TARGET="uid:$SID"
    SID_LINES=$(awk -F'|' -v t="$TARGET" "$AWK_TRIM"'
        NF >= 6 && trim($4) == t { print }' "$LOG_FILE")
    SID_CNT=$(printf '%s' "$SID_LINES" | awk 'NF' | wc -l | tr -d ' ')
    SID_MOD=$(printf '%s\n' "$SID_LINES" | awk -F'|' "$AWK_TRIM"'
        NF >= 6 && trim($2) == "USER_MOD" { n++ } END { print n + 0 }')

    echo "--------------------------------------------------------------"
    echo "【2】按学号（用户ID=$SID）查询"
    echo "   相关记录条数            : $SID_CNT"
    echo "   其中 USER_MOD（信息修改）: $SID_MOD"
    if [ "$SID_CNT" -gt 0 ]; then
        echo "   明细："
        printf '%s\n' "$SID_LINES" | sed 's/^/     /'
    else
        echo "   （未找到该学号的操作记录）"
    fi

    SID_REPORT=$(cat <<EOF
### 按学号查询：用户ID = $SID

- 相关记录条数：**$SID_CNT**
- 其中用户信息修改次数（USER_MOD）：**$SID_MOD**

EOF
)
    if [ "$SID_CNT" -gt 0 ]; then
        SID_REPORT="${SID_REPORT}

| 时间 | 操作 | 目标 | 操作人 | 说明 |
| --- | --- | --- | --- | --- |
"
        SID_REPORT="${SID_REPORT}$(printf '%s\n' "$SID_LINES" | awk -F'|' "$AWK_TRIM"'
            NF >= 6 {
                printf "| %s | %s | %s | %s | %s |\n", trim($1), trim($2), trim($4), trim($5), trim($6)
            }')"
        SID_REPORT="${SID_REPORT}
"
    fi
fi

# =========================================================================
# 4. 按课程号查询
# =========================================================================
CID_REPORT=""
if [ -n "$CID" ]; then
    TARGET="cid:$CID"
    CID_LINES=$(awk -F'|' -v t="$TARGET" "$AWK_TRIM"'
        NF >= 6 && trim($4) == t { print }' "$LOG_FILE")
    CID_CNT=$(printf '%s' "$CID_LINES" | awk 'NF' | wc -l | tr -d ' ')
    CID_MOD=$(printf '%s\n' "$CID_LINES" | awk -F'|' "$AWK_TRIM"'
        NF >= 6 && trim($2) == "COURSE_MOD" { n++ } END { print n + 0 }')

    echo "--------------------------------------------------------------"
    echo "【3】按课程号（课程ID=$CID）查询"
    echo "   相关记录条数              : $CID_CNT"
    echo "   其中 COURSE_MOD（内容修改）: $CID_MOD"
    if [ "$CID_CNT" -gt 0 ]; then
        echo "   明细："
        printf '%s\n' "$CID_LINES" | sed 's/^/     /'
    else
        echo "   （未找到该课程号的操作记录）"
    fi

    CID_REPORT=$(cat <<EOF
### 按课程号查询：课程ID = $CID

- 相关记录条数：**$CID_CNT**
- 其中课程信息修改次数（COURSE_MOD）：**$CID_MOD**

EOF
)
    if [ "$CID_CNT" -gt 0 ]; then
        CID_REPORT="${CID_REPORT}

| 时间 | 操作 | 目标 | 操作人 | 说明 |
| --- | --- | --- | --- | --- |
"
        CID_REPORT="${CID_REPORT}$(printf '%s\n' "$CID_LINES" | awk -F'|' "$AWK_TRIM"'
            NF >= 6 {
                printf "| %s | %s | %s | %s | %s |\n", trim($1), trim($2), trim($4), trim($5), trim($6)
            }')"
        CID_REPORT="${CID_REPORT}
"
    fi
fi

# =========================================================================
# 5. 日志文件压缩归档：压缩后拷贝到个人目录并解压
# =========================================================================
echo "--------------------------------------------------------------"
echo "【4】日志文件压缩与归档"
STAMP=$(date '+%Y%m%d_%H%M%S')
BASE=$(basename "$LOG_FILE")
PACK_NAME="${BASE%.log}_${STAMP}.tar.gz"

mkdir -p "$ARCHIVE_DIR"
# 只把日志文件本身打进包里，避免把整个目录树带进去
tar -czf "$ARCHIVE_DIR/$PACK_NAME" -C "$(dirname "$LOG_FILE")" "$BASE"
PACK_SIZE=$(du -h "$ARCHIVE_DIR/$PACK_NAME" | awk '{print $1}')

# 拷贝到个人目录（归档目录本身即个人目录）后解压
UNPACK_DIR="$ARCHIVE_DIR/unpack_${STAMP}"
mkdir -p "$UNPACK_DIR"
tar -xzf "$ARCHIVE_DIR/$PACK_NAME" -C "$UNPACK_DIR"

echo "   压缩包   : $ARCHIVE_DIR/$PACK_NAME （$PACK_SIZE）"
echo "   解压目录 : $UNPACK_DIR"
echo "   解压结果 : $(ls -1 "$UNPACK_DIR" | tr '\n' ' ')"
echo "   校验     : 原始 $TOTAL_LINES 行 / 解压后 $(awk 'NF' "$UNPACK_DIR/$BASE" | wc -l | tr -d ' ') 行"

# =========================================================================
# 6. 生成分析报告
# =========================================================================
REPORT_DIR="$(dirname "$REPORT_FILE")"
[ -d "$REPORT_DIR" ] || mkdir -p "$REPORT_DIR"

TOPN_TABLE=$(awk -F'|' "$AWK_TRIM"'
NF >= 6 && trim($4) ~ /^uid:/ {
    u = trim($4)
    sub(/^uid:/, "", u)
    cnt[u]++
}
END {
    for (k in cnt) printf "%s %d\n", k, cnt[k]
}' "$LOG_FILE" | sort -k2,2nr | awk '{ printf "| %s | %s |\n", $1, $2 }')

TOP_COURSE_TABLE=$(awk -F'|' "$AWK_TRIM"'
NF >= 6 && trim($4) ~ /^cid:/ {
    c = trim($4)
    sub(/^cid:/, "", c)
    cnt[c]++
}
END {
    for (k in cnt) printf "%s %d\n", k, cnt[k]
}' "$LOG_FILE" | sort -k2,2nr | awk '{ printf "| %s | %s |\n", $1, $2 }')

{
    echo "# 操作日志分析报告"
    echo
    echo "> 由 \`tools/log_analyze.sh\` 自动生成，请勿手工修改。"
    echo
    echo "- 日志文件：\`$DISPLAY_LOG\`"
    echo "- 日志行数：$TOTAL_LINES"
    echo "- 时间范围：${FIRST_TIME:-无} ~ ${LAST_TIME:-无}"
    echo "- 生成时间：$(date '+%Y-%m-%d %H:%M:%S')"
    echo "- 主机/用户：$(hostname) / ${USER:-$(id -un)}"
    echo
    echo "## 一、操作次数统计"
    echo
    echo "| 操作类型 | 含义 | 次数 |"
    echo "| --- | --- | --- |"
    echo "| USER_ADD | 新增用户 | $N_USER_ADD |"
    echo "| USER_MOD | **修改用户信息** | **$N_USER_MOD** |"
    echo "| USER_DEL | 删除用户 | $N_USER_DEL |"
    echo "| COURSE_ADD | 新增课程 | $N_COURSE_ADD |"
    echo "| COURSE_MOD | **修改课程信息** | **$N_COURSE_MOD** |"
    echo "| COURSE_DEL | 删除课程 | $N_COURSE_DEL |"
    echo "| DATA_LOAD | 载入初始数据 | $N_DATA_LOAD |"
    echo "| DATA_SAVE | 回写数据文件 | $N_DATA_SAVE |"
    echo
    echo "- 用户信息修改次数（USER_MOD）：**$N_USER_MOD**"
    echo "- 课程信息修改次数（COURSE_MOD）：**$N_COURSE_MOD**"
    echo "- 用户类操作合计：$USER_TOTAL　课程类操作合计：$COURSE_TOTAL"
    echo
    echo "## 二、按目标对象聚合"
    echo
    echo "### 2.1 被操作用户（按次数降序）"
    echo
    echo "| 用户ID | 操作次数 |"
    echo "| --- | --- |"
    if [ -n "$TOPN_TABLE" ]; then printf '%s\n' "$TOPN_TABLE"; else echo "| （无） | 0 |"; fi
    echo
    echo "### 2.2 被操作课程（按次数降序）"
    echo
    echo "| 课程ID | 操作次数 |"
    echo "| --- | --- |"
    if [ -n "$TOP_COURSE_TABLE" ]; then printf '%s\n' "$TOP_COURSE_TABLE"; else echo "| （无） | 0 |"; fi
    echo
    if [ -n "$SID_REPORT" ]; then
        echo "## 三、按学号检索明细"
        echo
        printf '%s\n' "$SID_REPORT"
    fi
    if [ -n "$CID_REPORT" ]; then
        echo "## 四、按课程号检索明细"
        echo
        printf '%s\n' "$CID_REPORT"
    fi
    echo "## 附：日志文件压缩归档"
    echo
    echo "- 压缩包：\`$ARCHIVE_DIR/$PACK_NAME\`"
    echo "- 解压目录：\`$UNPACK_DIR\`"
} > "$REPORT_FILE"

echo "--------------------------------------------------------------"
echo "【5】分析报告已生成"
echo "   报告文件 : $REPORT_FILE"
echo "   报告行数 : $(wc -l < "$REPORT_FILE" | tr -d ' ')"
echo "=============================================================="

if [ "$QUIET" -eq 0 ]; then
    echo
    echo "--------------------- 报告内容预览 ---------------------"
    cat "$REPORT_FILE"
fi

exit 0
