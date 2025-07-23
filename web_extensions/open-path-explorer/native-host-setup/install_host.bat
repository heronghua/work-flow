@echo off
setlocal

:: 设置变量
set HOST_NAME=com.example.open_path_host
set HOST_PATH=%cd%\open_path_host.py
set MANIFEST_PATH=%APPDATA%\Mozilla\NativeMessagingHosts\%HOST_NAME%.json
set CHROME_MANIFEST_PATH=%LOCALAPPDATA%\Google\Chrome\User Data\NativeMessagingHosts\%HOST_NAME%.json

:: 创建Firefox的manifest文件
echo {
echo   "name": "%HOST_NAME%",
echo   "description": "Open Path Host",
echo   "path": "python",
echo   "type": "stdio",
echo   "args": ["%HOST_PATH%"],
echo   "allowed_extensions": ["open-path-with-explorer@example.com"]
echo } > "%MANIFEST_PATH%"

:: 创建Chrome的manifest文件
echo {
echo   "name": "%HOST_NAME%",
echo   "description": "Open Path Host",
echo   "path": "python",
echo   "type": "stdio",
echo   "allowed_origins": [
echo       "chrome-extension://你的扩展ID/"
echo   ]
echo } > "%CHROME_MANIFEST_PATH%"

echo 安装完成，请确保已安装Python。
endlocal