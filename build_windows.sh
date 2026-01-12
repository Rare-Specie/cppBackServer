#!/bin/bash

# C++ 项目 Windows 版本交叉编译脚本
# 使用 mingw64 交叉编译器编译学生成绩管理系统后端的 Windows 版本

echo "开始交叉编译 C++ 项目为 Windows 版本..."

# 检查是否安装了 mingw64 交叉编译器
if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "错误: 未找到 mingw64 交叉编译器"
    echo "请安装: brew install mingw-w64"
    exit 1
fi

# 检查依赖库是否存在
echo "检查依赖库..."

# 检查 Crow 库
CROW_INCLUDE=""
if [ -d "/opt/homebrew/include/crow" ]; then
    CROW_INCLUDE="/opt/homebrew/include"
elif [ -d "/opt/homebrew/Cellar/crow/1.3.0/include/crow" ]; then
    CROW_INCLUDE="/opt/homebrew/Cellar/crow/1.3.0/include"
else
    echo "警告: Crow 库未找到，请确保已安装"
fi

# 检查 nlohmann/json 库
JSON_INCLUDE=""
if [ -d "/opt/homebrew/include/nlohmann" ]; then
    JSON_INCLUDE="/opt/homebrew/include"
elif [ -d "/opt/homebrew/Cellar/nlohmann-json/3.12.0/include/nlohmann" ]; then
    JSON_INCLUDE="/opt/homebrew/Cellar/nlohmann-json/3.12.0/include"
else
    echo "警告: nlohmann/json 库未找到，请确保已安装"
fi

# 创建 Windows 构建目录
BUILD_DIR="./build_windows"
mkdir -p "$BUILD_DIR"

# 创建临时的 Windows 兼容版本 auth.h
echo "创建 Windows 兼容版本的 auth.h..."

# 备份原始文件
if [ ! -f "include/auth.h.original" ]; then
    cp "include/auth.h" "include/auth.h.original"
fi

# 创建修改版的 auth.h，使用 Windows Crypto API 替代 OpenSSL
cat > "include/auth.h" << 'EOF'
#ifndef AUTH_H
#define AUTH_H

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include "models.h"
#include "data_manager.h"

#ifdef _WIN32
    #include <windows.h>
    #include <wincrypt.h>
#else
    #include <openssl/sha.h>
    #include <openssl/hmac.h>
#endif

using json = nlohmann::json;

class AuthManager {
private:
    DataManager* dataManager;
    const std::string JWT_SECRET = "your-secret-key-change-in-production";

    // Windows Crypto API 实现的 SHA256
    #ifdef _WIN32
    std::string sha256Windows(const std::string& str) {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        std::string result;
        
        if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
                if (CryptHashData(hHash, reinterpret_cast<const BYTE*>(str.c_str()), str.size(), 0)) {
                    BYTE hash[32];
                    DWORD hashLen = 32;
                    if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
                        std::stringstream ss;
                        for (int i = 0; i < hashLen; i++) {
                            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
                        }
                        result = ss.str();
                    }
                }
                CryptDestroyHash(hHash);
            }
            CryptReleaseContext(hProv, 0);
        }
        return result;
    }
    #endif

