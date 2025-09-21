#!/bin/bash

echo "=================================================="
echo "DslsOS Minimal Compilation Script"
echo "=================================================="

# 检查是否有C编译器
if command -v gcc &> /dev/null; then
    echo "Found GCC compiler"
    echo ""
    echo "Compiling with GCC..."
    gcc -o dslos_minimal minimal_dslos.c
elif command -v clang &> /dev/null; then
    echo "Found Clang compiler"
    echo ""
    echo "Compiling with Clang..."
    clang -o dslos_minimal minimal_dslos.c
else
    echo "ERROR: No C compiler found!"
    echo ""
    echo "Please install gcc or clang:"
    echo "  Ubuntu/Debian: sudo apt-get install build-essential"
    echo "  Fedora: sudo dnf install gcc"
    echo "  macOS: xcode-select --install"
    echo ""
    exit 1
fi

# 检查编译结果
if [ $? -eq 0 ]; then
    echo ""
    echo "=================================================="
    echo "COMPILATION SUCCESSFUL!"
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
    echo "=================================================="
    echo "COMPILATION FAILED!"
    echo "=================================================="
    echo ""
    echo "Please check for compilation errors."
fi