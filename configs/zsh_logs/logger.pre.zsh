#!/usr/bin/env zsh
# logger.zsh - 高级日志工具类
# 功能: 
#   1. 多级别日志 (DEBUG, INFO, WARN, ERROR)
#   2. 终端彩色输出
#   3. 自动日志轮转 (超过2MB自动归档)
#   4. 支持自定义日志文件路径
#   5. 显示调用函数名和行号

# ------------------------------------------------------------
# 配置区域 (用户可修改)
# ------------------------------------------------------------

# 默认日志级别 (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR)
LOG_LEVEL=${LOG_LEVEL:-1}

# 日志文件设置
LOG_ENABLED=true
LOG_FILE="${HOME}/.zsh_script.log"
MAX_LOG_SIZE=$((2 * 1024 * 1024))  # 2MB

# 终端颜色配置
COLOR_DEBUG="cyan"
COLOR_INFO="green"
COLOR_WARN="yellow"
COLOR_ERROR="red"
COLOR_TIMESTAMP="blue"
COLOR_CALLER="magenta"

# ------------------------------------------------------------
# 核心日志函数 (内部使用)
# ------------------------------------------------------------

# 检查并轮转日志文件
_log_rotate() {
    [[ ! -f "$LOG_FILE" ]] && return
    
    local file_size=$(wc -c < "$LOG_FILE" 2>/dev/null)
    [[ -z "$file_size" ]] && return
    
    if (( file_size > MAX_LOG_SIZE )); then
        local timestamp=$(date +"%Y%m%d_%H%M%S")
        local archive="${LOG_FILE}.${timestamp}.log"
        
        mv "$LOG_FILE" "$archive"
        print -P "%F{yellow}LOG ROTATED: $LOG_FILE -> $archive%f" >&2
    fi
}

# 获取调用者信息
_get_caller_info() {
    # 使用调用栈获取调用函数名
    local caller_func="main"
    if [[ ${#funcstack} -gt 2 ]]; then
        caller_func="${funcstack[2]}"
    fi
    
    # 获取行号 (zsh 特性)
    local caller_line=""
    if [[ -n "$ZSH_DEBUG_CMD" ]]; then
        caller_line=":$ZSH_DEBUG_CMD"
    fi
    
    echo "${caller_func}${caller_line}"
}

# 核心日志输出
_log_internal() {
    local level=$1
    local color=$2
    shift 2
    
    local timestamp=$(date +"%Y-%m-%d %H:%M:%S")
    local caller_info=$(_get_caller_info)
    local log_msg="[$timestamp] [$level] [$caller_info] $*"
    
    # 终端输出 (带颜色)
    print -P "%F{$COLOR_TIMESTAMP}[$timestamp]%f %F{$color}[$level]%f %F{$COLOR_CALLER}[$caller_info]%f $*" >&2
    
    # 文件输出 (无颜色)
    if $LOG_ENABLED; then
        _log_rotate
        echo "$log_msg" >> "$LOG_FILE"
    fi
}

# ------------------------------------------------------------
# 公共日志函数
# ------------------------------------------------------------

log_debug() {
    [[ $LOG_LEVEL -le 0 ]] && _log_internal "DEBUG" "$COLOR_DEBUG" "$@"
}

log_info() {
    [[ $LOG_LEVEL -le 1 ]] && _log_internal "INFO" "$COLOR_INFO" "$@"
}

log_warn() {
    [[ $LOG_LEVEL -le 2 ]] && _log_internal "WARN" "$COLOR_WARN" "$@"
}

log_error() {
    [[ $LOG_LEVEL -le 3 ]] && _log_internal "ERROR" "$COLOR_ERROR" "$@"
}

# 带成功状态的日志
log_success() {
    local msg="$@"
    print -P "%F{$COLOR_TIMESTAMP}[$(date +"%H:%M:%S")]%f %F{green}✔%f $msg" >&2
    log_info "SUCCESS: $msg"
}

# 带失败状态的日志
log_failure() {
    local msg="$@"
    print -P "%F{$COLOR_TIMESTAMP}[$(date +"%H:%M:%S")]%f %F{red}✘%f $msg" >&2
    log_error "FAILURE: $msg"
}

# 函数调用跟踪
log_enter() {
    [[ $LOG_LEVEL -le 0 ]] && _log_internal "DEBUG" "$COLOR_DEBUG" "--> $funcstack[1]()"
}

log_exit() {
    [[ $LOG_LEVEL -le 0 ]] && _log_internal "DEBUG" "$COLOR_DEBUG" "<-- $funcstack[1]() [$?]"
}

# ------------------------------------------------------------
# 工具函数
# ------------------------------------------------------------

# 设置日志级别
set_log_level() {
    case "$1" in
        debug|DEBUG) LOG_LEVEL=0 ;;
        info|INFO) LOG_LEVEL=1 ;;
        warn|WARN) LOG_LEVEL=2 ;;
        error|ERROR) LOG_LEVEL=3 ;;
        *) log_error "Invalid log level: $1. Valid options: debug, info, warn, error" ;;
    esac
}

# 查看日志文件
view_log() {
    if [[ -f "$LOG_FILE" ]]; then
        less -R "$LOG_FILE"
    else
        log_warn "Log file not found: $LOG_FILE"
    fi
}

# 清理旧日志
clean_logs() {
    local days=${1:-30}
    find "$(dirname "$LOG_FILE")" -name "$(basename "$LOG_FILE").*.log" -mtime +$days -exec rm -f {} \;
    log_info "Cleaned log files older than $days days"
}

# ------------------------------------------------------------
# 初始化
# ------------------------------------------------------------

# 确保日志目录存在
[[ ! -d "$(dirname "$LOG_FILE")" ]] && mkdir -p "$(dirname "$LOG_FILE")"

# 创建初始日志文件
$LOG_ENABLED && touch "$LOG_FILE"

# 输出初始化消息
log_debug "Logger initialized. Log level: $LOG_LEVEL, Log file: $LOG_FILE"
