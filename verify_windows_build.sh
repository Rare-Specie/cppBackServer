#!/bin/bash

# 验证 Windows 版本编译结果

echo "🔍 验证 Windows 版本编译结果..."

BUILD_DIR="./build_windows"
EXE_FILE="$BUILD_DIR/main.exe"

# 检查文件是否存在
if [ ! -f "$EXE_FILE" ]; then
    echo "❌ 错误: 未找到 $EXE_FILE"
    echo "请先运行 ./build_windows.sh 进行编译"
    exit 1
fi

echo "✅ 主程序文件存在"

# 检查文件大小
SIZE=$(stat -f%z "$EXE_FILE")
SIZE_MB=$(echo "scale=2; $SIZE / 1024 / 1024" | bc)
echo "📊 文件大小: $SIZE_MB MB"

# 检查文件类型
FILE_TYPE=$(file "$EXE_FILE")
echo "📋 文件类型: $FILE_TYPE"

# 检查是否为 PE32+ 可执行文件
if [[ "$FILE_TYPE" == *"PE32+ executable"* ]]; then
    echo "✅ 文件格式正确 (Windows 64位)"
else
    echo "⚠️  文件格式可能不正确"
fi

# 检查 README 文件
if [ -f "$BUILD_DIR/README_Windows.txt" ]; then
    echo "✅ 说明文档存在"
else
    echo "⚠️  缺少说明文档"
fi

# 检查是否需要 data 目录
echo ""
echo "📋 部署清单:"
echo "   ✓ main.exe (主程序)"
echo "   ✓ README_Windows.txt (说明文档)"
echo "   ⚠  data/ 目录 (需要手动创建)"
echo ""
echo "📁 data 目录应包含以下文件:"
echo "   - users.json"
echo "   - students.json"
echo "   - courses.json"
echo "   - grades.json"
echo "   - operation_logs.json"
echo "   - system_logs.json"
echo "   - backups.json"
echo "   - settings.json"
echo "   - tokens.json"

echo ""
echo "🚀 部署步骤:"
echo "1. 将 build_windows/ 目录复制到 Windows 系统"
echo "2. 在 Windows 上创建 data/ 目录并放入 JSON 文件"
echo "3. 双击 main.exe 运行程序"
echo "4. 在浏览器访问 http://localhost:21180/api"

echo ""
echo "🔍 验证完成！"