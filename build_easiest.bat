@echo off
echo ==================================================
echo DslsOS Easiest Build Script
echo ==================================================

REM Backup original CMakeLists.txt
if exist CMakeLists.txt (
    copy CMakeLists.txt CMakeLists_backup.txt >nul 2>nul
)

REM Use simple CMakeLists.txt
copy CMakeLists_simple.txt CMakeLists.txt >nul 2>nul

REM Create build directory
if not exist build_simple mkdir build_simple
cd build_simple

REM Try different CMake generators
echo.
echo Trying to build DslsOS...
echo.

REM Try 1: MinGW Makefiles
echo Attempt 1: MinGW Makefiles
cmake .. -G "MinGW Makefiles" 2>nul
if %ERRORLEVEL% EQU 0 (
    cmake --build . 2>nul
    if %ERRORLEVEL% EQU 0 (
        echo SUCCESS with MinGW!
        goto :success
    )
)

REM Try 2: Unix Makefiles
echo Attempt 2: Unix Makefiles
cmake .. -G "Unix Makefiles" 2>nul
if %ERRORLEVEL% EQU 0 (
    cmake --build . 2>nul
    if %ERRORLEVEL% EQU 0 (
        echo SUCCESS with Unix Makefiles!
        goto :success
    )
)

REM Try 3: Default generator
echo Attempt 3: Default generator
cmake .. 2>nul
if %ERRORLEVEL% EQU 0 (
    cmake --build . 2>nul
    if %ERRORLEVEL% EQU 0 (
        echo SUCCESS with default generator!
        goto :success
    )
)

REM If all CMake attempts failed, try direct compilation
echo.
echo CMake attempts failed. Trying direct compilation...
echo.

REM Try direct gcc compilation
gcc -o dslos_direct.exe ../minimal_dslos.c 2>nul
if %ERRORLEVEL% EQU 0 (
    echo SUCCESS with direct gcc compilation!
    move dslos_direct.exe ..\ >nul 2>nul
    goto :direct_success
)

REM Try direct cl compilation
cl /Fe:dslos_direct.exe ../minimal_dslos.c 2>nul
if %ERRORLEVEL% EQU 0 (
    echo SUCCESS with direct cl compilation!
    move dslos_direct.exe ..\ >nul 2>nul
    goto :direct_success
)

:fail
echo.
echo ==================================================
echo ALL BUILD ATTEMPTS FAILED!
echo ==================================================
echo.
echo Please install a C compiler:
echo 1. Visual Studio 2022 Community (recommended)
echo 2. MinGW-w64
echo 3. Or use: compile_minimal.bat (requires compiler)
echo.
cd ..
if exist CMakeLists_backup.txt (
    move CMakeLists_backup.txt CMakeLists.txt >nul 2>nul
)
pause
exit /b 1

:success
echo.
echo ==================================================
echo BUILD SUCCESSFUL!
echo ==================================================
echo.
echo Executable: dslos.exe
echo.
cd ..
move build_simple\dslos.exe . >nul 2>nul
if exist CMakeLists_backup.txt (
    move CMakeLists_backup.txt CMakeLists.txt >nul 2>nul
)
echo To run: dslos.exe
echo.
pause
exit /b 0

:direct_success
echo.
echo ==================================================
echo DIRECT COMPILATION SUCCESSFUL!
echo ==================================================
echo.
echo Executable: dslos_direct.exe
echo.
cd ..
if exist CMakeLists_backup.txt (
    move CMakeLists_backup.txt CMakeLists.txt >nul 2>nul
)
echo To run: dslos_direct.exe
echo.
pause
exit /b 0