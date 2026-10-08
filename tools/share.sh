#!/bin/bash
#############################################################################
# share.sh —— 日志/成果文件共享脚本（工具集 / tools）
#
# 对应课程要求：
#   组内成员之间、以及与助教之间，需要通过 wget 或 ftp 的方式
#   交换日志文件与分析结果。
#
# 用法：
#   bash tools/share.sh serve [端口]                      # 本机开启共享服务（供别人 wget）
#   bash tools/share.sh get   <URL> [保存目录]            # 用 wget 从共享地址下载
#   bash tools/share.sh pack  [文件...]                   # 压缩待共享文件（默认打包 log.log）
#   bash tools/share.sh put-ftp  <主机> <用户> <本地文件> [远端目录]
#   bash tools/share.sh get-ftp  <主机> <用户> <远端文件> [本地目录]
#   bash tools/share.sh -h
#
# 说明：
#   1. wget 方式最省事——一方 serve，另一方 get 即可，无需搭建任何服务；
#   2. ftp 方式需要组内已有一台 FTP 服务器（如 vsftpd），命令按匿名/实名方式登录；
#   3. 若本机没有 wget，脚本会自动降级用 curl 完成同样的下载。
#############################################################################

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
LOG_FILE="$PROJECT_ROOT/log.log"
RELEASE_DIR="$PROJECT_ROOT/RELEASE"

usage()
{
    sed -n '2,26p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
    exit 0
}

# 选择可用的下载工具：优先 wget，退化到 curl
pick_downloader()
{
    if command -v wget >/dev/null 2>&1; then
        echo "wget"
    elif command -v curl >/dev/null 2>&1; then
        echo "curl"
    else
        echo ""
    fi
}

# 选择可用的 HTTP 静态服务：优先 python3，退化到 python2
pick_httpserver()
{
    if command -v python3 >/dev/null 2>&1; then
        echo "python3 -m http.server"
    elif command -v python >/dev/null 2>&1; then
        echo "python -m SimpleHTTPServer"
    else
        echo ""
    fi
}

# ------------------------- 子命令：serve -------------------------
cmd_serve()
{
    local port="${1:-8000}"
    local srv
    srv="$(pick_httpserver)"

    if [ -z "$srv" ]; then
        echo "[错误] 未找到 python3/python，无法启动 HTTP 共享服务。" >&2
        echo "       可改用 ftp 方式：bash tools/share.sh put-ftp <主机> <用户> <文件>" >&2
        exit 1
    fi

    local ip
    ip="$(hostname -I 2>/dev/null | awk '{print $1}')"
    [ -n "$ip" ] || ip="$(hostname)"

    echo "=============================================================="
    echo " 日志共享服务已启动（wget 方式）"
    echo " 服务目录 : $PROJECT_ROOT"
    echo " 监听端口 : $port"
    echo " 组员下载 : wget http://$ip:$port/log.log"
    echo " 按 Ctrl+C 停止服务"
    echo "=============================================================="

    cd "$PROJECT_ROOT" || exit 1
    # shellcheck disable=SC2086
    exec $srv "$port"
}

# ------------------------- 子命令：get -------------------------
cmd_get()
{
    local url="${1:-}"
    local dir="${2:-$PROJECT_ROOT/RELEASE}"
    local tool

    if [ -z "$url" ]; then
        echo "[错误] 请给出下载地址，例如：bash tools/share.sh get http://192.168.1.20:8000/log.log" >&2
        exit 1
    fi

    tool="$(pick_downloader)"
    if [ -z "$tool" ]; then
        echo "[错误] 本机既没有 wget 也没有 curl，无法下载。" >&2
        exit 1
    fi

    mkdir -p "$dir"
    echo "[共享] 使用 $tool 下载：$url"
    if [ "$tool" = "wget" ]; then
        # -c 断点续传，-P 指定保存目录
        wget -c -P "$dir" "$url"
    else
        ( cd "$dir" && curl -L -O -C - "$url" )
    fi
    local rc=$?

    if [ "$rc" -eq 0 ]; then
        echo "[共享] 下载完成，保存于：$dir"
        ls -lh "$dir" | tail -n +2
    else
        echo "[错误] 下载失败（返回码 $rc）。" >&2
    fi
    return "$rc"
}

