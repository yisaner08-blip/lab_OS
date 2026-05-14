@echo off
chcp 65001 >nul
echo ============================================
echo   操作系统内核原型 - QEMU 运行脚本
echo ============================================
echo.
echo   启动 QEMU 运行 kernel.img ...
echo   串口输出将显示在下方：
echo   (如果没有输出，请关闭 QEMU 窗口重试)
echo ============================================
echo.

"D:\QUMU\qemu\qemu-system-i386.exe" -serial stdio kernel.img

echo.
echo ============================================
echo   QEMU 已退出。
echo ============================================
pause
