#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include <string>
#include <vector>
#include <optional>
#include <crow.h>
#include "models.h"
#include "data_manager.h"
#include "auth.h"
#include "middleware.h"

class UserService {
private:
    DataManager* dataManager;
    AuthManager* authManager;
    LogMiddleware* logger;

public:
    UserService(DataManager* dm, AuthManager* am, LogMiddleware* log) 
        : dataManager(dm), authManager(am), logger(log) {}

    // 获取用户列表（管理员权限）
    crow::response getUsers(const crow::request& req) {
        // 验证权限
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        // 获取查询参数
        std::string role = req.get_header_value("X-Query-Role");
        std::string search = req.get_header_value("X-Query-Search");
        int page = 1;
        int limit = 10;
        
        // 解析URL参数（Crow框架中需要手动处理）
        // 这里简化处理，实际应该解析URL参数

        auto users = dataManager->getUsers();
        
        // 筛选
        std::vector<User> filtered;
        for (const auto& user : users) {
            if (!role.empty() && user.role != role) continue;
            if (!search.empty() && 
                user.username.find(search) == std::string::npos &&
                user.name.find(search) == std::string::npos) continue;
            filtered.push_back(user);
        }

        // 分页
        auto result = paginate(filtered, page, limit);
        
        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username, 
                               "GET /users", "用户管理");
        }

        return jsonResponse(result);
    }

    // 创建用户
    crow::response createUser(const crow::request& req) {
        // 验证权限
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        // 解析请求体
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        // 验证必填字段
        if (!body.contains("username") || !body.contains("password") || 
            !body.contains("role") || !body.contains("name")) {
            return errorResponse("BadRequest", "Missing required fields", 400);
        }

        std::string username = body["username"];
        std::string password = body["password"];
        std::string role = body["role"];
        std::string name = body["name"];
        std::optional<std::string> className;
        if (body.contains("class") && !body["class"].is_null()) {
            className = body["class"];
        }

        // 验证角色
        if (role != "admin" && role != "teacher" && role != "student") {
            return errorResponse("BadRequest", "Invalid role", 400);
        }

        // 检查用户名是否已存在
        auto users = dataManager->getUsers();
        auto it = std::find_if(users.begin(), users.end(),
            [&](const User& u) { return u.username == username; });
        if (it != users.end()) {
            return errorResponse("Conflict", "Username already exists", 409);
        }

        // 创建用户（存储密码哈希）
        User newUser{
            dataManager->generateId(),
            username,
            authManager->sha256(password), // 存储密码哈希
            role,
            name,
            className,
            dataManager->getCurrentTimestamp(),
            dataManager->getCurrentTimestamp()
        };
        users.push_back(newUser);
        dataManager->saveUsers(users);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /users", "用户管理");
        }

        return jsonResponse(newUser, 201);
    }

    // 更新用户
    crow::response updateUser(const crow::request& req, const std::string& id) {
        // 验证权限
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        // 解析请求体
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        auto users = dataManager->getUsers();
        auto it = std::find_if(users.begin(), users.end(),
            [&](const User& u) { return u.id == id; });
        
        if (it == users.end()) {
            return errorResponse("NotFound", "User not found", 404);
        }

        // 更新字段
        if (body.contains("name") && !body["name"].is_null()) {
            it->name = body["name"];
        }
        if (body.contains("class") && !body["class"].is_null()) {
            it->className = body["class"];
        }
        if (body.contains("role") && !body["role"].is_null()) {
            std::string role = body["role"];
            if (role == "admin" || role == "teacher" || role == "student") {
                it->role = role;
            }
        }

        it->updatedAt = dataManager->getCurrentTimestamp();
        dataManager->saveUsers(users);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "PUT /users/" + id, "用户管理");
        }

        return jsonResponse(*it);
    }

    // 删除用户
    crow::response deleteUser(const crow::request& req, const std::string& id) {
        // 验证权限
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        auto users = dataManager->getUsers();
        auto it = std::find_if(users.begin(), users.end(),
            [&](const User& u) { return u.id == id; });
        
        if (it == users.end()) {
            return errorResponse("NotFound", "User not found", 404);
        }

        // 不能删除自己
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value() && currentUser.value().id == id) {
            return errorResponse("Conflict", "Cannot delete yourself", 409);
        }

        users.erase(it);
        dataManager->saveUsers(users);

        // 记录日志
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "DELETE /users/" + id, "用户管理");
        }

        return jsonResponse(std::string("User deleted successfully"));
    }

    // 批量导入用户
    crow::response batchImportUsers(const crow::request& req) {
        // 验证权限
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        // 解析请求体
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        if (!body.contains("users") || !body["users"].is_array()) {
            return errorResponse("BadRequest", "Missing users array", 400);
        }

        auto users = dataManager->getUsers();
        int success = 0;
        int failed = 0;

        for (const auto& userData : body["users"]) {
            try {
                std::string username = userData["username"];
                std::string password = userData["password"];
                std::string role = userData["role"];
                std::string name = userData["name"];
                
                // 验证
                if (role != "admin" && role != "teacher" && role != "student") {
                    failed++;
                    continue;
                }

                // 检查重复
                auto it = std::find_if(users.begin(), users.end(),
                    [&](const User& u) { return u.username == username; });
                if (it != users.end()) {
                    failed++;
                    continue;
                }

                User newUser{
                    dataManager->generateId(),
                    username,
                    authManager->sha256(password), // 存储密码哈希
                    role,
                    name,
                    userData.contains("class") && !userData["class"].is_null() ? 
                        std::optional<std::string>(userData["class"]) : std::nullopt,
                    dataManager->getCurrentTimestamp(),
                    dataManager->getCurrentTimestamp()
                };
                users.push_back(newUser);
                success++;
            } catch (...) {
                failed++;
            }
        }

        dataManager->saveUsers(users);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /users/batch", "用户管理");
        }

        json response = {
            {"success", success},
            {"failed", failed},
            {"message", "导入完成：成功" + std::to_string(success) + "条，失败" + std::to_string(failed) + "条"}
        };
        return jsonResponse(response);
    }

    // 批量删除用户
    crow::response batchDeleteUsers(const crow::request& req) {
        // 验证权限
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        // 解析请求体
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        if (!body.contains("ids") || !body["ids"].is_array()) {
            return errorResponse("BadRequest", "Missing ids array", 400);
        }

        auto users = dataManager->getUsers();
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        std::string currentUserId = currentUser.has_value() ? currentUser.value().id : "";

        int success = 0;
        int failed = 0;

        for (const auto& id : body["ids"]) {
            std::string userId = id;
            
            // 不能删除自己
            if (userId == currentUserId) {
                failed++;
                continue;
            }

            auto it = std::find_if(users.begin(), users.end(),
                [&](const User& u) { return u.id == userId; });
            
            if (it != users.end()) {
                users.erase(it);
                success++;
            } else {
                failed++;
            }
        }

        dataManager->saveUsers(users);

        // 记录日志
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "DELETE /users/batch", "用户管理");
        }

        json response = {
            {"success", success},
            {"failed", failed}
        };
        return jsonResponse(response);
    }

    // 重置密码
    crow::response resetPassword(const crow::request& req, const std::string& id) {
        // 验证权限
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        // 解析请求体
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        if (!body.contains("newPassword")) {
            return errorResponse("BadRequest", "Missing newPassword", 400);
        }

        std::string newPassword = body["newPassword"];
        if (!validatePassword(newPassword)) {
            return errorResponse("BadRequest", "Password must be at least 6 characters", 400);
        }

        auto users = dataManager->getUsers();
        auto it = std::find_if(users.begin(), users.end(),
            [&](const User& u) { return u.id == id; });
        
        if (it == users.end()) {
            return errorResponse("NotFound", "User not found", 404);
        }

        // 更新密码哈希
        it->passwordHash = authManager->sha256(newPassword);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "PUT /users/" + id + "/reset-password", "用户管理");
        }

        return jsonResponse(std::string("Password reset successfully"));
    }

    // 获取用户操作日志
    crow::response getUserLogs(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }

        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (!currentUser.has_value()) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 获取查询参数（简化处理）
        int page = 1;
        int limit = 10;

        auto logs = dataManager->getOperationLogs();
        
        // 筛选当前用户的日志
        std::vector<OperationLog> userLogs;
        for (const auto& log : logs) {
            if (log.userId == currentUser.value().id) {
                userLogs.push_back(log);
            }
        }

        // 分页
        auto result = paginate(userLogs, page, limit);
        return jsonResponse(result);
    }

    // 获取当前用户信息
    crow::response getCurrentUserProfile(const crow::request& req) {
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }

        auto user = authManager->getCurrentUser(token.substr(7));
        if (!user.has_value()) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        return jsonResponse(user.value());
    }

    // 修改密码
    crow::response changePassword(const crow::request& req) {
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }

        // 解析请求体
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        if (!body.contains("oldPassword") || !body.contains("newPassword")) {
            return errorResponse("BadRequest", "Missing oldPassword or newPassword", 400);
        }

        std::string oldPassword = body["oldPassword"];
        std::string newPassword = body["newPassword"];

        if (!validatePassword(newPassword)) {
            return errorResponse("BadRequest", "New password must be at least 6 characters", 400);
        }

        // 执行修改（包含旧密码验证）
        int res = authManager->changePassword(token.substr(7), oldPassword, newPassword);
        if (res == 1) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        } else if (res == 2) {
            return errorResponse("BadRequest", "Old password incorrect", 400);
        } else if (res == 3) {
            return errorResponse("NotFound", "User not found", 404);
        }

        return jsonResponse(std::string("Password changed successfully"));
    }
};

#endif // USER_SERVICE_H