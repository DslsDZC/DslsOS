#!/bin/bash

echo "=================================================="
echo "DslsOS WSL Build Fix"
echo "=================================================="

# Check if we're in the correct directory
if [ ! -f "minimal_dslos.c" ]; then
    echo "Error: minimal_dslos.c not found. Please run this script from the DslsOS directory."
    exit 1
fi

echo "Cleaning WSL build cache..."

# Remove the specific path mentioned in the error
WSL_PATH="/home/dslsdzc/.vs/DslsOS"
if [ -d "$WSL_PATH" ]; then
    echo "Found WSL build directory: $WSL_PATH"
    echo "Removing build cache..."
    rm -rf "$WSL_PATH/out/build/linux-debug"
    rm -f "$WSL_PATH/CMakeCache.txt"
    rm -rf "$WSL_PATH/CMakeFiles"
    echo "WSL build cache cleaned."
else
    echo "WSL build directory not found at: $WSL_PATH"
fi

# Also clean current directory
echo "Cleaning current directory..."
rm -rf out build CMakeCache.txt CMakeFiles 2>/dev/null

echo ""
echo "=================================================="
echo "Build cache cleaned!"
echo "=================================================="
echo ""
echo "Now trying to build DslsOS..."
echo ""

# Try to build with gcc directly first
echo "Attempting direct compilation with gcc..."
gcc -o dslos_minimal minimal_dslos.c

if [ $? -eq 0 ]; then
    echo ""
    echo "=================================================="
    echo "BUILD SUCCESSFUL!"
    echo "=================================================="
    echo ""
    echo "Executable created: dslos_minimal"
    echo ""
    echo "To run: ./dslos_minimal"
    echo ""
    echo "Running DslsOS now..."
    echo "=================================================="
    echo ""
    ./dslos_minimal
else
    echo ""
    echo "Direct compilation failed."
    echo "Please install gcc: sudo apt-get install build-essential"
    echo ""
fi