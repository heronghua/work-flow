#!/usr/bin/env python3
import sys
import json
import platform
import subprocess
import os

def open_path_in_explorer(path):
    system = platform.system()
    if system == 'Windows':
        # 对于Windows，使用explorer打开
        # 注意：如果路径是文件，则打开所在文件夹并选中文件；如果是文件夹，则打开文件夹
        if os.path.isfile(path):
            subprocess.Popen(f'explorer /select,"{path}"')
        else:
            subprocess.Popen(f'explorer "{path}"')
    elif system == 'Darwin':  # macOS
        subprocess.Popen(['open', path])
    else:  # Linux
        # 尝试使用xdg-open
        subprocess.Popen(['xdg-open', path])

def main():
    # 从stdin读取消息
    while True:
        # 读取前4个字节（消息长度）
        raw_length = sys.stdin.buffer.read(4)
        if not raw_length:
            break
        message_length = int.from_bytes(raw_length, byteorder='little')
        # 读取消息内容
        message = sys.stdin.buffer.read(message_length).decode('utf-8')
        data = json.loads(message)
        path = data.get('path')
        if path:
            open_path_in_explorer(path)
            # 发送一个简单的响应
            response = {"status": "success"}
        else:
            response = {"status": "error", "message": "No path provided"}

        # 将响应发送回扩展
        response_json = json.dumps(response)
        response_bytes = response_json.encode('utf-8')
        sys.stdout.buffer.write(len(response_bytes).to_bytes(4, byteorder='little'))
        sys.stdout.buffer.write(response_bytes)
        sys.stdout.buffer.flush()

if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        sys.stderr.write(str(e))
        sys.exit(1)