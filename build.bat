@echo off
setlocal

set QT_DIR=C:\Qt\5.15.2\mingw81_64
set MINGW_DIR=C:\Qt\Tools\mingw810_64\bin
set CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe

echo ================================
echo  DoubanQt Build Script (Qt5)
echo ================================

if not exist build mkdir build
cd build

echo [1/3] CMake Configure...
"%CMAKE_EXE%" .. -G "MinGW Makefiles" ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_MAKE_PROGRAM="%MINGW_DIR%\mingw32-make.exe" ^
    -DCMAKE_CXX_COMPILER="%MINGW_DIR%\g++.exe" ^
    -DCMAKE_C_COMPILER="%MINGW_DIR%\gcc.exe"

if errorlevel 1 (
    echo [ERROR] CMake configure failed!
    pause
    exit /b 1
)

echo [2/3] Building...
"%MINGW_DIR%\mingw32-make.exe" -j%NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo [3/3] Deploying Qt DLLs...
cd ..
if not exist dist mkdir dist
copy build\DoubanQt.exe dist\

:: 复制 Qt 核心 DLL
for %%f in (Qt5Core Qt5Gui Qt5Network Qt5Sql Qt5Widgets Qt5Xml) do (
    copy "%QT_DIR%\bin\%%f.dll" dist\ >nul 2>&1
)

:: 复制 MinGW 运行时
set MINGW_BIN=%MINGW_DIR%
for %%f in (libgcc_s_seh-1 libstdc++-6 libwinpthread-1) do (
    copy "%MINGW_BIN%\%%f.dll" dist\ >nul 2>&1
)

:: 复制 OpenSSL（HTTPS 必须）
for %%f in (libssl-1_1-x64 libcrypto-1_1-x64) do (
    copy "C:\Qt\Tools\mingw1120_64\opt\bin\%%f.dll" dist\ >nul 2>&1
)

:: 复制平台插件
if not exist dist\platforms mkdir dist\platforms
if not exist dist\sqldrivers mkdir dist\sqldrivers
copy "%QT_DIR%\plugins\platforms\qwindows.dll" dist\platforms\ >nul 2>&1
copy "%QT_DIR%\plugins\sqldrivers\qsqlite.dll" dist\sqldrivers\ >nul 2>&1

echo.
echo ================================
echo  Build SUCCESS! Run: dist\DoubanQt.exe
echo ================================
pause
