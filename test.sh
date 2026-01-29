#!/usr/bin/env bash
set -euo pipefail

# Configuration
BUILD_DIR="build"
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "🚀 [Build] Starting incremental build..."

mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Silent build unless error
if ! cmake .. -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1; then
    echo -e "${RED}CMake configuration failed${NC}"
    exit 1
fi

if ! make -j$(nproc) > /dev/null 2>&1; then
    echo -e "${RED}Compilation failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Build complete${NC}\n"

# Test Runner
run_test() {
    local name=$1
    local cmd=$2
    
    echo -n "Running ${name}..."
    if $cmd > /dev/null 2>&1; then
        echo -e "\r${GREEN}✓ ${name} passed${NC}            "
    else
        echo -e "\r${RED}✗ ${name} failed${NC}            "
        return 1
    fi
}

FAILED=0

# Execute Suite
run_test "Demo Integration" "./matching_engine_demo" || ((FAILED++))
run_test "Unit Tests"       "./matching_engine_unit_tests" || ((FAILED++))
run_test "Property Tests"   "./matching_engine_property_tests" || ((FAILED++))

if [[ "${1:-}" != "--quick" ]]; then
    run_test "Benchmarks" "./matching_engine_benchmarks" || ((FAILED++))
fi

echo ""
if [[ $FAILED -eq 0 ]]; then
    echo -e "${GREEN}All systems operational.${NC}"
    exit 0
else
    echo -e "${RED}$FAILED test(s) failed.${NC}"
    exit 1
fi