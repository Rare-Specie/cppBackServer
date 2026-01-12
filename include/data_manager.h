#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <nlohmann/json.hpp>
#include "models.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

class DataManager {
private:
    std::string dataDir;
    std::mutex mutex;
    
    // 数据文件路径
    std::string getUsersFile() const { return dataDir + "/users.json"; }
    std::string getStudentsFile() const { return dataDir + "/students.json"; }
    std::string getCoursesFile() const { return dataDir + "/courses.json"; }
    std::string getGradesFile() const { return dataDir + "/grades.json"; }
    std::string getOperationLogsFile() const { return dataDir + "/operation_logs.json"; }
    std::string getSystemLogsFile() const { return dataDir + "/system_logs.json"; }
    std::string getBackupsFile() const { return dataDir + "/backups.json"; }
    std::string getSettingsFile() const { return dataDir + "/settings.json"; }
    std::string getTokensFile() const { return dataDir + "/tokens.json"; }

    // 通用文件读写方法
    template<typename T>
    std::vector<T> readData(const std::string& filePath) {
        std::vector<T> items;
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return items;
        }
        
        try {
            json j;
            file >> j;
            if (j.is_array()) {
                for (const auto& item : j) {
                    items.push_back(item.get<T>());
                }
            }
        } catch (...) {
            // 文件可能为空或格式错误
        }
        return items;
    }

    template<typename T>
    void writeData(const std::string& filePath, const std::vector<T>& items) {
        json j = json::array();
        for (const auto& item : items) {
            j.push_back(item);
        }
        
        std::ofstream file(filePath);
        if (file.is_open()) {
            file << j.dump(2);
        }
    }

public:
    // 生成唯一ID
    std::string generateId() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << time_t << "_" << std::rand();
        return ss.str();
    }

    // 获取当前时间戳
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::ctime(&time_t);
        std::string str = ss.str();
        str.pop_back(); // 移除换行符
        return str;
    }

private:

