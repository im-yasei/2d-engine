#!/bin/bash
set -e

echo "Release build..."

# Clean and create build directory
echo "Cleaning build-release..."
rm -rf build-release
mkdir build-release && cd build-release

# Configure with Release flags and SFML path
echo "Configuring CMake (Release)..."
cmake -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DSFML_DIR=/opt/homebrew/opt/sfml@2/lib/cmake/SFML \
    ..

# Return to root and create symlink
cd ..
echo "Creating symlink for LSP..."
ln -sf build-release/compile_commands.json .

# Build project
echo "Building project..."
cd build-release && ninja

echo "Release build completed!"
echo "Run: ./build-release/client/2d-engine"