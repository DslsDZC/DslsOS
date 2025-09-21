@echo off
echo Building DslsOS...

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64

REM Build the project
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    cd ..
    exit /b 1
)

echo Build completed successfully!
echo.
echo Executable location: build\bin\Release\dslsos.exe
echo.
echo To run: build\bin\Release\dslsos.exe

cd ..