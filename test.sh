#!/usr/bin/env bash
# Run all tests and benchmarks
# Usage: ./run_all.sh [--quick]

set -euo pipefail

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

# Parse arguments
QUICK=false
if [[ "${1:-}" == "--quick" ]]; then
    QUICK=true
fi

# Check if build directory exists
if [[ ! -d "build" ]]; then
    echo -e "${RED}Error: build/ directory not found${NC}"
    echo "Run ./build.sh first"
    exit 1
fi

cd build

echo "╔════════════════════════════════════════╗"
echo "║   Matching Engine - Test Suite        ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Function to run with error handling
run_test() {
    local name=$1
    local cmd=$2
    
    echo -e "${BLUE}▶ Running ${name}...${NC}"
    if ${cmd} > /dev/null 2>&1; then
        echo -e "${GREEN}✓ ${name} passed${NC}"
        return 0
    else
        echo -e "${RED}✗ ${name} failed${NC}"
        return 1
    fi
}

# Run tests
FAILED=0

run_test "Demo" "./matching_engine_demo" || ((FAILED++))
run_test "Unit Tests" "./matching_engine_unit_tests" || ((FAILED++))
run_test "Property Tests" "./matching_engine_property_tests" || ((FAILED++))

if [[ "$QUICK" == false ]]; then
    run_test "Benchmarks" "./matching_engine_benchmarks" || ((FAILED++))
fi

echo ""
if [[ $FAILED -eq 0 ]]; then
    echo -e "${GREEN}✅ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}❌ ${FAILED} test(s) failed${NC}"
    exit 1
fi