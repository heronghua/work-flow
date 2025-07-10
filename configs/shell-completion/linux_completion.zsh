if ! command -v fzf &>/dev/null; then
    echo "警告: 请先安装 fzf: apt install fzf"
fi

# 设置公共选项
export FZF_CTRL_R_OPTS="
    --height 40%
    --reverse
    --prompt='历史命令搜索 > '
    --bind 'ctrl-d:page-down,ctrl-u:page-up'
    --preview 'echo -e \"\033[1m命令:\033[0m {}\"'
    --preview-window 'down:3:wrap'
    --color 'fg:252,bg:233,hl:220,fg+:252,bg+:235,hl+:220'
    --color 'info:144,prompt:161,spinner:108,pointer:168,marker:168'
    --header $'←→/C-d/C-u: 浏览 | ^R: 刷新 | ENTER: 执行 | ESC: 取消\n'
"

fzf-history-widget() {
        local original_buffer=$BUFFER
        local original_cursor=$CURSOR
        
        # Linux 使用更健壮的历史处理方式
        local selected
        if [[ -o extendedhistory ]]; then
            # 带时间戳的历史格式
            selected=$(fc -il 1 | \
                awk '{$1=""; print substr($0,2)}' | \
                fzf --height 40% --reverse --tac --query="$LBUFFER" ${=FZF_CTRL_R_OPTS})
        else
            # 普通历史格式
            selected=$(fc -rl 1 | \
                awk '{$1=""; print substr($0,2)}' | \
                fzf --height 40% --reverse --tac --query="$LBUFFER" ${=FZF_CTRL_R_OPTS})
        fi
        
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
