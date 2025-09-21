#!/bin/bash

echo "=================================================="
echo "Fixing VSCode WSL CMake Generator Conflict"
echo "=================================================="

# Target path from your error message
TARGET_PATH="/home/dslsdzc/.vs/DslsOS"

echo "Target path: $TARGET_PATH"
echo ""

# Check if the target path exists
if [ ! -d "$TARGET_PATH" ]; then
    echo "Target directory not found: $TARGET_PATH"
    echo "Creating directory structure..."
    mkdir -p "$TARGET_PATH"
fi

# Navigate to target directory
cd "$TARGET_PATH" || {
    echo "Cannot navigate to: $TARGET_PATH"
    exit 1
}

echo "Current directory: $(pwd)"
echo ""

# Clean the specific build directory mentioned in error
echo "Cleaning linux-debug build directory..."
if [ -d "out/build/linux-debug" ]; then
    rm -rf out/build/linux-debug
    echo "✓ Removed out/build/linux-debug"
fi

# Remove any CMake cache files in the target directory
echo "Cleaning CMake cache files..."
if [ -f "CMakeCache.txt" ]; then
    rm -f CMakeCache.txt
    echo "✓ Removed CMakeCache.txt"
fi

if [ -d "CMakeFiles" ]; then
    rm -rf CMakeFiles
    echo "✓ Removed CMakeFiles directory"
fi

# Clean any other build directories
if [ -d "out/build" ]; then
    find out/build -type d -name "*debug" -o -name "*release" | head -5 | while read dir; do
        echo "✓ Removed $dir"
        rm -rf "$dir"
    done
fi

echo ""
echo "=================================================="
echo "Cache cleaning completed!"
echo "=================================================="
echo ""
echo "Now you can:"
echo "1. Rebuild in VSCode (it should work now)"
echo "2. Or run manual build:"
echo "   cd $TARGET_PATH"
echo "   cmake . -G 'Unix Makefiles'"
echo "   cmake --build ."
echo ""
echo "DslsOS source files should be in this directory."
echo "If not, you may need to copy them from your Windows directory."
echo ""