public:
    DataManager(const std::string& dir) : dataDir(dir) {
        // 确保数据目录存在
        if (!fs::exists(dataDir)) {
            fs::create_directories(dataDir);
        }
        
        // 初始化默认数据
        initializeDefaultData();
    }

    void initializeDefaultData() {
        // 初始化系统设置
        if (!fs::exists(getSettingsFile())) {
            SystemSettings settings{
                7,   // backupInterval
                30,  // logRetentionDays
                5,   // maxLoginAttempts
                30   // sessionTimeout
            };
            std::vector<SystemSettings> settingsVec = {settings};
            writeData(getSettingsFile(), settingsVec);
        }

        // 初始化默认用户（管理员、教师、学生）
        if (!fs::exists(getUsersFile())) {
            std::vector<User> users;
            
            // 管理员: admin / admin123
            User admin{
                generateId(),
                "admin",
                "240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9", // SHA256("admin123")
                "admin",
                "管理员",
                "计算机2401",
                getCurrentTimestamp(),
                getCurrentTimestamp()
            };
            users.push_back(admin);
            
            // 教师: teacher / teacher123
            User teacher{
                generateId(),
                "teacher",
                "cde383eee8ee7a4400adf7a15f716f179a2eb97646b37e089eb8d6d04e663416", // SHA256("teacher123")
                "teacher",
                "张老师",
                "计算机2401",
                getCurrentTimestamp(),
                getCurrentTimestamp()
            };
            users.push_back(teacher);
            
            // 学生: student / student123
            User student{
                generateId(),
                "student",
                "703b0a3d6ad75b649a28adde7d83c6251da457549263bc7ff45ec709b0a8448b", // SHA256("student123")
                "student",
                "李学生",
                "计算机2401",
                getCurrentTimestamp(),
                getCurrentTimestamp()
            };
            users.push_back(student);
            
            writeData(getUsersFile(), users);
        }

        // 初始化空数据文件
        if (!fs::exists(getStudentsFile())) {
            writeData(getStudentsFile(), std::vector<Student>{});
        }
        if (!fs::exists(getCoursesFile())) {
            writeData(getCoursesFile(), std::vector<Course>{});
        }
        if (!fs::exists(getGradesFile())) {
            writeData(getGradesFile(), std::vector<Grade>{});
        }
        if (!fs::exists(getOperationLogsFile())) {
            writeData(getOperationLogsFile(), std::vector<OperationLog>{});
        }
        if (!fs::exists(getSystemLogsFile())) {
            writeData(getSystemLogsFile(), std::vector<SystemLog>{});
        }
        if (!fs::exists(getBackupsFile())) {
            writeData(getBackupsFile(), std::vector<Backup>{});
        }
        if (!fs::exists(getTokensFile())) {
            writeData(getTokensFile(), std::vector<JWTToken>{});
        }
    }

    // 用户管理
    std::vector<User> getUsers() {
        std::lock_guard<std::mutex> lock(mutex);
        return readData<User>(getUsersFile());
    }

    void saveUsers(const std::vector<User>& users) {
        std::lock_guard<std::mutex> lock(mutex);
        writeData(getUsersFile(), users);
    }

    // 学生管理
    std::vector<Student> getStudents() {
        std::lock_guard<std::mutex> lock(mutex);
        return readData<Student>(getStudentsFile());
    }

    void saveStudents(const std::vector<Student>& students) {
        std::lock_guard<std::mutex> lock(mutex);
        writeData(getStudentsFile(), students);
    }

    // 课程管理
    std::vector<Course> getCourses() {
        std::lock_guard<std::mutex> lock(mutex);
        return readData<Course>(getCoursesFile());
    }

    void saveCourses(const std::vector<Course>& courses) {
        std::lock_guard<std::mutex> lock(mutex);
        writeData(getCoursesFile(), courses);
    }

    // 成绩管理
    std::vector<Grade> getGrades() {
        std::lock_guard<std::mutex> lock(mutex);
        return readData<Grade>(getGradesFile());
    }

    void saveGrades(const std::vector<Grade>& grades) {
        std::lock_guard<std::mutex> lock(mutex);
        writeData(getGradesFile(), grades);
    }

    // 操作日志
    std::vector<OperationLog> getOperationLogs() {
        std::lock_guard<std::mutex> lock(mutex);
        return readData<OperationLog>(getOperationLogsFile());
    }

    void saveOperationLogs(const std::vector<OperationLog>& logs) {
        std::lock_guard<std::mutex> lock(mutex);
        writeData(getOperationLogsFile(), logs);
    }

    // 系统日志
    std::vector<SystemLog> getSystemLogs() {
        std::lock_guard<std::mutex> lock(mutex);
        return readData<SystemLog>(getSystemLogsFile());
    }

    void saveSystemLogs(const std::vector<SystemLog>& logs) {
        std::lock_guard<std::mutex> lock(mutex);
        writeData(getSystemLogsFile(), logs);
    }

    // 备份管理
    std::vector<Backup> getBackups() {
        std::lock_guard<std::mutex> lock(mutex);
        return readData<Backup>(getBackupsFile());
    }

    void saveBackups(const std::vector<Backup>& backups) {
        std::lock_guard<std::mutex> lock(mutex);
        writeData(getBackupsFile(), backups);
    }

    // 系统设置
    SystemSettings getSettings() {
        std::lock_guard<std::mutex> lock(mutex);
        auto settings = readData<SystemSettings>(getSettingsFile());
        if (settings.empty()) {
            return SystemSettings{7, 30, 5, 30};
        }
        return settings[0];
    }

    void saveSettings(const SystemSettings& settings) {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<SystemSettings> settingsVec = {settings};
        writeData(getSettingsFile(), settingsVec);
    }

    // Token管理
    std::vector<JWTToken> getTokens() {
        std::lock_guard<std::mutex> lock(mutex);
        return readData<JWTToken>(getTokensFile());
    }

    void saveTokens(const std::vector<JWTToken>& tokens) {
        std::lock_guard<std::mutex> lock(mutex);
        writeData(getTokensFile(), tokens);
    }

    // 备份数据
    bool backupData(const std::string& backupName, const std::string& createdBy) {
        try {
            std::string backupDir = dataDir + "/backups/" + backupName;
            fs::create_directories(backupDir);
            
            // 复制所有数据文件
            std::vector<std::string> files = {
                "users.json", "students.json", "courses.json", "grades.json",
                "operation_logs.json", "system_logs.json", "settings.json"
            };
            
            long long totalSize = 0;
            for (const auto& file : files) {
                std::string src = dataDir + "/" + file;
                std::string dst = backupDir + "/" + file;
                if (fs::exists(src)) {
                    fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
                    totalSize += fs::file_size(src);
                }
            }
            
            // 记录备份信息
            auto backups = getBackups();
            Backup backup{
                generateId(),
                backupName,
                totalSize,
                getCurrentTimestamp(),
                createdBy
            };
            backups.push_back(backup);
            saveBackups(backups);
            
            return true;
        } catch (...) {
            return false;
        }
    }

    // 恢复备份
    bool restoreBackup(const std::string& backupId) {
        try {
            auto backups = getBackups();
            auto it = std::find_if(backups.begin(), backups.end(), 
                [&](const Backup& b) { return b.id == backupId; });
            if (it == backups.end()) return false;
            
            std::string backupDir = dataDir + "/backups/" + it->name;
            if (!fs::exists(backupDir)) return false;
            
            // 恢复所有文件
            std::vector<std::string> files = {
                "users.json", "students.json", "courses.json", "grades.json",
                "operation_logs.json", "system_logs.json", "settings.json"
            };
            
            for (const auto& file : files) {
                std::string src = backupDir + "/" + file;
                std::string dst = dataDir + "/" + file;
                if (fs::exists(src)) {
                    fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
                }
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }

    // 删除备份
    bool deleteBackup(const std::string& backupId) {
        try {
            auto backups = getBackups();
            auto it = std::find_if(backups.begin(), backups.end(), 
                [&](const Backup& b) { return b.id == backupId; });
            if (it == backups.end()) return false;
            
            std::string backupDir = dataDir + "/backups/" + it->name;
            if (fs::exists(backupDir)) {
                fs::remove_all(backupDir);
            }
            
            backups.erase(it);
            saveBackups(backups);
            
            return true;
        } catch (...) {
            return false;
        }
    }

    // 清理日志
    void cleanLogs(int retentionDays) {
        auto now = std::chrono::system_clock::now();
        auto cutoff = now - std::chrono::hours(24 * retentionDays);
        
        // 清理操作日志
        auto opLogs = getOperationLogs();
        opLogs.erase(std::remove_if(opLogs.begin(), opLogs.end(),
            [&](const OperationLog& log) {
                std::tm tm = {};
                std::istringstream ss(log.createdAt);
                ss >> std::get_time(&tm, "%a %b %d %H:%M:%S %Y");
                auto logTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));
                return logTime < cutoff;
            }), opLogs.end());
        saveOperationLogs(opLogs);
        
        // 清理系统日志
        auto sysLogs = getSystemLogs();
        sysLogs.erase(std::remove_if(sysLogs.begin(), sysLogs.end(),
            [&](const SystemLog& log) {
                std::tm tm = {};
                std::istringstream ss(log.createdAt);
                ss >> std::get_time(&tm, "%a %b %d %H:%M:%S %Y");
                auto logTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));
                return logTime < cutoff;
            }), sysLogs.end());
        saveSystemLogs(sysLogs);
    }
};

#endif // DATA_MANAGER_H