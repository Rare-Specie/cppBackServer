#!/bin/bash

# C++ 项目编译脚本
# 使用 clang++ 编译学生成绩管理系统后端

echo "开始编译 C++ 项目..."

# 检查是否安装了 clang++
if ! command -v clang++ &> /dev/null; then
    echo "错误: 未找到 clang++ 编译器"
    echo "请安装 Xcode Command Line Tools: xcode-select --install"
    exit 1
fi

# 检查依赖库
if [ ! -d "/opt/homebrew/include/crow" ] && [ ! -d "/opt/homebrew/Cellar/crow/1.3.0/include/crow" ]; then
    echo "警告: Crow 库未找到"
    echo "请安装: brew install crow"
fi

if [ ! -d "/opt/homebrew/include/nlohmann" ] && [ ! -d "/opt/homebrew/Cellar/nlohmann-json/3.12.0/include/nlohmann" ]; then
    echo "警告: nlohmann/json 库未找到"
    echo "请安装: brew install nlohmann-json"
fi

if [ ! -d "/opt/homebrew/include/openssl" ] && [ ! -d "/opt/homebrew/Cellar/openssl@3/3.6.0/include/openssl" ]; then
    echo "警告: OpenSSL 库未找到"
    echo "请安装: brew install openssl"
fi

# 编译命令
echo "正在编译 main.cpp..."
clang++ -std=c++17 \
    -I/opt/homebrew/include \
    -I/opt/homebrew/Cellar/crow/1.3.0/include \
    main.cpp \
    -o main \
    -L/opt/homebrew/lib \
    -lssl \
    -lcrypto

# 检查编译结果
if [ $? -eq 0 ]; then
    echo "✅ 编译成功！"
    echo "生成的可执行文件: ./main"
    echo ""
    echo "运行程序: ./main"
    echo "访问 API: http://localhost:21180/api"
else
    echo "❌ 编译失败！"
    exit 1
fi