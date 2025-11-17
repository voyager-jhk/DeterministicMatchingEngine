#!/bin/bash

# 运行所有测试和演示

echo "╔════════════════════════════════════════════════════════════╗"
echo "║   Running All Tests and Benchmarks                        ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

cd build || exit

echo "▶️  Running Demo..."
./matching_engine_demo
echo ""

echo "▶️  Running Unit Tests..."
./matching_engine_unit_tests
echo ""

echo "▶️  Running Property Tests..."
./matching_engine_property_tests
echo ""

echo "▶️  Running Benchmarks..."
./matching_engine_benchmarks
echo ""

echo "✅ All tests completed!"
