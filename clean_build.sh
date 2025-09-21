#!/bin/bash

echo "=================================================="
echo "DslsOS Build Cache Cleaner"
echo "=================================================="

# Clean common build directories
echo "Cleaning build caches..."

# Remove out directory if it exists
if [ -d "out" ]; then
    echo "Removing out directory..."
    rm -rf out
fi

# Remove build directory if it exists
if [ -d "build" ]; then
    echo "Removing build directory..."
    rm -rf build
fi

# Remove CMakeCache.txt if it exists
if [ -f "CMakeCache.txt" ]; then
    echo "Removing CMakeCache.txt..."
    rm -f CMakeCache.txt
fi

# Remove CMakeFiles directory if it exists
if [ -d "CMakeFiles" ]; then
    echo "Removing CMakeFiles directory..."
    rm -rf CMakeFiles
fi

# Remove any .vs directory
if [ -d ".vs" ]; then
    echo "Removing .vs directory..."
    rm -rf .vs
fi

echo ""
echo "=================================================="
echo "Build cache cleaned successfully!"
echo "=================================================="
echo ""
echo "Now you can try building again with:"
echo "1. cmake . -G 'Unix Makefiles'"
echo "2. cmake --build ."
echo ""
echo "Or use: ./build.sh"
echo ""