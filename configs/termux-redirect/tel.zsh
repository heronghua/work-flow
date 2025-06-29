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
