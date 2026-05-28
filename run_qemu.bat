@echo off
echo ============================================
echo   lab_OS - QEMU Launcher
echo ============================================
echo.
echo   Starting QEMU with kernel.img ...
echo   Serial output will appear below:
echo ============================================
echo.

"D:\QUMU\qemu\qemu-system-i386.exe" -serial stdio kernel.img

echo.
echo ============================================
echo   QEMU exited.
echo ============================================
pause