public:
    // SHA256哈希（公开方法，供其他类使用）
    std::string sha256(const std::string& str) {
        #ifdef _WIN32
        return sha256Windows(str);
        #else
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(str.c_str()), str.size(), hash);
        
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
        #endif
    }

    // 生成JWT Token
    std::string generateJWT(const std::string& userId) {
        auto now = std::chrono::system_clock::now();
        auto expires = now + std::chrono::hours(24); // 24小时过期
        
        auto now_t = std::chrono::system_clock::to_time_t(now);
        auto expires_t = std::chrono::system_clock::to_time_t(expires);
        
        std::stringstream ss, es;
        ss << std::ctime(&now_t);
        es << std::ctime(&expires_t);
        
        std::string issuedAt = ss.str();
        issuedAt.pop_back();
        std::string expiresAt = es.str();
        expiresAt.pop_back();
        
        // 简单的JWT实现（实际生产应使用标准JWT库）
        std::string token = sha256(userId + issuedAt + JWT_SECRET);
        
        // 保存token
        auto tokens = dataManager->getTokens();
        JWTToken jwtToken{
            token,
            issuedAt,
            expiresAt,
            userId
        };
        tokens.push_back(jwtToken);
        dataManager->saveTokens(tokens);
        
        return token;
    }

    // 验证Token是否有效
    bool isTokenValid(const std::string& token) {
        auto tokens = dataManager->getTokens();
        auto it = std::find_if(tokens.begin(), tokens.end(),
            [&](const JWTToken& t) { return t.token == token; });
        
        if (it == tokens.end()) return false;
        
        // 检查过期时间
        std::tm tm = {};
        std::istringstream ss(it->expiresAt);
        ss >> std::get_time(&tm, "%a %b %d %H:%M:%S %Y");
        
        auto expires = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        auto now = std::chrono::system_clock::now();
        
        return now < expires;
    }

    // 从Token获取用户ID
    std::optional<std::string> getUserIdFromToken(const std::string& token) {
        auto tokens = dataManager->getTokens();
        auto it = std::find_if(tokens.begin(), tokens.end(),
            [&](const JWTToken& t) { return t.token == token; });
        
        if (it == tokens.end()) return std::nullopt;
        
        // 检查过期
        std::tm tm = {};
        std::istringstream ss(it->expiresAt);
        ss >> std::get_time(&tm, "%a %b %d %H:%M:%S %Y");
        
        auto expires = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        auto now = std::chrono::system_clock::now();
        
        if (now >= expires) return std::nullopt;
        
        return it->userId;
    }

    AuthManager(DataManager* dm) : dataManager(dm) {}

    // 用户登录
    std::optional<std::pair<std::string, User>> login(const std::string& username, const std::string& password, const std::string& role) {
        auto users = dataManager->getUsers();
        
        // 查找用户
        auto it = std::find_if(users.begin(), users.end(),
            [&](const User& u) { 
                return u.username == username && u.role == role; 
            });
        
        if (it == users.end()) return std::nullopt;
        
        // 验证密码哈希
        std::string passwordHash = sha256(password);
        if (it->passwordHash != passwordHash) {
            return std::nullopt;
        }
        
        // 生成Token
        std::string token = generateJWT(it->id);
        
        // 记录操作日志
        auto logs = dataManager->getOperationLogs();
        OperationLog log{
            dataManager->generateId(),
            it->id,
            it->username,
            "登录",
            "认证",
            "", // IP在实际使用中需要从请求中获取
            dataManager->getCurrentTimestamp()
        };
        logs.push_back(log);
        dataManager->saveOperationLogs(logs);
        
        return std::make_pair(token, *it);
    }

    // 用户登出
    bool logout(const std::string& token) {
        auto tokens = dataManager->getTokens();
        auto it = std::remove_if(tokens.begin(), tokens.end(),
            [&](const JWTToken& t) { return t.token == token; });
        
        if (it == tokens.end()) return false;
        
        tokens.erase(it, tokens.end());
        dataManager->saveTokens(tokens);
        
        return true;
    }

    // 验证Token
    bool verifyToken(const std::string& token) {
        return isTokenValid(token);
    }

    // 获取当前用户信息
    std::optional<User> getCurrentUser(const std::string& token) {
        auto userId = getUserIdFromToken(token);
        if (!userId.has_value()) return std::nullopt;
        
        auto users = dataManager->getUsers();
        auto it = std::find_if(users.begin(), users.end(),
            [&](const User& u) { return u.id == userId.value(); });
        
        if (it == users.end()) return std::nullopt;
        
        return *it;
    }

    // 修改密码
    // 返回值: 0=success, 1=invalid token, 2=old password incorrect, 3=user not found
    int changePassword(const std::string& token, const std::string& oldPassword, const std::string& newPassword) {
        auto user = getCurrentUser(token);
        if (!user.has_value()) return 1; // invalid token

        // 简单trim函数，避免前端/网络传输带来的前后空白影响匹配
        auto trim = [](const std::string& s) {
            auto start = s.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return std::string("");
            auto end = s.find_last_not_of(" \t\n\r");
            return s.substr(start, end - start + 1);
        };

        std::string oldPwd = trim(oldPassword);
        std::string newPwd = trim(newPassword);

        // 验证旧密码
        std::string oldHash = sha256(oldPwd);
        if (user.value().passwordHash != oldHash) {
            return 2; // old password incorrect
        }
        
        // 更新密码哈希
        auto users = dataManager->getUsers();
        auto it = std::find_if(users.begin(), users.end(),
            [&](const User& u) { return u.id == user.value().id; });
        
        if (it == users.end()) return 3;
        
        it->passwordHash = sha256(newPwd);
        it->updatedAt = dataManager->getCurrentTimestamp();
        dataManager->saveUsers(users);
        
        // 记录操作日志
        auto logs = dataManager->getOperationLogs();
        OperationLog log{
            dataManager->generateId(),
            user.value().id,
            user.value().username,
            "修改密码",
            "用户管理",
            "",
            dataManager->getCurrentTimestamp()
        };
        logs.push_back(log);
        dataManager->saveOperationLogs(logs);
        
        return 0;
    }

    // 检查权限
    bool hasPermission(const std::string& token, const std::vector<std::string>& requiredRoles) {
        auto user = getCurrentUser(token);
        if (!user.has_value()) return false;
        
        // 管理员有所有权限
        if (user.value().role == "admin") return true;
        
        // 检查是否需要特定角色
        for (const auto& role : requiredRoles) {
            if (user.value().role == role) return true;
        }
        
        return false;
    }

    // 获取用户角色
    std::optional<std::string> getUserRole(const std::string& token) {
        auto user = getCurrentUser(token);
        if (!user.has_value()) return std::nullopt;
        return user.value().role;
    }
};

