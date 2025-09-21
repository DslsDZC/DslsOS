@echo off
echo ===================================
echo DslsOS Simple Build Script
echo ===================================

REM 检查Visual Studio是否安装
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Visual Studio compiler not found!
    echo Please install Visual Studio 2022 with C++ development tools
    echo.
    echo You can download it from: https://visualstudio.microsoft.com/
    echo.
    echo After installation, run this script from "Developer Command Prompt for VS"
    pause
    exit /b 1
)

echo Visual Studio compiler found: OK

REM 创建构建目录
if not exist build_simple mkdir build_simple
cd build_simple

echo.
echo ===================================
echo Creating minimal build without CMake...
echo ===================================

REM 创建简单的编译脚本
echo @echo off > compile.bat
echo echo Compiling DslsOS... >> compile.bat
echo cl.exe /W4 /O2 /nologo /Fe:dslos_simple.exe ..\*.c >> compile.bat
echo if %%ERRORLEVEL%% EQU 0 ( >> compile.bat
echo     echo. >> compile.bat
echo     echo =================================== >> compile.bat
echo     echo BUILD SUCCESSFUL! >> compile.bat
echo     echo Executable: dslos_simple.exe >> compile.bat
echo     echo =================================== >> compile.bat
echo     echo. >> compile.bat
echo     echo Running DslsOS... >> compile.bat
echo     echo. >> compile.bat
echo     dslos_simple.exe >> compile.bat
echo ) else ( >> compile.bat
echo     echo. >> compile.bat
echo     echo =================================== >> compile.bat
echo     echo BUILD FAILED! >> compile.bat
echo     =================================== >> compile.bat
echo ) >> compile.bat
echo pause >> compile.bat

echo Created compile.bat in build_simple directory
echo.
echo To compile and run DslsOS:
echo   1. cd build_simple
echo   2. compile.bat
echo.

cd ..
echo Simple build script created successfully!
echo.
echo ===================================
echo QUICK START:
echo ===================================
echo cd build_simple
echo compile.bat
echo ===================================

pause