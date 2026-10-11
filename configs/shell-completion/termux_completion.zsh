is_termux() {
    [[ -d "/data/data/com.termux/files/usr" ]] || 
    [[ -n "$TERMUX_VERSION" ]] || 
    [[ "$PREFIX" == "/data/data/com.termux/files/usr"* ]] ||
    { [[ $(uname -o) == "Android" ]] && command -v pkg >/dev/null; }
}

if ! is_termux ; then
   return 
fi

if ! command -v fzf &>/dev/null; then
    echo "警告: 请先安装 fzf: pkg install fzf"
fi


local M_COMPLETION_DIR="${0:A:h}"
source ${M_COMPLETION_DIR}/base_completion.sh

FZF_VI_OPTS="
  --reverse
  --preview 'bat --color=always {}'
 "

fzf-history-widget() {
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
