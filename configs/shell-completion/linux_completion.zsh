is_linux() {
    # 检查是否不是 Termux 且内核是 Linux
    ! is_termux && [[ $(uname -s) == "Linux" ]]
}

is_termux() {
    [[ -d "/data/data/com.termux/files/usr" ]] || 
    [[ -n "$TERMUX_VERSION" ]] || 
    [[ "$PREFIX" == "/data/data/com.termux/files/usr"* ]] ||
    { [[ $(uname -o) == "Android" ]] && command -v pkg >/dev/null; }
}

if ! is_linux; then
    return
fi
if ! command -v fzf &>/dev/null; then
    echo "警告: 请先安装 fzf: apt install fzf"
fi

local M_COMPLETION_DIR="${0:A:h}"
source ${M_COMPLETION_DIR}/base_completion.sh
FZF_VI_OPTS="
  --height 40% 
  --reverse 
"
fzf-history-widget() {
    # 保存原始状态
    local original_buffer=$BUFFER
    local original_cursor=$CURSOR
    local selected
    local delimiter=$'\x1f'

    # 使用 Zsh 内部历史数组避免外部命令
    local -a history_items
    local hist_limit=2000  # 限制历史记录数量

    # 准备历史记录数据
    if [[ -o extendedhistory ]]; then
        # 带时间戳的历史记录
        history_items=(${(f)"$(history -i 1 | tail -n $hist_limit)"})
    else
        # 普通历史记录
        history_items=(${(f)"$(history 1 | tail -n $hist_limit)"})
    fi

    # 创建临时文件存储处理后的历史
    local temp_file=$(mktemp)
    trap "rm -f '$temp_file'" EXIT

    # 使用 Zsh 内置处理避免外部命令
    for item in "${history_items[@]}"; do
        if [[ -o extendedhistory ]]; then
            # 带时间戳的处理
            local cmd=${item#*[0-9][0-9]:[0-9][0-9] }  # 移除时间戳
            cmd=${cmd#* }  # 移除日期
            cmd=${cmd#* }  # 移除时间
            local full=${item#* }  # 移除序号
            print -r -- "${cmd}${delimiter}${full}"
        else
            # 普通历史处理
            local cmd=${item#* }
            cmd=${cmd#* }  # 移除序号
            print -r -- "${cmd}${delimiter}${item}"
        fi
    done > "$temp_file"

    # 使用非阻塞方式调用 fzf
    selected=$(
        fzf --height 40% --reverse --tac --query="$LBUFFER" \
            --delimiter="$delimiter" \
            --with-nth=1 \
            --preview="printf %s {2}" \
            --preview-window='up:3:wrap:nohidden' \
            --bind='ctrl-c:abort' \
            --no-sort \
            < "$temp_file" ${=FZF_CTRL_R_OPTS}
    )

    # 处理选择结果
    if [[ -n "$selected" ]]; then
        BUFFER="${selected%%$delimiter*}"
        CURSOR=$#BUFFER
    else
        BUFFER=$original_buffer
        CURSOR=$original_cursor
    fi

    # 确保终端状态恢复
    zle reset-prompt
    zle redisplay
}

fzf-history-widget1() {
    local original_buffer=$BUFFER
    local original_cursor=$CURSOR
    local selected
    local delimiter=$'\x1f'  # ASCII unit separator

    if [[ -o extendedhistory ]]; then
        # 带时间戳的历史记录处理
        selected=$(fc -il 1 | 
            awk -v d="$delimiter" '{
                # 提取命令部分（去掉序号、日期和时间）
                cmd = $0;
                sub(/^[[:space:]]*[0-9]+[[:space:]]+/, "", cmd);  # 移除序号
                sub(/^[^[:space:]]+[[:space:]]+[^[:space:]]+[[:space:]]+/, "", cmd);  # 移除日期和时间
                # 提取完整历史行（含时间戳）
                full = $0;
                sub(/^[[:space:]]*[0-9]+[[:space:]]+/, "", full);  # 移除序号，保留时间戳和命令
                # 输出：命令部分 + 分隔符 + 完整历史行
                print cmd d full;
            }' |
            fzf --height 40% --reverse --tac --query="$LBUFFER" \
                --delimiter="$delimiter" \
                --with-nth=1 \
                --preview="echo -e {2}" \
                --preview-window=up:3:wrap ${=FZF_CTRL_R_OPTS})
    else
        # 普通历史记录处理
        selected=$(fc -rl 1 | 
            awk -v d="$delimiter" '{
                # 提取命令部分（去掉序号）
                cmd = substr($0, index($0,$2));
                # 输出：命令部分 + 分隔符 + 完整行
                print cmd d $0;
            }' |
            fzf --height 40% --reverse --tac --query="$LBUFFER" \
                --delimiter="$delimiter" \
                --with-nth=1 \
                --preview="echo -e {2}" \
                --preview-window=up:3:wrap ${=FZF_CTRL_R_OPTS})
    fi

    if [[ -n "$selected" ]]; then
        # 只取命令部分（分隔符前的内容）
        BUFFER="${selected%%$delimiter*}"
        CURSOR=$#BUFFER
    else
        BUFFER=$original_buffer
        CURSOR=$original_cursor
    fi

    zle redisplay
}
# 公共部分（两种平台都适用）
zle -N fzf-history-widget
bindkey '^R' fzf-history-widget

