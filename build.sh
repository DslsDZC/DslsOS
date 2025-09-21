#!/bin/bash

echo "Building DslsOS..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Build failed!"
    cd ..
    exit 1
fi

echo "Build completed successfully!"
echo ""
echo "Executable location: build/bin/dslsos"
echo ""
echo "To run: ./build/bin/dslsos"

cd ..