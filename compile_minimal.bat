@echo off
echo ==================================================
echo DslsOS Minimal Compilation Script
echo ==================================================

REM 检查是否有C编译器
where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Found Visual Studio compiler
    echo.
    echo Compiling with Microsoft C Compiler...
    cl.exe /W4 /O2 /nologo /Fe:dslos_minimal.exe minimal_dslos.c
    goto :check_result
)

where gcc >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Found GCC compiler
    echo.
    echo Compiling with GCC...
    gcc -o dslos_minimal.exe minimal_dslos.c
    goto :check_result
)

where clang >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Found Clang compiler
    echo.
    echo Compiling with Clang...
    clang -o dslos_minimal.exe minimal_dslos.c
    goto :check_result
)

echo ERROR: No C compiler found!
echo.
echo Please install one of the following:
echo - Visual Studio (cl.exe)
echo - MinGW (gcc.exe)
echo - LLVM (clang.exe)
echo.
pause
exit /b 1

:check_result
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ==================================================
    echo COMPILATION SUCCESSFUL!
    echo ==================================================
    echo.
    echo Executable created: dslos_minimal.exe
    echo.
    echo To run: dslos_minimal.exe
    echo.
    echo Running DslsOS now...
    echo ==================================================
    echo.
    dslos_minimal.exe
) else (
    echo.
    echo ==================================================
    echo COMPILATION FAILED!
    echo ==================================================
    echo.
    echo Please check for compilation errors.
)
pause