# ------------------------- 子命令：pack -------------------------
cmd_pack()
{
    local stamp base name
    stamp="$(date '+%Y%m%d_%H%M%S')"
    mkdir -p "$RELEASE_DIR"

    local files=("$@")
    if [ "${#files[@]}" -eq 0 ]; then
        if [ ! -f "$LOG_FILE" ]; then
            echo "[错误] 未找到日志文件 $LOG_FILE，请先运行程序或指定要打包的文件。" >&2
            exit 1
        fi
        files=("$LOG_FILE")
    fi

    # 以第一个文件的名字作为包名，便于区分
    base="$(basename "${files[0]}")"
    name="${base%%.*}_${stamp}.tar.gz"

    # 先把待共享文件集中到临时目录，再整体打包，
    # 这样压缩包内是干净的文件名（不含绝对路径），对方解压即用。
    local stage
    stage="$(mktemp -d)"
    # 无论正常结束还是中途 exit，退出时都清掉临时目录；
    # 用 ${stage:-} 是为了兼容 set -u（退出时局部变量已出作用域）
    trap 'rm -rf "${stage:-}"' EXIT

    local n=0
    for f in "${files[@]}"; do
        if [ ! -e "$f" ]; then
            echo "[警告] 跳过不存在的文件：$f" >&2
            continue
        fi
        cp -R "$f" "$stage/"
        n=$((n + 1))
    done

    if [ "$n" -eq 0 ]; then
        echo "[错误] 打包失败：没有可用文件。" >&2
        exit 1
    fi

    tar -czf "$RELEASE_DIR/$name" -C "$stage" .

    echo "[共享] 已打包：$RELEASE_DIR/$name （$(du -h "$RELEASE_DIR/$name" | awk '{print $1}')，含 $n 个文件）"
    echo "       发送给对方：bash tools/share.sh serve 8000"
    echo "       对方接收  ：bash tools/share.sh get http://<你的IP>:8000/RELEASE/$name"
}

# ------------------------- 子命令：put-ftp -------------------------
cmd_put_ftp()
{
    local host="${1:-}" user="${2:-}" localfile="${3:-}" remotedir="${4:-/}"

    if [ -z "$host" ] || [ -z "$user" ] || [ -z "$localfile" ]; then
        echo "[错误] 参数不足：put-ftp <主机> <用户> <本地文件> [远端目录]" >&2
        exit 1
    fi
    if [ ! -f "$localfile" ]; then
        echo "[错误] 本地文件不存在：$localfile" >&2
        exit 1
    fi

    # 优先用 curl 的 ftp 上传（无需交互，便于写进脚本）；
    # 没有 curl 时退化到系统 ftp 客户端。
    if command -v curl >/dev/null 2>&1; then
        echo "[共享] curl ftp 上传：$localfile -> ftp://$host$remotedir"
        curl -T "$localfile" "ftp://$host$remotedir" --user "$user"
    else
        echo "[共享] ftp 客户端上传：$localfile -> $host:$remotedir（需要交互输入密码）"
        ftp -n "$host" <<EOF
user $user
cd $remotedir
put $localfile
bye
EOF
    fi
    echo "[共享] FTP 上传结束。"
}

# ------------------------- 子命令：get-ftp -------------------------
cmd_get_ftp()
{
    local host="${1:-}" user="${2:-}" remotefile="${3:-}" localdir="${4:-$RELEASE_DIR}"

    if [ -z "$host" ] || [ -z "$user" ] || [ -z "$remotefile" ]; then
        echo "[错误] 参数不足：get-ftp <主机> <用户> <远端文件> [本地目录]" >&2
        exit 1
    fi

    mkdir -p "$localdir"

    if command -v curl >/dev/null 2>&1; then
        echo "[共享] curl ftp 下载：ftp://$host/$remotefile -> $localdir"
        ( cd "$localdir" && curl -O "ftp://$host/$remotefile" --user "$user" )
    else
        echo "[共享] ftp 客户端下载：$host:$remotefile -> $localdir（需要交互输入密码）"
        ( cd "$localdir" && ftp -n "$host" <<EOF
user $user
binary
get $remotefile
bye
EOF
        )
    fi
    echo "[共享] FTP 下载结束，文件位于：$localdir"
}

# ------------------------- 主入口 -------------------------
case "${1:-}" in
    serve)   shift; cmd_serve "$@" ;;
    get)     shift; cmd_get "$@" ;;
    pack)    shift; cmd_pack "$@" ;;
    put-ftp) shift; cmd_put_ftp "$@" ;;
    get-ftp) shift; cmd_get_ftp "$@" ;;
    -h|--help|help|"") usage ;;
    *)
        echo "[错误] 未知子命令：$1" >&2
        echo "       可用子命令：serve / get / pack / put-ftp / get-ftp" >&2
        exit 1
        ;;
esac
