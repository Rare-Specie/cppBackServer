#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

#include <string>
#include <vector>
#include <crow.h>
#include "auth.h"
#include "data_manager.h"

// 认证中间件
class AuthMiddleware {
private:
    AuthManager* authManager;

public:
    AuthMiddleware(AuthManager* am) : authManager(am) {}

    // 检查请求头中的Token
    std::optional<std::string> getTokenFromRequest(const crow::request& req) {
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.empty()) return std::nullopt;
        
        // Bearer Token格式
        if (authHeader.substr(0, 7) != "Bearer ") return std::nullopt;
        
        return authHeader.substr(7);
    }

    // 验证Token中间件
    bool verifyToken(const crow::request& req, crow::response& res) {
        auto token = getTokenFromRequest(req);
        if (!token.has_value()) {
            res.code = 401;
            res.body = R"({"error": "Unauthorized", "message": "Missing or invalid Authorization header"})";
            return false;
        }
        
        if (!authManager->verifyToken(token.value())) {
            res.code = 401;
            res.body = R"({"error": "Unauthorized", "message": "Invalid or expired token"})";
            return false;
        }
        
        return true;
    }

    // 检查权限中间件
    bool checkPermission(const crow::request& req, crow::response& res, const std::vector<std::string>& requiredRoles) {
        auto token = getTokenFromRequest(req);
        if (!token.has_value()) {
            res.code = 401;
            res.body = R"({"error": "Unauthorized", "message": "Missing token"})";
            return false;
        }
        
        if (!authManager->hasPermission(token.value(), requiredRoles)) {
            res.code = 403;
            res.body = R"({"error": "Forbidden", "message": "Insufficient permissions"})";
            return false;
        }
        
        return true;
    }

    // 获取当前用户
    std::optional<User> getCurrentUser(const crow::request& req) {
        auto token = getTokenFromRequest(req);
        if (!token.has_value()) return std::nullopt;
        return authManager->getCurrentUser(token.value());
    }
};

// 日志中间件
class LogMiddleware {
private:
    DataManager* dataManager;

public:
    LogMiddleware(DataManager* dm) : dataManager(dm) {}

    // 记录系统日志
    void logSystem(const std::string& level, const std::string& message, const std::string& module, const std::string& ip = "") {
        auto logs = dataManager->getSystemLogs();
        SystemLog log{
            dataManager->generateId(),
            level,
            message,
            module,
            ip.empty() ? std::nullopt : std::optional<std::string>(ip),
            dataManager->getCurrentTimestamp()
        };
        logs.push_back(log);
        dataManager->saveSystemLogs(logs);
    }

    // 记录操作日志
    void logOperation(const std::string& userId, const std::string& username, const std::string& action, const std::string& module, const std::string& ip = "") {
        auto logs = dataManager->getOperationLogs();
        OperationLog log{
            dataManager->generateId(),
            userId,
            username,
            action,
            module,
            ip.empty() ? std::nullopt : std::optional<std::string>(ip),
            dataManager->getCurrentTimestamp()
        };
        logs.push_back(log);
        dataManager->saveOperationLogs(logs);
    }

    // 记录请求日志
    void logRequest(const crow::request& req, const crow::response& res, const std::optional<User>& user = std::nullopt) {
        std::string action = "REQUEST";
        std::string module = "API";
        std::string ip = req.get_header_value("X-Forwarded-For");
        if (ip.empty()) ip = req.get_header_value("Remote-Addr");
        
        std::string level = (res.code >= 400) ? "warning" : "info";
        std::string message = "Request processed | Response: " + std::to_string(res.code);
        
        if (user.has_value()) {
            logOperation(user.value().id, user.value().username, action, module, ip);
        } else {
            logSystem(level, message, module, ip);
        }
    }
};

// 辅助函数：JSON响应
inline crow::response jsonResponse(const json& data, int code = 200) {
    crow::response res(code);
    res.set_header("Content-Type", "application/json");
    res.body = data.dump();
    return res;
}

inline crow::response jsonResponse(const std::string& message, int code = 200) {
    json j = {{"message", message}};
    crow::response res(code);
    res.set_header("Content-Type", "application/json");
    res.body = j.dump();
    return res;
}

inline crow::response errorResponse(const std::string& error, const std::string& message, int code = 400) {
    json j = {
        {"error", error},
        {"message", message}
    };
    return jsonResponse(j, code);
}

// 分页辅助函数
template<typename T>
json paginate(const std::vector<T>& data, int page, int limit) {
    int total = data.size();
    int start = (page - 1) * limit;
    int end = std::min(start + limit, total);
    
    if (start >= total) {
        return json{
            {"data", json::array()},
            {"total", total},
            {"page", page},
            {"limit", limit}
        };
    }
    
    std::vector<T> pageData(data.begin() + start, data.begin() + end);
    json jData = json::array();
    for (const auto& item : pageData) {
        jData.push_back(item);
    }
    
    return json{
        {"data", jData},
        {"total", total},
        {"page", page},
        {"limit", limit}
    };
}

// 搜索和筛选辅助函数
inline bool matchesSearch(const std::string& text, const std::string& search) {
    if (search.empty()) return true;
    return text.find(search) != std::string::npos;
}

// 数据验证辅助函数
inline bool validateEmail(const std::string& email) {
    // 简单的邮箱格式验证
    return email.find('@') != std::string::npos && email.find('.') != std::string::npos;
}

inline bool validatePhone(const std::string& phone) {
    // 简单的手机号验证（1开头的11位数字）
    return phone.length() == 11 && phone[0] == '1' && 
           std::all_of(phone.begin(), phone.end(), ::isdigit);
}

inline bool validateScore(int score) {
    return score >= 0 && score <= 100;
}

inline bool validatePassword(const std::string& password) {
    return password.length() >= 6;
}

#endif // MIDDLEWARE_H