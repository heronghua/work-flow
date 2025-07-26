@echo off
setlocal enabledelayedexpansion

:: set environment
set HOST_NAME=com.example.open_path_host
set HOST_PATH=%cd%\open_path_host.py
set MANIFEST_PATH=%APPDATA%\Mozilla\NativeMessagingHosts\%HOST_NAME%.json
set CHROME_MANIFEST_PATH=%LOCALAPPDATA%\Google\Chrome\User Data\NativeMessagingHosts\%HOST_NAME%.json

:: TODO test create Firefox manifest
echo {
echo   "name": "%HOST_NAME%",
echo   "description": "Open Path Host",
echo   "path": "python",
echo   "type": "stdio",
echo   "args": ["%HOST_PATH%"],
echo   "allowed_extensions": ["open-path-with-explorer@example.com"]
echo } > "%MANIFEST_PATH%"

:: create Chrome manifest
:: create bat
echo @echo off > %cd%\open_path_host.bat
echo python %cd%\open_path_host.py >> %cd%/open_path_host.bat

echo { > "%CHROME_MANIFEST_PATH%"
echo   "name": "%HOST_NAME%", >> "%CHROME_MANIFEST_PATH%"
echo   "description": "Open Path Host", >> "%CHROME_MANIFEST_PATH%" 

set "currentDir=%cd%"
set "currentDir=!currentDir:\=\\!"
echo   "path": "!currentDir!\\open_path_host.bat", >> "%CHROME_MANIFEST_PATH%"

echo   "type": "stdio", >> "%CHROME_MANIFEST_PATH%"
echo   "allowed_origins": [ >> "%CHROME_MANIFEST_PATH%"
echo       "chrome-extension://你的扩展ID/" >> "%CHROME_MANIFEST_PATH%"
echo   ] >> "%CHROME_MANIFEST_PATH%"
echo } >> "%CHROME_MANIFEST_PATH%"

echo install complete , ensure you have python environment.
endlocal
