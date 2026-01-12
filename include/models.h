#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 用户模型
struct User {
    std::string id;
    std::string username;
    std::string passwordHash; // 存储密码哈希
    std::string role; // admin, teacher, student
    std::string name;
    std::optional<std::string> className;
    std::string createdAt;
    std::string updatedAt;

    // JSON序列化
    friend void to_json(json& j, const User& u) {
        j = json{
            {"id", u.id},
            {"username", u.username},
            {"passwordHash", u.passwordHash},
            {"role", u.role},
            {"name", u.name},
            {"createdAt", u.createdAt},
            {"updatedAt", u.updatedAt}
        };
        if (u.className.has_value()) {
            j["class"] = u.className.value();
        }
    }

    friend void from_json(const json& j, User& u) {
        j.at("id").get_to(u.id);
        j.at("username").get_to(u.username);
        if (j.contains("passwordHash") && !j["passwordHash"].is_null()) {
            j.at("passwordHash").get_to(u.passwordHash);
        }
        j.at("role").get_to(u.role);
        j.at("name").get_to(u.name);
        if (j.contains("class") && !j["class"].is_null()) {
            u.className = j["class"].get<std::string>();
        }
        j.at("createdAt").get_to(u.createdAt);
        j.at("updatedAt").get_to(u.updatedAt);
    }
};

// 学生模型
struct Student {
    std::string id;
    std::string studentId;
    std::string name;
    std::string className;
    std::optional<std::string> gender;
    std::optional<std::string> phone;
    std::optional<std::string> email;
    std::string createdAt;
    std::string updatedAt;

    friend void to_json(json& j, const Student& s) {
        j = json{
            {"id", s.id},
            {"studentId", s.studentId},
            {"name", s.name},
            {"class", s.className},
            {"createdAt", s.createdAt},
            {"updatedAt", s.updatedAt}
        };
        if (s.gender.has_value()) j["gender"] = s.gender.value();
        if (s.phone.has_value()) j["phone"] = s.phone.value();
        if (s.email.has_value()) j["email"] = s.email.value();
    }

    friend void from_json(const json& j, Student& s) {
        j.at("id").get_to(s.id);
        j.at("studentId").get_to(s.studentId);
        j.at("name").get_to(s.name);
        j.at("class").get_to(s.className);
        if (j.contains("gender") && !j["gender"].is_null()) s.gender = j["gender"].get<std::string>();
        if (j.contains("phone") && !j["phone"].is_null()) s.phone = j["phone"].get<std::string>();
        if (j.contains("email") && !j["email"].is_null()) s.email = j["email"].get<std::string>();
        j.at("createdAt").get_to(s.createdAt);
        j.at("updatedAt").get_to(s.updatedAt);
    }
};

// 课程模型
struct Course {
    std::string id;
    std::string courseId;
    std::string name;
    int credit;
    std::optional<std::string> teacher;
    std::optional<std::string> description;
    std::string createdAt;
    std::string updatedAt;

    friend void to_json(json& j, const Course& c) {
        j = json{
            {"id", c.id},
            {"courseId", c.courseId},
            {"name", c.name},
            {"credit", c.credit},
            {"createdAt", c.createdAt},
            {"updatedAt", c.updatedAt}
        };
        if (c.teacher.has_value()) j["teacher"] = c.teacher.value();
        if (c.description.has_value()) j["description"] = c.description.value();
    }

    friend void from_json(const json& j, Course& c) {
        j.at("id").get_to(c.id);
        j.at("courseId").get_to(c.courseId);
        j.at("name").get_to(c.name);
        j.at("credit").get_to(c.credit);
        if (j.contains("teacher") && !j["teacher"].is_null()) c.teacher = j["teacher"].get<std::string>();
        if (j.contains("description") && !j["description"].is_null()) c.description = j["description"].get<std::string>();
        j.at("createdAt").get_to(c.createdAt);
        j.at("updatedAt").get_to(c.updatedAt);
    }
};

// 成绩模型
struct Grade {
    std::string id;
    std::string studentId;
    std::string studentName;
    std::string courseId;
    std::string courseName;
    int score;
    std::optional<std::string> semester;
    std::string createdAt;
    std::string updatedAt;

    friend void to_json(json& j, const Grade& g) {
        j = json{
            {"id", g.id},
            {"studentId", g.studentId},
            {"studentName", g.studentName},
            {"courseId", g.courseId},
            {"courseName", g.courseName},
            {"score", g.score},
            {"createdAt", g.createdAt},
            {"updatedAt", g.updatedAt}
        };
        if (g.semester.has_value()) j["semester"] = g.semester.value();
    }

