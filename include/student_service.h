#ifndef STUDENT_SERVICE_H
#define STUDENT_SERVICE_H

#include <string>
#include <vector>
#include <optional>
#include <crow.h>
#include "models.h"
#include "data_manager.h"
#include "auth.h"
#include "middleware.h"

class StudentService {
private:
    DataManager* dataManager;
    AuthManager* authManager;
    LogMiddleware* logger;

public:
    StudentService(DataManager* dm, AuthManager* am, LogMiddleware* log) 
        : dataManager(dm), authManager(am), logger(log) {}

    // 获取学生列表
    crow::response getStudents(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 获取查询参数（简化处理）
        int page = 1;
        int limit = 10;
        std::string classFilter = "";
        std::string search = "";

        auto students = dataManager->getStudents();
        
        // 筛选
        std::vector<Student> filtered;
        for (const auto& student : students) {
            if (!classFilter.empty() && student.className != classFilter) continue;
            if (!search.empty()) {
                if (student.studentId.find(search) == std::string::npos &&
                    student.name.find(search) == std::string::npos) continue;
            }
            filtered.push_back(student);
        }

        // 分页
        auto result = paginate(filtered, page, limit);
        
        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /students", "学生管理");
        }

        return jsonResponse(result);
    }

    // 获取学生详情
    crow::response getStudent(const crow::request& req, const std::string& id) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        auto students = dataManager->getStudents();
        auto it = std::find_if(students.begin(), students.end(),
            [&](const Student& s) { return s.id == id; });
        
        if (it == students.end()) {
            return errorResponse("NotFound", "Student not found", 404);
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /students/" + id, "学生管理");
        }

        return jsonResponse(*it);
    }

    // 添加学生
    crow::response createStudent(const crow::request& req) {
        // 验证权限（管理员/教师）
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin", "teacher"})) {
            return errorResponse("Forbidden", "Admin or teacher only", 403);
        }

        // 解析请求体
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        // 验证必填字段
        if (!body.contains("studentId") || !body.contains("name") || !body.contains("class")) {
            return errorResponse("BadRequest", "Missing required fields", 400);
        }

        std::string studentId = body["studentId"];
        std::string name = body["name"];
        std::string className = body["class"];
        
        std::optional<std::string> gender;
        std::optional<std::string> phone;
        std::optional<std::string> email;

        if (body.contains("gender") && !body["gender"].is_null()) {
            gender = body["gender"];
        }
        if (body.contains("phone") && !body["phone"].is_null()) {
            phone = body["phone"];
            if (!validatePhone(phone.value())) {
                return errorResponse("BadRequest", "Invalid phone format", 400);
            }
        }
        if (body.contains("email") && !body["email"].is_null()) {
            email = body["email"];
            if (!validateEmail(email.value())) {
                return errorResponse("BadRequest", "Invalid email format", 400);
            }
        }

        // 检查学号是否已存在
        auto students = dataManager->getStudents();
        auto it = std::find_if(students.begin(), students.end(),
            [&](const Student& s) { return s.studentId == studentId; });
        if (it != students.end()) {
            return errorResponse("Conflict", "Student ID already exists", 409);
        }

        // 创建学生
        Student newStudent{
            dataManager->generateId(),
            studentId,
            name,
            className,
            gender,
            phone,
            email,
            dataManager->getCurrentTimestamp(),
            dataManager->getCurrentTimestamp()
        };
        students.push_back(newStudent);
        dataManager->saveStudents(students);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /students", "学生管理");
        }

        return jsonResponse(newStudent, 201);
    }

    // 更新学生
    crow::response updateStudent(const crow::request& req, const std::string& id) {
        // 验证权限
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin", "teacher"})) {
            return errorResponse("Forbidden", "Admin or teacher only", 403);
        }

        // 解析请求体
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        auto students = dataManager->getStudents();
        auto it = std::find_if(students.begin(), students.end(),
            [&](const Student& s) { return s.id == id; });
        
        if (it == students.end()) {
            return errorResponse("NotFound", "Student not found", 404);
        }

        // 更新字段
        if (body.contains("name") && !body["name"].is_null()) {
            it->name = body["name"];
        }
        if (body.contains("class") && !body["class"].is_null()) {
            it->className = body["class"];
        }
        if (body.contains("gender") && !body["gender"].is_null()) {
            it->gender = body["gender"];
        }
        if (body.contains("phone") && !body["phone"].is_null()) {
            std::string phone = body["phone"];
            if (!validatePhone(phone)) {
                return errorResponse("BadRequest", "Invalid phone format", 400);
            }
            it->phone = phone;
        }
        if (body.contains("email") && !body["email"].is_null()) {
            std::string email = body["email"];
            if (!validateEmail(email)) {
                return errorResponse("BadRequest", "Invalid email format", 400);
            }
            it->email = email;
        }

        it->updatedAt = dataManager->getCurrentTimestamp();
        dataManager->saveStudents(students);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "PUT /students/" + id, "学生管理");
        }

        return jsonResponse(*it);
    }

    // 删除学生
    crow::response deleteStudent(const crow::request& req, const std::string& id) {
        // 验证权限（管理员）
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        auto students = dataManager->getStudents();
        auto it = std::find_if(students.begin(), students.end(),
            [&](const Student& s) { return s.id == id; });
        
        if (it == students.end()) {
            return errorResponse("NotFound", "Student not found", 404);
        }

        students.erase(it);
        dataManager->saveStudents(students);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "DELETE /students/" + id, "学生管理");
        }

        return jsonResponse(std::string("Student deleted successfully"));
    }

    // 获取学生成绩概览
    crow::response getStudentGrades(const crow::request& req, const std::string& studentId) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 检查学生是否存在
        auto students = dataManager->getStudents();
        auto studentIt = std::find_if(students.begin(), students.end(),
            [&](const Student& s) { return s.studentId == studentId; });
        
        if (studentIt == students.end()) {
            return errorResponse("NotFound", "Student not found", 404);
        }

        // 获取成绩
        auto grades = dataManager->getGrades();
        std::vector<Grade> studentGrades;
        for (const auto& grade : grades) {
            if (grade.studentId == studentId) {
                studentGrades.push_back(grade);
            }
        }

        // 计算统计信息
        int totalCourses = studentGrades.size();
        double avgScore = 0.0;
        int passCount = 0;
        int totalScore = 0;

        for (const auto& grade : studentGrades) {
            totalScore += grade.score;
            if (grade.score >= 60) passCount++;
        }

        if (totalCourses > 0) {
            avgScore = static_cast<double>(totalScore) / totalCourses;
        }

        double passRate = totalCourses > 0 ? 
            (static_cast<double>(passCount) / totalCourses) * 100.0 : 0.0;

        // 最近成绩（最多5条）
        std::vector<Grade> recentGrades;
        int count = 0;
        for (auto it = studentGrades.rbegin(); it != studentGrades.rend() && count < 5; ++it, ++count) {
            recentGrades.push_back(*it);
        }

        json result = {
            {"totalCourses", totalCourses},
            {"avgScore", avgScore},
            {"passRate", passRate},
            {"totalScore", totalScore},
            {"recentGrades", json::array()}
        };

        for (const auto& grade : recentGrades) {
            result["recentGrades"].push_back({
                {"courseName", grade.courseName},
                {"score", grade.score},
                {"semester", grade.semester.value_or("")}
            });
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /students/" + studentId + "/grades", "学生管理");
        }

        return jsonResponse(result);
    }

    // 批量导入学生（简化处理，实际应该支持文件上传）
    crow::response batchImportStudents(const crow::request& req) {
        // 验证权限
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        // 解析请求体（JSON数组）
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        if (!body.is_array()) {
            return errorResponse("BadRequest", "Expected array of students", 400);
        }

        auto students = dataManager->getStudents();
        int success = 0;
        int failed = 0;

        for (const auto& studentData : body) {
            try {
                std::string studentId = studentData["studentId"];
                std::string name = studentData["name"];
                std::string className = studentData["class"];

                // 检查重复
                auto it = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == studentId; });
                if (it != students.end()) {
                    failed++;
                    continue;
                }

                Student newStudent{
                    dataManager->generateId(),
                    studentId,
                    name,
                    className,
                    studentData.contains("gender") && !studentData["gender"].is_null() ? 
                        std::optional<std::string>(studentData["gender"]) : std::nullopt,
                    studentData.contains("phone") && !studentData["phone"].is_null() ? 
                        std::optional<std::string>(studentData["phone"]) : std::nullopt,
                    studentData.contains("email") && !studentData["email"].is_null() ? 
                        std::optional<std::string>(studentData["email"]) : std::nullopt,
                    dataManager->getCurrentTimestamp(),
                    dataManager->getCurrentTimestamp()
                };
                students.push_back(newStudent);
                success++;
            } catch (...) {
                failed++;
            }
        }

        dataManager->saveStudents(students);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /students/batch", "学生管理");
        }

        json response = {
            {"success", success},
            {"failed", failed},
            {"message", "导入完成：成功" + std::to_string(success) + "条，失败" + std::to_string(failed) + "条"}
        };
        return jsonResponse(response);
    }

    // 导出学生数据（简化处理，返回JSON）
    crow::response exportStudents(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 获取查询参数
        std::string format = "excel"; // 默认

        auto students = dataManager->getStudents();
        
        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /students/export", "学生管理");
        }

        // 返回JSON数据（实际应该生成Excel/CSV文件）
        json result = json::array();
        for (const auto& student : students) {
            result.push_back(student);
        }

        return jsonResponse(result);
    }
};

#endif // STUDENT_SERVICE_H