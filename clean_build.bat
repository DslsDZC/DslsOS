@echo off
echo ==================================================
echo DslsOS Build Cache Cleaner
echo ==================================================

REM Remove out directory if it exists
if exist "out" (
    echo Removing out directory...
    rmdir /s /q out
)

REM Remove build directory if it exists
if exist "build" (
    echo Removing build directory...
    rmdir /s /q build
)

REM Remove CMakeCache.txt if it exists
if exist "CMakeCache.txt" (
    echo Removing CMakeCache.txt...
    del /f /q CMakeCache.txt
)

REM Remove CMakeFiles directory if it exists
if exist "CMakeFiles" (
    echo Removing CMakeFiles directory...
    rmdir /s /q CMakeFiles
)

REM Remove any .vs directory
if exist ".vs" (
    echo Removing .vs directory...
    rmdir /s /q .vs
)

echo.
echo ==================================================
echo Build cache cleaned successfully!
echo ==================================================
echo.
echo Now you can try building again with:
echo 1. build.bat
echo 2. compile_minimal.bat
echo 3. build_easiest.bat
echo.
pause