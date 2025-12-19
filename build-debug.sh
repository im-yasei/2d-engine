#!/bin/bash
set -e

echo "Debug build..."

# Clean and create build directory
echo "Cleaning build-debug..."
rm -rf build-debug
mkdir build-debug && cd build-debug

# Configure with Debug flags
echo "Configuring CMake (Debug)..."
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# Return to root and create symlink
cd ..
echo "Creating symlink for LSP..."
ln -sf build-debug/compile_commands.json .

# Build project
echo "Building project..."
cd build-debug && ninja

echo "Debug build completed!"
echo "Run: ./build-debug/client-server/2d-engine"
