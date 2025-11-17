# ============================================================================
# CMakeLists.txt - Matching Engine Build Configuration
# ============================================================================

cmake_minimum_required(VERSION 3.14)
project(MatchingEngine VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Compiler Settings
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter)
endif()

# ============================================================================
# Include Directories
# ============================================================================

include_directories(${CMAKE_SOURCE_DIR}/src)

# ============================================================================
# Main Executable
# ============================================================================

add_executable(matching_engine_demo
    src/main.cpp
)

# ============================================================================
# Unit Tests
# ============================================================================

add_executable(matching_engine_unit_tests
    tests/unit_tests.cpp
)

# ============================================================================
# Property Tests
# ============================================================================

add_executable(matching_engine_property_tests
    tests/property_tests.cpp
)

# ============================================================================
# Benchmarks
# ============================================================================

add_executable(matching_engine_benchmarks
    benchmarks/perf.cpp
)

# ============================================================================
# Build Types
# ============================================================================

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# ============================================================================
# Installation
# ============================================================================

install(TARGETS 
    matching_engine_demo
    matching_engine_unit_tests
    matching_engine_property_tests
    matching_engine_benchmarks
    DESTINATION bin
)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "  Matching Engine Build Configuration")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "")
message(STATUS "  Targets:")
message(STATUS "    - matching_engine_demo           (Main demo)")
message(STATUS "    - matching_engine_unit_tests     (Unit tests)")
message(STATUS "    - matching_engine_property_tests (Property tests)")
message(STATUS "    - matching_engine_benchmarks     (Performance)")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "")