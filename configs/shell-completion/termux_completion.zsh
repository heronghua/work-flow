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

fzf-history-widget() {
        local original_buffer=$BUFFER
        local original_cursor=$CURSOR

        # Termux 使用更简单的历史获取方式
        local selected=$(history -n 1 | \
            fzf --height 40% --tac --query="$LBUFFER" ${=FZF_CTRL_R_OPTS})

        if [[ -n "$selected" ]]; then
            BUFFER="$selected"
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
