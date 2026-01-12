#ifndef SYSTEM_SERVICE_H
#define SYSTEM_SERVICE_H

#include <string>
#include <vector>
#include <optional>
#include <crow.h>
#include <fstream>
#include <sstream>
#include "models.h"
#include "data_manager.h"
#include "auth.h"
#include "middleware.h"

class SystemService {
private:
    DataManager* dataManager;
    AuthManager* authManager;
    LogMiddleware* logger;

public:
    SystemService(DataManager* dm, AuthManager* am, LogMiddleware* log) 
        : dataManager(dm), authManager(am), logger(log) {}

    // 创建备份
    crow::response createBackup(const crow::request& req) {
        // 验证权限（管理员）
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (!currentUser.has_value()) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 生成备份名称
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "backup_" << time_t << ".zip";
        std::string backupName = ss.str();

        // 执行备份
        bool success = dataManager->backupData(backupName, currentUser.value().username);

        if (!success) {
            return errorResponse("InternalError", "Backup failed", 500);
        }

        // 获取备份信息
        auto backups = dataManager->getBackups();
        auto backupIt = std::find_if(backups.begin(), backups.end(),
            [&](const Backup& b) { return b.name == backupName; });

        if (backupIt == backups.end()) {
            return errorResponse("InternalError", "Backup created but info not found", 500);
        }

        // 记录日志
        logger->logOperation(currentUser.value().id, currentUser.value().username,
                           "POST /system/backup", "系统管理");

        return jsonResponse(*backupIt, 201);
    }

    // 获取备份列表
    crow::response getBackups(const crow::request& req) {
        // 验证权限（管理员）
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        auto backups = dataManager->getBackups();

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /system/backups", "系统管理");
        }

        return jsonResponse(backups);
    }

    // 恢复备份
    crow::response restoreBackup(const crow::request& req) {
        // 验证权限（管理员）
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

        if (!body.contains("backupId")) {
            return errorResponse("BadRequest", "Missing backupId", 400);
        }

        std::string backupId = body["backupId"];

        // 执行恢复
        bool success = dataManager->restoreBackup(backupId);

        if (!success) {
            return errorResponse("InternalError", "Restore failed", 500);
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /system/restore", "系统管理");
        }

        return jsonResponse(std::string("Backup restored successfully"));
    }

    // 删除备份
    crow::response deleteBackup(const crow::request& req, const std::string& backupId) {
        // 验证权限（管理员）
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        // 执行删除
        bool success = dataManager->deleteBackup(backupId);

        if (!success) {
            return errorResponse("NotFound", "Backup not found", 404);
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "DELETE /system/backups/" + backupId, "系统管理");
        }

        return jsonResponse(std::string("Backup deleted successfully"));
    }

    // 获取系统日志
    crow::response getSystemLogs(const crow::request& req) {
        // 验证权限（管理员）
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
        std::string level = req.get_header_value("X-Query-Level");
        std::string startTime = req.get_header_value("X-Query-StartTime");
        std::string endTime = req.get_header_value("X-Query-EndTime");
        
        // 解析字段选择参数
        bool fullData = requestFullData(req);
        std::vector<std::string> fields = parseFieldsParam(req);

        auto logs = dataManager->getSystemLogs();
        
        // 筛选（先过滤再分页）
        std::vector<SystemLog> filtered;
        for (const auto& log : logs) {
            if (!level.empty() && log.level != level) continue;
            // 时间筛选简化处理
            filtered.push_back(log);
        }

        // 分页（使用ISO日期格式）
        auto result = paginateWithISO(filtered, page, limit, 
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
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /system/logs", "系统管理");
        }

        return jsonResponse(result);
    }

    // 获取系统设置
    crow::response getSettings(const crow::request& req) {
        // 验证权限（管理员）
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        auto settings = dataManager->getSettings();

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /system/settings", "系统管理");
        }

        return jsonResponse(settings);
    }

    // 更新系统设置
    crow::response updateSettings(const crow::request& req) {
        // 验证权限（管理员）
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
        if (!body.contains("backupInterval") || !body.contains("logRetentionDays") ||
            !body.contains("maxLoginAttempts") || !body.contains("sessionTimeout")) {
            return errorResponse("BadRequest", "Missing required fields", 400);
        }

        SystemSettings settings{
            body["backupInterval"],
            body["logRetentionDays"],
            body["maxLoginAttempts"],
            body["sessionTimeout"]
        };

        dataManager->saveSettings(settings);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "PUT /system/settings", "系统管理");
        }

        return jsonResponse(std::string("Settings updated successfully"));
    }

    // 清理日志
    crow::response cleanLogs(const crow::request& req) {
        // 验证权限（管理员）
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        // 获取设置中的保留天数
        auto settings = dataManager->getSettings();
        dataManager->cleanLogs(settings.logRetentionDays);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /system/clean-logs", "系统管理");
        }

        return jsonResponse(std::string("Logs cleaned successfully"));
    }

    // 导出日志（简化处理，返回CSV格式的JSON）
    crow::response exportLogs(const crow::request& req) {
        // 验证权限（管理员）
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        // 获取查询参数
        std::string level = req.get_header_value("X-Query-Level");
        std::string startTime = req.get_header_value("X-Query-StartTime");
        std::string endTime = req.get_header_value("X-Query-EndTime");

        auto logs = dataManager->getSystemLogs();
        
        // 筛选
        std::vector<SystemLog> filtered;
        for (const auto& log : logs) {
            if (!level.empty() && log.level != level) continue;
            // 时间筛选简化处理
            filtered.push_back(log);
        }

        // 生成CSV内容（简化处理，返回JSON格式）
        json result = json::array();
        for (const auto& log : filtered) {
            result.push_back({
                {"id", log.id},
                {"level", log.level},
                {"message", log.message},
                {"module", log.module},
                {"ip", log.ip.value_or("")},
                {"createdAt", dataManager->convertToISO8601(log.createdAt)}
            });
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /system/export-logs", "系统管理");
        }

        // 返回JSON（实际应该返回CSV文件）
        return jsonResponse(result);
    }
};

#endif // SYSTEM_SERVICE_H