    friend void from_json(const json& j, Grade& g) {
        j.at("id").get_to(g.id);
        j.at("studentId").get_to(g.studentId);
        j.at("studentName").get_to(g.studentName);
        j.at("courseId").get_to(g.courseId);
        j.at("courseName").get_to(g.courseName);
        j.at("score").get_to(g.score);
        if (j.contains("semester") && !j["semester"].is_null()) g.semester = j["semester"].get<std::string>();
        j.at("createdAt").get_to(g.createdAt);
        j.at("updatedAt").get_to(g.updatedAt);
    }
};

// 操作日志模型
struct OperationLog {
    std::string id;
    std::string userId;
    std::string username;
    std::string action;
    std::string module;
    std::optional<std::string> ip;
    std::string createdAt;

    friend void to_json(json& j, const OperationLog& log) {
        j = json{
            {"id", log.id},
            {"userId", log.userId},
            {"username", log.username},
            {"action", log.action},
            {"module", log.module},
            {"createdAt", log.createdAt}
        };
        if (log.ip.has_value()) j["ip"] = log.ip.value();
    }

    friend void from_json(const json& j, OperationLog& log) {
        j.at("id").get_to(log.id);
        j.at("userId").get_to(log.userId);
        j.at("username").get_to(log.username);
        j.at("action").get_to(log.action);
        j.at("module").get_to(log.module);
        if (j.contains("ip") && !j["ip"].is_null()) log.ip = j["ip"].get<std::string>();
        j.at("createdAt").get_to(log.createdAt);
    }
};

// 系统日志模型
struct SystemLog {
    std::string id;
    std::string level; // info, warning, error
    std::string message;
    std::string module;
    std::optional<std::string> ip;
    std::string createdAt;

    friend void to_json(json& j, const SystemLog& log) {
        j = json{
            {"id", log.id},
            {"level", log.level},
            {"message", log.message},
            {"module", log.module},
            {"createdAt", log.createdAt}
        };
        if (log.ip.has_value()) j["ip"] = log.ip.value();
    }

    friend void from_json(const json& j, SystemLog& log) {
        j.at("id").get_to(log.id);
        j.at("level").get_to(log.level);
        j.at("message").get_to(log.message);
        j.at("module").get_to(log.module);
        if (j.contains("ip") && !j["ip"].is_null()) log.ip = j["ip"].get<std::string>();
        j.at("createdAt").get_to(log.createdAt);
    }
};

// 备份模型
struct Backup {
    std::string id;
    std::string name;
    long long size;
    std::string createdAt;
    std::string createdBy;

    friend void to_json(json& j, const Backup& b) {
        j = json{
            {"id", b.id},
            {"name", b.name},
            {"size", b.size},
            {"createdAt", b.createdAt},
            {"createdBy", b.createdBy}
        };
    }

    friend void from_json(const json& j, Backup& b) {
        j.at("id").get_to(b.id);
        j.at("name").get_to(b.name);
        j.at("size").get_to(b.size);
        j.at("createdAt").get_to(b.createdAt);
        j.at("createdBy").get_to(b.createdBy);
    }
};

// 系统设置模型
struct SystemSettings {
    int backupInterval;
    int logRetentionDays;
    int maxLoginAttempts;
    int sessionTimeout;

    friend void to_json(json& j, const SystemSettings& s) {
        j = json{
            {"backupInterval", s.backupInterval},
            {"logRetentionDays", s.logRetentionDays},
            {"maxLoginAttempts", s.maxLoginAttempts},
            {"sessionTimeout", s.sessionTimeout}
        };
    }

    friend void from_json(const json& j, SystemSettings& s) {
        j.at("backupInterval").get_to(s.backupInterval);
        j.at("logRetentionDays").get_to(s.logRetentionDays);
        j.at("maxLoginAttempts").get_to(s.maxLoginAttempts);
        j.at("sessionTimeout").get_to(s.sessionTimeout);
    }
};

// JWT Token结构
struct JWTToken {
    std::string token;
    std::string issuedAt;
    std::string expiresAt;
    std::string userId;

    friend void to_json(json& j, const JWTToken& t) {
        j = json{
            {"token", t.token},
            {"issuedAt", t.issuedAt},
            {"expiresAt", t.expiresAt},
            {"userId", t.userId}
        };
    }

    friend void from_json(const json& j, JWTToken& t) {
        j.at("token").get_to(t.token);
        j.at("issuedAt").get_to(t.issuedAt);
        j.at("expiresAt").get_to(t.expiresAt);
        j.at("userId").get_to(t.userId);
    }
};

#endif // MODELS_H