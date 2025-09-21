@echo off
echo ===================================
echo DslsOS 编译器检查工具
echo ===================================

echo 检查Visual Studio编译器...
where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo ✅ 找到Visual Studio编译器 (cl.exe)
    where cl
    goto :check_gcc
) else (
    echo ❌ 未找到Visual Studio编译器
)

echo.
echo 检查MinGW GCC编译器...
where gcc >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo ✅ 找到MinGW GCC编译器 (gcc.exe)
    where gcc
    goto :check_clang
) else (
    echo ❌ 未找到MinGW GCC编译器
)

echo.
echo 检查Clang编译器...
where clang >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo ✅ 找到Clang编译器 (clang.exe)
    where clang
    goto :check_path
) else (
    echo ❌ 未找到Clang编译器
)

goto :no_compiler

:check_gcc
where gcc >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✅ 也找到GCC编译器 (gcc.exe)
)
goto :check_path

:check_clang
where clang >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✅ 也找到Clang编译器 (clang.exe)
)

:check_path
echo.
echo ===================================
echo 推荐的解决方案：
echo ===================================
echo.
echo 选项1: 使用最小版本（无需CMake）
echo compile_minimal.bat
echo.
echo 选项2: 如果有编译器，请确保在正确的环境中运行：
echo - Visual Studio: "Developer Command Prompt for VS"
echo - MinGW: 确保MinGW/bin在PATH中
echo.
goto :end

:no_compiler
echo.
echo ===================================
echo ❌ 没有找到C编译器！
echo ===================================
echo.
echo 请安装以下任一编译器：
echo.
echo 1. Visual Studio 2022 Community（推荐）
echo    https://visualstudio.microsoft.com/vs/community/
echo    安装时选择"使用C++的桌面开发"
echo.
echo 2. MinGW-w64（轻量级）
echo    https://www.mingw-w64.org/
echo    或者使用MSYS2: https://www.msys2.org/
echo.
echo 3. Clang/LLVM
echo    https://clang.llvm.org/
echo.
echo 安装完成后，重新运行此脚本检查。
echo.
echo 临时解决方案：
echo compile_minimal.bat
echo.

:end
pause