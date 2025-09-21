# DslsOS VSCode WSL CMake Fix Script

Write-Host "==================================================" -ForegroundColor Green
Write-Host "Fixing VSCode WSL CMake Generator Conflict" -ForegroundColor Green
Write-Host "==================================================" -ForegroundColor Green

$targetPath = "/home/dslsdzc/.vs/DslsOS"

Write-Host "Target WSL path: $targetPath" -ForegroundColor Yellow
Write-Host ""

try {
    # Check if we can access WSL
    $wslOutput = wsl echo "WSL is accessible" 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "WSL is not accessible"
    }

    Write-Host "WSL is accessible, proceeding with cleanup..." -ForegroundColor Green

    # Run cleanup commands in WSL
    $commands = @"
cd "$targetPath" 2>/dev/null || { echo "Creating target directory..."; mkdir -p "$targetPath"; cd "$targetPath"; }
echo "Current directory: \$(pwd)"
echo "Cleaning build cache..."
rm -rf out/build/linux-debug 2>/dev/null && echo "✓ Removed linux-debug build directory" || echo "Directory not found"
rm -f CMakeCache.txt 2>/dev/null && echo "✓ Removed CMakeCache.txt" || echo "File not found"
rm -rf CMakeFiles 2>/dev/null && echo "✓ Removed CMakeFiles directory" || echo "Directory not found"
echo ""
echo "=================================================="
echo "WSL Cache Cleaning Complete!"
echo "=================================================="
echo ""
echo "You can now rebuild in VSCode!"
"@

    # Execute commands in WSL
    wsl bash -c "$commands"

    Write-Host ""
    Write-Host "✓ Cache cleaning completed!" -ForegroundColor Green
    Write-Host "You can now rebuild in VSCode without the generator conflict." -ForegroundColor Green

} catch {
    Write-Host "Error: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Manual cleanup required:" -ForegroundColor Yellow
    Write-Host "1. Open WSL terminal" -ForegroundColor Yellow
    Write-Host "2. Run: cd /home/dslsdzc/.vs/DslsOS" -ForegroundColor Yellow
    Write-Host "3. Run: rm -rf out/build/linux-debug CMakeCache.txt CMakeFiles" -ForegroundColor Yellow
    Write-Host "4. Then rebuild in VSCode" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Press any key to exit..." -ForegroundColor Cyan
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")