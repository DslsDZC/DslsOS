@echo off
echo ==================================================
echo DslsOS CMake Generator Conflict Fixer
echo ==================================================
echo.

echo Error: Generator conflict between Ninja and Unix Makefiles
echo Solution: Clean build cache and rebuild
echo.

REM Try to clean the specific path mentioned in error
echo Attempting to clean: /home/dslsdzc/.vs/DslsOS/out/build/linux-debug

REM Since we're on Windows, try PowerShell to access WSL paths
powershell -Command "
try {
    if (Test-Path '/home/dslsdzc/.vs/DslsOS') {
        Write-Host 'Found WSL build directory, cleaning...'
        Remove-Item -Path '/home/dslsdzc/.vs/DslsOS/out/build/linux-debug' -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host 'WSL build directory cleaned'
    }
} catch {
    Write-Host 'Could not access WSL path, will clean local directories instead'
}
"

echo.
echo Cleaning local build directories...
call clean_build.bat

echo.
echo ==================================================
echo Alternative Solutions:
echo ==================================================
echo.
echo 1. If using WSL/Linux environment:
echo    cd /home/dslsdzc/.vs/DslsOS
echo    rm -rf out/build/linux-debug
echo    cmake . -G 'Unix Makefiles'
echo.
echo 2. If using Windows:
echo    clean_build.bat
echo    build.bat
echo.
echo 3. Use minimal version (no CMake needed):
echo    compile_minimal.bat
echo.
echo 4. Try our automated fix:
echo    build_easiest.bat
echo.
echo ==================================================
echo Choose the method that matches your environment
echo ==================================================
echo.
pause