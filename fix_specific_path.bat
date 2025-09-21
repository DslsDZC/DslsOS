@echo off
echo ==================================================
echo Fixing VSCode WSL Generator Conflict
echo ==================================================
echo.

echo Target path: /home/dslsdzc/.vs/DslsOS/out/build/linux-debug
echo.

REM Method 1: Try using PowerShell to access WSL
echo Attempting PowerShell WSL access...
powershell -Command "wsl rm -rf /home/dslsdzc/.vs/DslsOS/out/build/linux-debug 2>''$null'' && echo Success: WSL directory cleaned" 2>nul

if %ERRORLEVEL% EQU 0 (
    echo ✓ WSL directory cleaned via PowerShell
    goto :copy_files
)

REM Method 2: Try direct WSL command
echo Attempting direct WSL command...
wsl rm -rf /home/dslsdzc/.vs/DslsOS/out/build/linux-debug 2>nul

if %ERRORLEVEL% EQU 0 (
    echo ✓ WSL directory cleaned via WSL
    goto :copy_files
)

REM Method 3: Provide manual instructions
echo.
echo ==================================================
echo AUTOMATIC CLEANING FAILED
echo ==================================================
echo.
echo Please run these commands manually in WSL:
echo.
echo 1. Open WSL terminal (Ubuntu/Debian)
echo 2. Run: rm -rf /home/dslsdzc/.vs/DslsOS/out/build/linux-debug
echo 3. Run: cd /home/dslsdzc/.vs/DslsOS
echo 4. Run: rm -f CMakeCache.txt
echo 5. Run: rm -rf CMakeFiles
echo 6. Then rebuild in VSCode
echo.
echo Alternative: Use the minimal version instead
echo compile_minimal.bat
echo.
goto :end

:copy_files
echo.
echo Checking if DslsOS files exist in WSL directory...
echo.

REM Copy necessary files to WSL directory if they don't exist
powershell -Command "if (-not (wsl test -f /home/dslsdzc/.vs/DslsOS/minimal_dslos.c)) { wsl cp /mnt/c/Users/dslsd/source/repos/DslsOS/minimal_dslos.c /home/dslsdzc/.vs/DslsOS/; echo 'Copied minimal_dslos.c to WSL' }" 2>nul

powershell -Command "if (-not (wsl test -f /home/dslsdzc/.vs/DslsOS/CMakeLists.txt)) { wsl cp /mnt/c/Users/dslsd/source/repos/DslsOS/CMakeLists.txt /home/dslsdzc/.vs/DslsOS/; echo 'Copied CMakeLists.txt to WSL' }" 2>nul

echo.
echo ==================================================
echo CLEANING COMPLETED!
echo ==================================================
echo.
echo You can now rebuild in VSCode.
echo The generator conflict should be resolved.
echo.

:end
pause