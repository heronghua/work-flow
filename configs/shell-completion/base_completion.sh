# 定义快捷键：vi + Tab 打开并执行 fzf
vi-fzf-completion() {
    # 只处理以 "vi " 开头的命令
    if [[ $LBUFFER == vi\ * ]]; then
        # 保存当前命令行内容
        local original_buffer=$LBUFFER

        # 使用 fd 或 find 查找文件
        local selected
        if command -v fd &>/dev/null; then
            selected=$(fd --type f --hidden --exclude .git 2>/dev/null | fzf --reverse)
        else
            selected=$(find . -type f 2>/dev/null | fzf --reverse)
        fi

        # 清除当前命令行
        LBUFFER=""
        zle redisplay

        if [[ -n $selected ]]; then
            # 直接执行 vi 命令打开文件
            BUFFER="vi ${(q)selected}"
            zle accept-line  # 模拟回车执行命令
        else
            # 如果用户取消选择，恢复原始命令行
            LBUFFER=$original_buffer
            zle redisplay
        fi
    else
        # 其他情况执行正常补全
        zle expand-or-complete
    fi
}

# 注册小部件
zle -N vi-fzf-completion

# 绑定到 Tab 键
bindkey '^I' vi-fzf-completion