#endif // AUTH_H
EOF

echo "正在编译 main.cpp 为 Windows 可执行文件..."

# 编译命令 - Windows 版本，使用 Windows Crypto API
# 注意：Crow 和 nlohmann/json 都是纯头文件库，不需要链接
# 不需要链接 OpenSSL，使用 Windows 原生 Crypto API
x86_64-w64-mingw32-g++ -std=c++17 \
    -I"$CROW_INCLUDE" \
    -I"$JSON_INCLUDE" \
    -D_WIN32 \
    main.cpp \
    -o "$BUILD_DIR/main.exe" \
    -lws2_32 \
    -ladvapi32 \
    -lmswsock \
    -static-libgcc \
    -static-libstdc++ \
    -static

# 检查编译结果
if [ $? -eq 0 ]; then
    echo "✅ Windows 版本编译成功！"
    echo "生成的可执行文件: $BUILD_DIR/main.exe"
    echo ""
    echo "文件信息:"
    file "$BUILD_DIR/main.exe"
    echo ""
    echo "要运行 Windows 版本，请将 build_windows 目录复制到 Windows 系统上"
    echo "在 Windows 上运行: main.exe"
    echo "访问 API: http://localhost:21180/api"
    echo ""
    echo "注意: Windows 版本使用静态链接，通常不需要额外的 DLL 文件"
    echo "如果运行时出现问题，请检查:"
    echo "  - 防火墙设置"
    echo "  - data 目录是否存在"
    echo "  - 端口 21180 是否被占用"
else
    echo "❌ 编译失败！"
    echo "请检查错误信息"
    
    # 恢复原始 auth.h
    if [ -f "include/auth.h.original" ]; then
        cp "include/auth.h.original" "include/auth.h"
        echo "已恢复原始 auth.h 文件"
    fi
    exit 1
fi

# 恢复原始 auth.h
if [ -f "include/auth.h.original" ]; then
    cp "include/auth.h.original" "include/auth.h"
    echo "已恢复原始 auth.h 文件"
fi

# 创建运行说明文件
cat > "$BUILD_DIR/README_Windows.txt" << 'EOF'
Windows 版本运行说明
=====================

1. 运行环境要求：
   - Windows 10 或更高版本（64位）
   - 无需额外安装运行时库（已静态链接）

2. 运行方法：
   - 双击 main.exe 运行
   - 或在命令行中运行: main.exe

3. 访问 API：
   - 默认监听地址: http://localhost:21180
   - API 文档请参考项目根目录的 API文档.md

4. 数据目录：
   - 程序会在当前目录下寻找 data 文件夹
   - 请确保 data 目录及其中的 JSON 文件存在
   - data 目录结构：
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

5. 注意事项：
   - 防火墙可能会阻止程序运行，请允许程序通过防火墙
   - 如果需要修改端口，请修改源代码中的端口设置
   - 确保 data 目录有读写权限
   - 程序会在首次运行时自动创建必要的数据文件

6. 常见问题：
   - 如果无法访问 API，检查防火墙设置
   - 如果提示端口被占用，修改端口或关闭占用程序
   - 如果数据读取错误，检查 data 目录权限和文件格式

7. 技术说明：
   - 使用 Windows Crypto API 进行 SHA256 哈希
   - 静态链接所有依赖，便于部署
   - 支持跨平台数据文件格式

EOF

echo "Windows 版本构建说明已保存到: $BUILD_DIR/README_Windows.txt"