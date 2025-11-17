#!/bin/bash

# 快速构建脚本

echo "Building Matching Engine..."

cd build || exit
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

echo ""
echo "✅ Build complete!"
echo ""
echo "Available executables:"
echo "  - ./matching_engine_demo"
echo "  - ./matching_engine_unit_tests"
echo "  - ./matching_engine_property_tests"
echo "  - ./matching_engine_benchmarks"
