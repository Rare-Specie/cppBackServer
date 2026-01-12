#ifndef AUTH_H
#define AUTH_H

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <nlohmann/json.hpp>
#include "models.h"
#include "data_manager.h"

using json = nlohmann::json;

class AuthManager {
private:
    DataManager* dataManager;
    const std::string JWT_SECRET = "your-secret-key-change-in-production";

public:
    // SHA256哈希（公开方法，供其他类使用）
    std::string sha256(const std::string& str) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(str.c_str()), str.size(), hash);
        
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
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