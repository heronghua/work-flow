# 添加以下内容
tel:() {
    # 提取号码部分（去掉"tel:"前缀）
    local number="${1#tel:}"
    
    # 检查是否安装了 termux-api
    if ! command -v termux-telephony-call &> /dev/null; then
        echo "错误：请先安装 termux-api 包"
        echo "运行：pkg install termux-api"
        return 1
    fi
    
    # 拨打号码
    termux-telephony-call "$number"
}

# 1. 启用扩展模式匹配
shopt -s extglob

# 2. 定义命令未找到时的处理函数
command_not_found_handle() {
    local cmd="$1"

    # 匹配 tel: 开头的命令
    if [[ "$cmd" == tel:+([0-9#*]) ]]; then
        local number="${cmd#tel:}"

        # 检查是否安装了 termux-api
        if ! command -v termux-telephony-call &> /dev/null; then
            echo "错误：请先安装 termux-api 包"
            echo "运行：pkg install termux-api"
            return 127
        fi

        # 拨打号码
        termux-telephony-call "$number"
        return $?  # 返回命令的退出状态

    # 可以添加其他模式匹配规则...
    else
        # 默认处理
        echo "bash: $cmd: command not found"
        return 127
    fi
}
