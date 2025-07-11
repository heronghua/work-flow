# 自定义 vi 命令的补全函数
_vi_complete() {
    local cur
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    
    # 当按两次 Tab 时触发 fzf
    if [[ ${COMP_TYPE} -eq 63 ]]; then  # 63 是两次 Tab 的特殊标识
        # 使用 fzf 选择文件
        local selected
        selected=$(find . -type f 2>/dev/null | fzf --height 40% --reverse --preview 'head -100 {}')
        
        if [[ -n "$selected" ]]; then
            # 直接打开选中的文件
            vi "$selected"
            # 清空命令行
            printf '\e[2K\r'
        fi
    else
        # 常规文件补全
        COMPREPLY=( $(compgen -f -- "${cur}") )
    fi
}

# 将自定义补全函数绑定到 vi 命令
complete -F _vi_complete vi
