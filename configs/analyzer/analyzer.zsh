#!/bin/zsh

# 定义 ANR 函数
ANR() {
  # 创建临时 Python 脚本
  local py_script=$(mktemp)
  
  cat > "$py_script" <<'PYTHON_END'
#!/usr/bin/env python3
import os
import re
import sys
import pyperclip  # 导入剪贴板模块

def highlight_match(line, pattern):
    """高亮显示匹配部分"""
    highlighted = []
    last_end = 0
    
    for match in re.finditer(pattern, line):
        # 添加匹配前的部分
        start, end = match.span()
        highlighted.append(line[last_end:start])
        
        # 添加高亮匹配部分
        highlighted.append("\033[1;31m")  # 红色开始
        highlighted.append(line[start:end])
        highlighted.append("\033[0m")     # 重置样式
        
        last_end = end
    
    # 添加剩余部分
    highlighted.append(line[last_end:])
    return ''.join(highlighted)

def main():
    # 固定正则表达式模式
    pattern = "Anr in|Anr.*Total|Anr in.*media.module"
    
    # 从剪贴板获取目录路径
    directory = pyperclip.paste().strip()
    
    # 如果没有从剪贴板获取到有效路径，则提示用户输入
    if not directory or not os.path.isdir(directory):
        print(f"\033[1;33m剪贴板内容不是有效目录: '{directory}'\033[0m")
        print("\033[1;32m请输入要搜索的文件夹路径:\033[0m")
        directory = input("> ").strip()
    
    # 验证路径
    if not os.path.isdir(directory):
        print(f"\033[1;31m错误：路径不是目录 - '{directory}'\033[0m")
        sys.exit(1)
    
    # 编译正则表达式
    try:
        regex = re.compile(pattern)
    except re.error as e:
        print(f"\033[1;31m无效的正则表达式: {e}\033[0m")
        sys.exit(1)
    
    # 用于存储结果 {文件路径: [(行号, 高亮内容)]}
    results = {}
    
    # 递归搜索文件
    for root, _, files in os.walk(directory):
        for file in files:
            file_path = os.path.join(root, file)
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    file_matches = []
                    for line_num, line in enumerate(f, 1):
                        if regex.search(line):
                            # 高亮匹配内容
                            highlighted_line = highlight_match(line.rstrip(), regex.pattern)
                            file_matches.append((line_num, highlighted_line))
                    
                    # 如果文件有匹配项，添加到结果
                    if file_matches:
                        results[file_path] = file_matches
            except Exception as e:
                # 跳过无法读取的文件
                continue
    
    # 输出结果
    if results:
        print("\n\033[1;32m搜索结果:\033[0m")
        for file_path, matches in results.items():
            # 打印文件路径
            print(f"\n\033[1;34m{file_path}\033[0m")
            
            # 打印该文件的所有匹配行
            for line_num, content in matches:
                print(f"  \033[1;33m{line_num}\033[0m: {content}")
    else:
        print("\033[1;33m未找到匹配项\033[0m")

if __name__ == "__main__":
    main()
PYTHON_END

  # 设置颜色
  local GREEN=$'\e[1;32m'
  local BLUE=$'\e[1;34m'
  local YELLOW=$'\e[1;33m'
  local RED=$'\e[1;31m'
  local RESET=$'\e[0m'
  
  # 检查是否已安装pyperclip
  if ! python3 -c "import pyperclip" &>/dev/null; then
    echo "${YELLOW}正在安装pyperclip模块...${RESET}"
    if ! pip3 install pyperclip; then
      echo "${RED}错误：无法安装pyperclip模块${RESET}"
      rm -f "$py_script"
      return 1
    fi
  fi
  
  # 执行 Python 脚本
  echo "${GREEN}使用正则表达式: \"Anr in|Anr.*Total|Anr in.*media.module\"${RESET}"
  echo "${BLUE}正在搜索...${RESET}"
  python3 "$py_script"
  
  # 清理临时文件
  rm -f "$py_script"
}