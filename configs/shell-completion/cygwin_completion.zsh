local M_COMPLETION_DIR="${0:A:h}"
if [[ "$(uname)" =~ "CYGWIN_*" ]]; then
        echo "is cygwin"
        source ${M_COMPLETION_DIR}/base_completion.sh
        # 确保 fzf 已安装
        FZF_VI_OPTS="
          --reverse
        "
        if ! command -v fzf &>/dev/null; then
            echo "警告: 请先安装 fzf: apt-cyg install fzf"
        fi

        export FZF_CTRL_R_OPTS="
            --reverse
            --bind '?:toggle-preview'
        "

        # 自定义 Ctrl+R 历史搜索小部件（修复多字节问题）
        fzf-history-widget() {
            # 保存当前命令行内容
            local original_buffer=$BUFFER
            local original_cursor=$CURSOR
            
            # 使用 zsh 内置命令处理历史记录，避免 awk 问题
            local selected=$(fc -l 1 | \
                while read -r time cmd; do
                    # 安全打印命令（处理多字节字符）
                    printf "%s\n" "$cmd"
                done | \
                fzf --reverse --tac --no-sort --query="$LBUFFER" ${=FZF_CTRL_R_OPTS})
            
            # 如果用户选择了命令
            if [[ -n "$selected" ]]; then
                # 将选中的命令放入命令行
                BUFFER="$selected"
                CURSOR=$#BUFFER  # 将光标移动到行尾
            else
                # 用户取消选择，恢复原始命令行
                BUFFER=$original_buffer
                CURSOR=$original_cursor
            fi
            
            # 刷新显示
            zle redisplay
        }

        # 注册小部件
        zle -N fzf-history-widget

        # 绑定到 Ctrl+R
        bindkey '^R' fzf-history-widget

fi
