@echo off
echo ==================================================
echo DslsOS Simple CMake Build
echo ==================================================

REM Create build directory
if not exist build mkdir build
cd build

REM Configure CMake
echo Configuring CMake...
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo Trying with default generator...
    cmake ..
)

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake failed! Please install a C compiler.
    echo.
    echo Recommended solutions:
    echo 1. Install Visual Studio 2022 Community
    echo 2. Install MinGW-w64
    echo 3. Use the minimal version: compile_minimal.bat
    echo.
    cd ..
    pause
    exit /b 1
)

REM Build
echo.
echo Building DslsOS...
cmake --build .

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ==================================================
    echo BUILD SUCCESSFUL!
    echo ==================================================
    echo.
    echo Executable: build\bin\dslsos.exe
    echo.
    echo To run: build\bin\dslsos.exe
    echo.
) else (
    echo.
    echo ==================================================
    echo BUILD FAILED!
    echo ==================================================
    echo.
    cd ..
    pause
    exit /b 1
)

cd ..
pause