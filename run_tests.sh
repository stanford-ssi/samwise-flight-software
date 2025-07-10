#!/bin/bash

# Samwise Flight Software Test Runner
# This script runs all unit and integration tests

set -e  # Exit on first error

echo "=== Samwise Flight Software Test Suite ==="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

FAILED=0
PASSED=0

run_test() {
    local test_name="$1"
    local test_command="$2"
    
    echo -n "Running $test_name... "
    
    if $test_command > /tmp/test_output.log 2>&1; then
        echo -e "${GREEN}PASSED${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}FAILED${NC}"
        echo "Error output:"
        cat /tmp/test_output.log
        echo ""
        FAILED=$((FAILED + 1))
    fi
}

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}Error: Must run from project root directory${NC}"
    echo "Current directory: $(pwd)"
    echo "Files in current directory:"
    ls -la
    exit 1
fi

echo "Running from directory: $(pwd)"

# Create test build directory
TEST_BUILD_DIR="build-tests"
echo "Creating test build directory: $TEST_BUILD_DIR"
mkdir -p "$TEST_BUILD_DIR"
cd "$TEST_BUILD_DIR"

# Configure for testing
echo "Configuring for test build..."
if ! cmake -DPROFILE=TEST .. > /tmp/cmake_config.log 2>&1; then
    echo -e "${RED}CMake configuration failed${NC}"
    cat /tmp/cmake_config.log
    exit 1
fi

# Build tests
echo "Building tests..."
if ! make > /tmp/make_build.log 2>&1; then
    echo -e "${RED}Build failed${NC}"
    cat /tmp/make_build.log
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"
echo ""

# Run tests
echo "Running tests..."
echo "=================="

# HAL Tests
if [ -x "src/hal/test/hal_test" ]; then
    run_test "HAL Basic Tests" "./src/hal/test/hal_test"
else
    echo -e "${YELLOW}HAL tests not found, skipping...${NC}"
fi

# Packet Authentication Tests
if [ -x "src/packet/test/packet_test" ]; then
    run_test "Packet Authentication Tests" "./src/packet/test/packet_test"
else
    echo -e "${YELLOW}Packet tests not found, skipping...${NC}"
fi

# Onboard LED Driver Tests
if [ -x "src/drivers/onboard_led/test/onboard_led_test" ]; then
    run_test "Onboard LED Driver Tests" "./src/drivers/onboard_led/test/onboard_led_test"
else
    echo -e "${YELLOW}Onboard LED tests not found, skipping...${NC}"
fi

# Flash Driver Tests
if [ -x "src/drivers/flash/test/flash_test" ]; then
    run_test "Flash Driver Tests" "./src/drivers/flash/test/flash_test"
else
    echo -e "${YELLOW}Flash tests not found, skipping...${NC}"
fi

# ADM1176 Driver Tests
if [ -x "src/drivers/adm1176/test/adm1176_test" ]; then
    run_test "ADM1176 Driver Tests" "./src/drivers/adm1176/test/adm1176_test"
else
    echo -e "${YELLOW}ADM1176 tests not found, skipping...${NC}"
fi

# Task tests (currently excluded from TEST builds)
if [ -x "src/tasks/beacon/test/beacon_task_test" ]; then
    run_test "Beacon Task Tests" "./src/tasks/beacon/test/beacon_task_test"
else
    echo -e "${YELLOW}Beacon tests not found, skipping...${NC}"
fi

if [ -x "src/tasks/print/test/print_task_test" ]; then
    run_test "Print Task Tests" "./src/tasks/print/test/print_task_test"
else
    echo -e "${YELLOW}Print tests not found, skipping...${NC}"
fi

# Summary
echo ""
echo "===================="
echo "Test Results Summary"
echo "===================="
echo -e "Tests passed: ${GREEN}$PASSED${NC}"
echo -e "Tests failed: ${RED}$FAILED${NC}"
echo -e "Total tests:  $((PASSED + FAILED))"

if [ $FAILED -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed! 🎉${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed! 😞${NC}"
    exit 1
fi