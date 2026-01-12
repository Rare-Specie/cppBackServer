# Windows 版本编译指南

## 概述

本项目支持在 macOS 上使用 mingw64 交叉编译器编译 Windows 版本。生成的可执行文件可以在 Windows 系统上运行。

**重要说明**：由于 OpenSSL 在 Windows 上的兼容性问题，Windows 版本使用 Windows 原生的 Crypto API 进行 SHA256 哈希，而不是 OpenSSL。这确保了最佳的 Windows 兼容性。

## 前置要求

### 1. 安装 mingw64 交叉编译器

在 macOS 上使用 Homebrew 安装：

```bash
brew install mingw-w64
```

### 2. 确保依赖库已安装

```bash
# Crow 库 (头文件库)
brew install crow

# nlohmann/json 库 (头文件库)
brew install nlohmann-json
```

## 编译步骤

### 使用自动编译脚本（推荐）

```bash
# 进入项目目录
cd /Users/rarespecies/Documents/folder/cppBackServer

# 运行 Windows 编译脚本
./build_windows.sh
```

编译成功后，所有文件将生成在 `build_windows/` 目录中。

### 手动编译（了解原理）

```bash
# 创建构建目录
mkdir -p build_windows

# 临时修改 auth.h 以支持 Windows Crypto API
# （编译脚本会自动处理此步骤）

# 编译 Windows 版本
x86_64-w64-mingw32-g++ -std=c++17 \
    -I/opt/homebrew/include \
    -I/opt/homebrew/Cellar/crow/1.3.0/include \
    -D_WIN32 \
    main.cpp \
    -o build_windows/main.exe \
    -lws2_32 \
    -ladvapi32 \
    -lmswsock \
    -static-libgcc \
    -static-libstdc++ \
    -static
```
```

## 运行 Windows 版本

### 1. 文件传输

将以下文件复制到 Windows 系统：
- `build_windows/main.exe` (主程序)
- `data/` 目录 (所有数据文件)
- `API文档.md` (API 说明)

### 2. 在 Windows 上运行

```cmd
# 方法一：双击运行
main.exe

# 方法二：命令行运行
.\main.exe
```

### 3. 访问 API

默认监听地址：`http://localhost:21180`

示例：
- 登录：`POST http://localhost:21180/api/auth/login`
- 获取用户列表：`GET http://localhost:21180/api/users`

## 注意事项

### 1. 数据目录

程序需要 `data` 目录及其中的 JSON 文件：
```
data/
├── users.json
├── students.json
├── courses.json
├── grades.json
├── operation_logs.json
├── system_logs.json
├── backups.json
├── settings.json
└── tokens.json
```

### 2. 防火墙设置

Windows 防火墙可能会阻止程序运行：
- 首次运行时，允许程序通过防火墙
- 或者手动添加入站规则允许 21180 端口

### 3. 依赖 DLL 文件

如果使用静态链接（脚本默认），通常不需要额外的 DLL。如果出现 DLL 缺失错误：

- `libstdc++-6.dll` / `libgcc_s_seh-1.dll`：安装 Visual C++ Redistributable
- OpenSSL 相关 DLL：可能需要手动复制 OpenSSL DLL 到程序目录

检查依赖：
```cmd
# 在 Windows 上使用 PowerShell
dumpbin /dependents main.exe
```

### 4. 路径分隔符

代码使用 `/` 作为路径分隔符，Windows 通常能正确处理，但建议：
- 在 Windows 上运行时，确保程序在包含 `data` 目录的目录中运行
- 或者修改 `main.cpp` 中的初始化路径

### 5. 端口冲突

如果 21180 端口被占用：
- 修改 `main.cpp` 中的 `port` 变量
- 重新编译

## 常见问题

### Q: 编译时出现 "OpenSSL not found" 错误？
A: 确保 mingw-w64 的 OpenSSL 开发文件已安装，或手动指定 OpenSSL 路径。

### Q: Windows 上运行时出现 "data directory not found"？
A: 确保 `data` 目录与 `main.exe` 在同一目录下。

### Q: 程序启动但无法访问 API？
A: 检查防火墙设置，确保 21180 端口未被阻止。

### Q: 出现 SSL 相关错误？
A: Windows 版本可能需要额外的 OpenSSL DLL 文件，或者使用静态链接版本。

## 调试技巧

### 1. 查看详细错误信息

在 Windows 命令行运行：
```cmd
.\main.exe 2>&1
```

### 2. 检查端口占用

```cmd
netstat -ano | findstr :21180
```

### 3. 查看进程

```cmd
tasklist | findstr main
```

## 发布准备

如果要发布 Windows 版本，建议：

1. **完整测试**：在干净的 Windows 环境中测试
2. **依赖打包**：如果需要 DLL，一并打包
3. **文档准备**：包含运行说明和 API 文档
4. **数据初始化**：确保 `data` 目录包含必要的初始数据

## 技术细节

### 交叉编译说明

- **编译器**：x86_64-w64-mingw32-g++
- **目标平台**：Windows 64位
- **链接库**：
  - `libssl` / `libcrypto`：OpenSSL 加密库
  - `libws2_32`：Windows Socket 库
  - `-static-libgcc` / `-static-libstdc++`：静态链接 C++ 运行时

### 架构兼容性

- **x86_64**：64位 Windows 系统
- **32位**：如需 32位版本，使用 `i686-w64-mingw32-g++`

### 性能考虑

- 静态链接会增加文件大小，但减少依赖
- Windows 版本性能与 macOS 版本基本一致
- 网络性能取决于 Windows 系统配置

## 快速开始

### 1. 编译
```bash
./build_windows.sh
```

### 2. 部署到 Windows
将 `build_windows/` 目录复制到 Windows 系统

### 3. 准备数据
在 Windows 上，确保 `build_windows/` 目录下有 `data/` 文件夹

### 4. 运行
双击 `main.exe` 或在命令行运行

### 5. 测试 API
访问 `http://localhost:21180/api` 查看 API 是否正常工作

## 文件说明

编译后 `build_windows/` 目录包含：

- `main.exe` - 主程序（约 25MB）
- `README_Windows.txt` - Windows 运行说明
- `data/` - 需要手动创建的数据目录

## 技术特点

✅ **完全静态链接** - 无需额外 DLL 文件  
✅ **Windows 原生 API** - 使用 Windows Crypto API 替代 OpenSSL  
✅ **跨平台数据兼容** - JSON 数据文件格式兼容  
✅ **高性能** - 与 macOS 版本性能相当  
✅ **易于部署** - 单个可执行文件 + 数据目录

## 支持的 Windows 版本

- Windows 10 (64位)
- Windows 11 (64位)
- Windows Server 2016+
- Windows Server 2019+
- Windows Server 2022+

## 获取帮助

如果编译或运行遇到问题：

1. 检查 `build_windows.sh` 的输出信息
2. 查看 `README_Windows.txt` 中的常见问题
3. 确保 Windows 防火墙允许程序运行
4. 检查端口 21180 是否被其他程序占用