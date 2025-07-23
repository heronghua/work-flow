#!/bin/bash

HOST_NAME=com.example.open_path_host
HOST_PATH=$(pwd)/open_path_host.py

# Firefox manifest目录
FIREFOX_MANIFEST_DIR="$HOME/.mozilla/native-messaging-hosts"
# Chrome/Chromium manifest目录
CHROME_MANIFEST_DIR="$HOME/.config/google-chrome/NativeMessagingHosts"
CHROMIUM_MANIFEST_DIR="$HOME/.config/chromium/NativeMessagingHosts"

# 创建目录
mkdir -p "$FIREFOX_MANIFEST_DIR"
mkdir -p "$CHROME_MANIFEST_DIR"
mkdir -p "$CHROMIUM_MANIFEST_DIR"

# 创建Firefox的manifest文件
cat > "$FIREFOX_MANIFEST_DIR/$HOST_NAME.json" << EOF
{
  "name": "$HOST_NAME",
  "description": "Open Path Host",
  "path": "$HOST_PATH",
  "type": "stdio",
  "allowed_extensions": ["open-path-with-explorer@example.com"]
}
EOF

# 创建Chrome的manifest文件
cat > "$CHROME_MANIFEST_DIR/$HOST_NAME.json" << EOF
{
  "name": "$HOST_NAME",
  "description": "Open Path Host",
  "path": "$HOST_PATH",
  "type": "stdio",
  "allowed_origins": [
    "chrome-extension://你的扩展ID/"
  ]
}
EOF

# 创建Chromium的manifest文件
cp "$CHROME_MANIFEST_DIR/$HOST_NAME.json" "$CHROMIUM_MANIFEST_DIR/"

# 设置可执行权限
chmod +x $HOST_PATH

echo "安装完成。"