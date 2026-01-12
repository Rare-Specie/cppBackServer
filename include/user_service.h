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

        // 解析分页参数（支持字符串和整数）
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        
        // 获取过滤参数
        std::string role = req.get_header_value("X-Query-Role");
        std::string search = req.get_header_value("X-Query-Search");

        auto users = dataManager->getUsers();
        
        // 筛选（先过滤再分页）
        std::vector<User> filtered;
        for (const auto& user : users) {
            if (!role.empty() && user.role != role) continue;
            if (!search.empty() && 
                user.username.find(search) == std::string::npos &&
                user.name.find(search) == std::string::npos) continue;
            filtered.push_back(user);
        }

        // 分页（使用ISO日期格式）
        auto result = paginateWithISO(filtered, page, limit, 
            [this](const std::string& ts) { return dataManager->convertToISO8601(ts); });
        
        // 记录日志（包含分页参数）
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            std::string logMsg = "GET /users | page=" + std::to_string(page) + 
                               ", limit=" + std::to_string(limit) + 
                               ", filtered=" + std::to_string(filtered.size());
            logger->logOperation(currentUser.value().id, currentUser.value().username, 
                               logMsg, "用户管理");
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

        // 可选 studentId（仅在 role == "student" 时使用）
        std::optional<std::string> studentId;
        if (body.contains("studentId") && !body["studentId"].is_null()) {
            studentId = body["studentId"];
        }

        // 验证角色
        if (role != "admin" && role != "teacher" && role != "student") {
            return errorResponse("BadRequest", "Invalid role", 400);
        }

        // 如果是学生且传入了 studentId，则验证该学生存在
        if (role == "student" && studentId.has_value()) {
            auto students = dataManager->getStudents();
            auto sIt = std::find_if(students.begin(), students.end(),
                [&](const Student& s) { return s.studentId == studentId.value(); });
            if (sIt == students.end()) {
                return errorResponse("NotFound", "Student record not found for given studentId", 404);
            }
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
            studentId,
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
        // 更新 studentId（仅适用于学生账号）
        if (body.contains("studentId")) {
            if (body["studentId"].is_null()) {
                it->studentId = std::nullopt;
            } else {
                std::string newStudentId = body["studentId"];
                // 验证学生存在
                auto students = dataManager->getStudents();
                auto sIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == newStudentId; });
                if (sIt == students.end()) {
                    return errorResponse("NotFound", "Student record not found for given studentId", 404);
                }
                it->studentId = newStudentId;
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

    // 批量导入用户（支持两种格式）
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

        // 支持两种格式：直接数组或 {users: [...]}
        json usersArray;
        if (body.is_array()) {
            usersArray = body;
        } else if (body.is_object() && body.contains("users") && body["users"].is_array()) {
            usersArray = body["users"];
        } else {
            return errorResponse("BadRequest", "Expected array of users or {users: [...]}", 400);
        }

        auto existingUsers = dataManager->getUsers();
        json successItems = json::array();
        json failedItems = json::array();

        for (size_t i = 0; i < usersArray.size(); ++i) {
            const auto& userData = usersArray[i];
            json errorDetails = json::object();
            errorDetails["index"] = i;
            
            try {
                // 验证必填字段
                if (!userData.contains("username") || userData["username"].is_null()) {
                    errorDetails["error"] = "Missing required field: username";
                    failedItems.push_back(errorDetails);
                    continue;
                }
                if (!userData.contains("password") || userData["password"].is_null()) {
                    errorDetails["error"] = "Missing required field: password";
                    failedItems.push_back(errorDetails);
                    continue;
                }
                if (!userData.contains("role") || userData["role"].is_null()) {
                    errorDetails["error"] = "Missing required field: role";
                    failedItems.push_back(errorDetails);
                    continue;
                }
                if (!userData.contains("name") || userData["name"].is_null()) {
                    errorDetails["error"] = "Missing required field: name";
                    failedItems.push_back(errorDetails);
                    continue;
                }

                std::string username = userData["username"];
                std::string password = userData["password"];
                std::string role = userData["role"];
                std::string name = userData["name"];
                
                // 验证角色
                if (role != "admin" && role != "teacher" && role != "student") {
                    errorDetails["error"] = "Invalid role: " + role;
                    failedItems.push_back(errorDetails);
                    continue;
                }

                // 验证密码长度
                if (password.length() < 6) {
                    errorDetails["error"] = "Password must be at least 6 characters";
                    failedItems.push_back(errorDetails);
                    continue;
                }

                // 检查重复
                auto it = std::find_if(existingUsers.begin(), existingUsers.end(),
                    [&](const User& u) { return u.username == username; });
                if (it != existingUsers.end()) {
                    errorDetails["error"] = "Username already exists: " + username;
                    failedItems.push_back(errorDetails);
                    continue;
                }

                std::optional<std::string> batchStudentId;
                if (userData.contains("studentId") && !userData["studentId"].is_null()) {
                    batchStudentId = userData["studentId"];
                    // 如果是学生，验证学生记录存在
                    if (role == "student") {
                        auto students = dataManager->getStudents();
                        auto sIt = std::find_if(students.begin(), students.end(),
                            [&](const Student& s) { return s.studentId == batchStudentId.value(); });
                        if (sIt == students.end()) {
                            errorDetails["error"] = "Student record not found for studentId: " + batchStudentId.value();
                            failedItems.push_back(errorDetails);
                            continue;
                        }
                    }
                }

                // 创建用户
                User newUser{
                    dataManager->generateId(),
                    username,
                    authManager->sha256(password), // 存储密码哈希
                    role,
                    name,
                    userData.contains("class") && !userData["class"].is_null() ? 
                        std::optional<std::string>(userData["class"]) : std::nullopt,
                    batchStudentId,
                    dataManager->getCurrentTimestamp(),
                    dataManager->getCurrentTimestamp()
                };
                existingUsers.push_back(newUser);
                
                // 记录成功项
                json successItem = json::object();
                successItem["index"] = i;
                successItem["username"] = username;
                successItem["role"] = role;
                successItems.push_back(successItem);
                
            } catch (const std::exception& e) {
                errorDetails["error"] = "Unexpected error: " + std::string(e.what());
                failedItems.push_back(errorDetails);
            } catch (...) {
                errorDetails["error"] = "Unknown error occurred";
                failedItems.push_back(errorDetails);
            }
        }

        // 保存数据
        if (successItems.size() > 0) {
            dataManager->saveUsers(existingUsers);
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            std::string logMsg = "POST /users/batch | total=" + std::to_string(usersArray.size()) +
                               ", success=" + std::to_string(successItems.size()) +
                               ", failed=" + std::to_string(failedItems.size());
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               logMsg, "用户管理");
        }

        json response = {
            {"success", successItems.size()},
            {"failed", failedItems.size()},
            {"successItems", successItems},
            {"failedItems", failedItems},
            {"message", "导入完成：成功" + std::to_string(successItems.size()) + "条，失败" + std::to_string(failedItems.size()) + "条"}
        };
        
        // 如果有失败项，返回400状态码
        if (failedItems.size() > 0 && successItems.size() == 0) {
            return jsonResponse(response, 400);
        } else if (failedItems.size() > 0) {
            return jsonResponse(response, 207); // 部分成功
        }
        return jsonResponse(response, 201);
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

        // 解析分页参数（支持字符串和整数）
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        
        // 解析字段选择参数
        std::vector<std::string> fields = parseFieldsParam(req);

        auto logs = dataManager->getOperationLogs();
        
        // 筛选当前用户的日志
        std::vector<OperationLog> userLogs;
        for (const auto& log : logs) {
            if (log.userId == currentUser.value().id) {
                userLogs.push_back(log);
            }
        }

        // 分页（使用ISO日期格式）
        auto result = paginateWithISO(userLogs, page, limit, 
            [this](const std::string& ts) { return dataManager->convertToISO8601(ts); });
        
        // 如果指定了fields，进行字段过滤
        if (!fields.empty() && result.contains("data")) {
            json filteredData = json::array();
            for (auto& item : result["data"]) {
                json filteredItem;
                for (const auto& field : fields) {
                    if (item.contains(field)) {
                        filteredItem[field] = item[field];
                    }
                }
                filteredData.push_back(filteredItem);
            }
            result["data"] = filteredData;
        }
        
        // 记录日志（包含分页参数）
        std::string logMsg = "GET /user/logs | page=" + std::to_string(page) + 
                           ", limit=" + std::to_string(limit) + 
                           ", total=" + std::to_string(userLogs.size());
        logger->logOperation(currentUser.value().id, currentUser.value().username,
                           logMsg, "用户管理");

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