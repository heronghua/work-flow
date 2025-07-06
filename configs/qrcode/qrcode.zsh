qr() {
    # 检查是否安装了 qrencode
    if ! command -v qrencode &> /dev/null; then
        echo "错误：需要安装 qrencode"
        echo "安装:"
        echo "  Linux/Debian: sudo apt install qrencode"
        echo "  Termux: pkg install qrencode"
        echo "  Cygwin: 安装 qrencode 包"
        return 1
    fi
    
    # 获取输入
    local input
    if [ $# -eq 0 ]; then
        input=$(cat)
    else
        input="$*"
    fi
    
    # 检查输入是否为空
    if [ -z "$input" ]; then
        echo "错误：未提供二维码内容"
        return 1
    fi
    
    # 直接在终端显示二维码
    # qrencode -t ANSIUTF8 "$input"
    qrencode -m 2 -t ANSI "$input"
}

# 添加在线服务选项
qrcode-online() {
    # 获取输入
    local input
    if [ $# -eq 0 ]; then
        input=$(cat)
    else
        input="$*"
    fi
    
    # 检查输入是否为空
    if [ -z "$input" ]; then
        echo "错误：未提供二维码内容"
        return 1
    fi
    
    # 使用在线服务
    curl -sS -d "$input" https://qrcode.show
    echo  # 添加空行
}


wifi() {
        qr "f"
}
