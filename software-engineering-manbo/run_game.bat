@echo off
chcp 65001 > nul
title 大富翁游戏
cls

echo 正在编译游戏...
gcc -g Richc.c cJSON.c -o richc.exe -lm

if %errorlevel% neq 0 (
    echo 编译失败! 请检查错误信息
    pause
    exit /b 1
)

echo 编译成功!

REM 检查是否存在 preset.json 文件
if exist "test/case6.1/preset.json" (
    echo.
    echo [提示] 在 test/case1/ 目录下检测到 preset.json 文件
    echo [提示] 游戏将加载预设配置并直接开始游戏
) else (
    echo.
    echo [提示] 在 test/case1/ 目录下未找到 preset.json 文件
    echo [提示] 游戏将正常启动，进入角色选择界面
)

echo.
echo 正在启动游戏...
echo.
richc.exe "test/case6.1/"

echo.
echo 游戏已退出
pause