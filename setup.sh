#!/bin/bash

# ============================================================================
# Setup Script for Matching Engine Project
# 自动创建项目结构和文件
# ============================================================================

set -e  # Exit on error

echo "╔════════════════════════════════════════════════════════════╗"
echo "║   Matching Engine Project Setup                           ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# 项目根目录
PROJECT_ROOT="."

# 检查是否已存在
#if [ -d "$PROJECT_ROOT" ]; then
#    echo "⚠️  Directory $PROJECT_ROOT already exists!"
#    read -p "Do you want to overwrite it? (y/n) " -n 1 -r
#    echo
#    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
#        echo "Aborted."
#        exit 1
#    fi
#    rm -rf "$PROJECT_ROOT"
#fi

# 创建目录结构
#echo "📁 Creating directory structure..."
#mkdir -p "$PROJECT_ROOT"/{src,tests,benchmarks,docs,build}

# 提示用户
echo "📂 Directory structure(current folder):"
echo "   $PROJECT_ROOT/"
echo "   ├── src/           (Core implementation)"
echo "   ├── tests/         (Unit and property tests)"
echo "   ├── benchmarks/    (Performance tests)"
echo "   ├── docs/          (Documentation)"
echo "   └── build/         (Build artifacts)"
echo ""
echo "📝 Next steps:"
echo ""
echo "1. Copy the header files to src/:"
echo "   - types.hpp"
echo "   - order.hpp"
echo "   - events.hpp"
echo "   - orderbook.hpp"
echo "   - replay.hpp"
echo "   - main.cpp"
echo ""
echo "2. Copy test files to tests/:"
echo "   - unit_tests.cpp"
echo "   - property_tests.cpp"
echo ""
echo "3. Copy benchmark file to benchmarks/:"
echo "   - perf.cpp"
echo ""
echo "4. Copy CMakeLists.txt to project root"
echo ""
echo "5. Copy README.md to project root"
echo ""
echo "6. Build the project:"
echo "   cd $PROJECT_ROOT/build"
echo "   cmake .."
echo "   make -j\$(nproc)"
echo ""
echo "7. Run the executables:"
echo "   ./matching_engine_demo"
echo "   ./matching_engine_unit_tests"
echo "   ./matching_engine_property_tests"
echo "   ./matching_engine_benchmarks"
echo ""

# 创建一个快速构建脚本
cat > "$PROJECT_ROOT/build.sh" << 'EOF'
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
EOF

chmod +x "$PROJECT_ROOT/build.sh"

# 创建一个运行所有测试的脚本
cat > "$PROJECT_ROOT/run_all.sh" << 'EOF'
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
EOF

chmod +x "$PROJECT_ROOT/run_all.sh"

# 创建 .gitignore
cat > "$PROJECT_ROOT/.gitignore" << 'EOF'
# Build artifacts
build/
*.o
*.exe
*.out

# IDE files
.vscode/
.idea/
*.swp
*.swo

# OS files
.DS_Store
Thumbs.db

# Log files
*.log

# CMake files
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
Makefile
EOF

# 创建占位文件说明
cat > "$PROJECT_ROOT/SETUP_INSTRUCTIONS.txt" << 'EOF'
========================
SETUP INSTRUCTIONS
========================

You have created the project structure. Now you need to:

1. Copy all the code files from the artifacts to their respective directories:

   src/types.hpp
   src/order.hpp
   src/events.hpp
   src/orderbook.hpp
   src/replay.hpp
   src/main.cpp
   
   tests/unit_tests.cpp
   tests/property_tests.cpp
   
   benchmarks/perf.cpp
   
   CMakeLists.txt (in root)
   README.md (in root)

2. Build the project:
   cd build
   cmake ..
   make -j$(nproc)

3. Run:
   ./matching_engine_demo

Or use the convenience scripts:
   ./build.sh      # Build everything
   ./run_all.sh    # Run all tests and benchmarks

========================
EOF

echo "📄 Created helper scripts:"
echo "   - build.sh      (Quick build script)"
echo "   - run_all.sh    (Run all tests)"
echo "   - .gitignore    (Git ignore file)"
echo ""
echo "✨ Setup complete! Follow the instructions above to populate the files."
echo ""
echo "Happy coding! 🚀"