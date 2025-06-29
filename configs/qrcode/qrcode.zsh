qr() {
    # 检查是否安装了 qrencode
    if ! command -v qrencode &> /dev/null; then
        echo "错误：需要安装 qrencode"
        echo "Linux/Debian: sudo apt install qrencode"
        echo "Termux: pkg install qrencode"
        echo "Cygwin: 通过安装程序选择 qrencode 包"
        return 1
    fi
    
    # 检查参数
    if [ $# -eq 0 ]; then
        echo "用法: qr <文本内容>"
        echo "示例: qr 'https://example.com'"
        return 1
    fi
    
    # 合并所有参数
    local text="$*"
    
    # 系统检测
    local os_type
    case "$(uname -s)" in
        Linux*)  os_type="linux" ;;
        CYGWIN*) os_type="cygwin" ;;
        *)       os_type="other" ;;
    esac
    
    # 检查是否在 Termux 中
    if [ -n "$TERMUX_VERSION" ]; then
        os_type="termux"
    fi
    
    # 根据不同系统处理
    case "$os_type" in
        termux|linux)
            # 检查是否安装了 libcaca
            if ! command -v cacaview &> /dev/null && [ "$os_type" = "termux" ]; then
                echo "提示: 在 Termux 中安装 libcaca 可显示图片: pkg install libcaca"
            fi
            
            if command -v cacaview &> /dev/null; then
                # 使用 libcaca 显示图片
                local tmpfile="$(mktemp -u).png"
                qrencode -s 10 -m 2 -o "$tmpfile" "$text"
                cacaview "$tmpfile"
                rm -f "$tmpfile"
            else
                # 回退到文本模式
                qrencode -t UTF8 "$text"
            fi
            ;;
        
        cygwin)
            # Cygwin 处理
            if command -v img2txt &> /dev/null; then
                # 使用 libcaca 的 img2txt
                local tmpfile="$(mktemp -u).png"
                qrencode -s 10 -m 2 -o "$tmpfile" "$text"
                img2txt "$tmpfile"
                rm -f "$tmpfile"
            else
                # 回退到文本模式
                qrencode -t UTF8 "$text"
            fi
            ;;
        
        *)
            # 其他系统使用文本模式
            qrencode -t UTF8 "$text"
            ;;
    esac
}

# 添加文本模式快捷方式
qrd() {
    [ $# -eq 0 ] && { echo "用法: qrd <文本>"; return 1; }
    qrencode -t UTF8 "$*"
}

# 添加保存功能
qrsave() {
    [ $# -lt 2 ] && { 
        echo "用法: qrsave <文件名> <文本>"
        return 1
    }
    qrencode -s 15 -m 3 -o "$1" "${@:2}"
}
