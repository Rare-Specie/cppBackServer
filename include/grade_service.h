#ifndef GRADE_SERVICE_H
#define GRADE_SERVICE_H

#include <string>
#include <vector>
#include <optional>
#include <crow.h>
#include "models.h"
#include "data_manager.h"
#include "auth.h"
#include "middleware.h"

class GradeService {
private:
    DataManager* dataManager;
    AuthManager* authManager;
    LogMiddleware* logger;

public:
    GradeService(DataManager* dm, AuthManager* am, LogMiddleware* log) 
        : dataManager(dm), authManager(am), logger(log) {}

    // 获取成绩列表
    crow::response getGrades(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 获取查询参数
        int page = 1;
        int limit = 10;
        std::string studentId = "";
        std::string courseId = "";
        std::string classFilter = "";

        auto grades = dataManager->getGrades();
        auto students = dataManager->getStudents();
        auto courses = dataManager->getCourses();
        
        // 筛选
        std::vector<Grade> filtered;
        for (const auto& grade : grades) {
            if (!studentId.empty() && grade.studentId != studentId) continue;
            if (!courseId.empty() && grade.courseId != courseId) continue;
            
            if (!classFilter.empty()) {
                // 查找学生班级
                auto studentIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == grade.studentId; });
                if (studentIt == students.end() || studentIt->className != classFilter) continue;
            }
            
            filtered.push_back(grade);
        }

        // 分页
        auto result = paginate(filtered, page, limit);
        
        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /grades", "成绩管理");
        }

        return jsonResponse(result);
    }

    // 录入成绩
    crow::response createGrade(const crow::request& req) {
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
        if (!body.contains("studentId") || !body.contains("courseId") || !body.contains("score")) {
            return errorResponse("BadRequest", "Missing required fields", 400);
        }

        std::string studentId = body["studentId"];
        std::string courseId = body["courseId"];
        int score = body["score"];
        
        // 验证成绩范围
        if (!validateScore(score)) {
            return errorResponse("BadRequest", "Score must be between 0 and 100", 400);
        }

        // 验证学生和课程是否存在
        auto students = dataManager->getStudents();
        auto studentIt = std::find_if(students.begin(), students.end(),
            [&](const Student& s) { return s.studentId == studentId; });
        if (studentIt == students.end()) {
            return errorResponse("NotFound", "Student not found", 404);
        }

        auto courses = dataManager->getCourses();
        auto courseIt = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.courseId == courseId; });
        if (courseIt == courses.end()) {
            return errorResponse("NotFound", "Course not found", 404);
        }

        // 检查是否已存在相同成绩记录
        auto grades = dataManager->getGrades();
        auto gradeIt = std::find_if(grades.begin(), grades.end(),
            [&](const Grade& g) { 
                return g.studentId == studentId && g.courseId == courseId && 
                       g.semester == (body.contains("semester") && !body["semester"].is_null() ? 
                           std::optional<std::string>(body["semester"]) : std::nullopt);
            });
        if (gradeIt != grades.end()) {
            return errorResponse("Conflict", "Grade already exists for this student and course", 409);
        }

        // 创建成绩
        std::optional<std::string> semester;
        if (body.contains("semester") && !body["semester"].is_null()) {
            semester = body["semester"];
        }

        Grade newGrade{
            dataManager->generateId(),
            studentId,
            studentIt->name,
            courseId,
            courseIt->name,
            score,
            semester,
            dataManager->getCurrentTimestamp(),
            dataManager->getCurrentTimestamp()
        };
        grades.push_back(newGrade);
        dataManager->saveGrades(grades);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /grades", "成绩管理");
        }

        return jsonResponse(newGrade, 201);
    }

    // 更新成绩
    crow::response updateGrade(const crow::request& req, const std::string& id) {
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

        if (!body.contains("score")) {
            return errorResponse("BadRequest", "Missing score", 400);
        }

        int score = body["score"];
        if (!validateScore(score)) {
            return errorResponse("BadRequest", "Score must be between 0 and 100", 400);
        }

        auto grades = dataManager->getGrades();
        auto it = std::find_if(grades.begin(), grades.end(),
            [&](const Grade& g) { return g.id == id; });
        
        if (it == grades.end()) {
            return errorResponse("NotFound", "Grade not found", 404);
        }

        it->score = score;
        it->updatedAt = dataManager->getCurrentTimestamp();
        dataManager->saveGrades(grades);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "PUT /grades/" + id, "成绩管理");
        }

        return jsonResponse(*it);
    }

    // 删除成绩
    crow::response deleteGrade(const crow::request& req, const std::string& id) {
        // 验证权限（管理员/教师）
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin", "teacher"})) {
            return errorResponse("Forbidden", "Admin or teacher only", 403);
        }

        auto grades = dataManager->getGrades();
        auto it = std::find_if(grades.begin(), grades.end(),
            [&](const Grade& g) { return g.id == id; });
        
        if (it == grades.end()) {
            return errorResponse("NotFound", "Grade not found", 404);
        }

        grades.erase(it);
        dataManager->saveGrades(grades);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "DELETE /grades/" + id, "成绩管理");
        }

        return jsonResponse(std::string("Grade deleted successfully"));
    }

    // 获取课程成绩列表
    crow::response getCourseGrades(const crow::request& req, const std::string& courseId) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 检查课程是否存在
        auto courses = dataManager->getCourses();
        auto courseIt = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.id == courseId; });
        
        if (courseIt == courses.end()) {
            return errorResponse("NotFound", "Course not found", 404);
        }

        // 获取查询参数
        std::string semester = "";

        auto grades = dataManager->getGrades();
        auto students = dataManager->getStudents();
        
        std::vector<json> result;

        for (const auto& grade : grades) {
            if (grade.courseId == courseId) {
                if (!semester.empty() && grade.semester != semester) continue;

                // 查找学生信息
                auto studentIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == grade.studentId; });
                
                if (studentIt != students.end()) {
                    json item = {
                        {"studentId", grade.studentId},
                        {"name", studentIt->name},
                        {"class", studentIt->className},
                        {"score", grade.score},
                        {"gradeId", grade.id}
                    };
                    result.push_back(item);
                }
            }
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /grades/course/" + courseId, "成绩管理");
        }

        return jsonResponse(result);
    }

    // 批量更新成绩
    crow::response batchUpdateGrades(const crow::request& req) {
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

        if (!body.contains("courseId") || !body.contains("semester") || !body.contains("grades")) {
            return errorResponse("BadRequest", "Missing required fields", 400);
        }

        std::string courseId = body["courseId"];
        std::string semester = body["semester"];
        auto gradesArray = body["grades"];

        // 验证课程是否存在
        auto courses = dataManager->getCourses();
        auto courseIt = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.courseId == courseId; });
        if (courseIt == courses.end()) {
            return errorResponse("NotFound", "Course not found", 404);
        }

        auto students = dataManager->getStudents();
        auto existingGrades = dataManager->getGrades();
        
        int success = 0;
        int failed = 0;

        for (const auto& gradeData : gradesArray) {
            try {
                std::string studentId = gradeData["studentId"];
                int score = gradeData["score"];

                // 验证成绩范围
                if (!validateScore(score)) {
                    failed++;
                    continue;
                }

                // 验证学生是否存在
                auto studentIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == studentId; });
                if (studentIt == students.end()) {
                    failed++;
                    continue;
                }

                // 查找是否已存在记录
                auto it = std::find_if(existingGrades.begin(), existingGrades.end(),
                    [&](const Grade& g) { 
                        return g.studentId == studentId && g.courseId == courseId && 
                               g.semester == semester;
                    });

                if (it != existingGrades.end()) {
                    // 更新现有成绩
                    it->score = score;
                    it->updatedAt = dataManager->getCurrentTimestamp();
                } else {
                    // 创建新成绩
                    Grade newGrade{
                        dataManager->generateId(),
                        studentId,
                        studentIt->name,
                        courseId,
                        courseIt->name,
                        score,
                        semester,
                        dataManager->getCurrentTimestamp(),
                        dataManager->getCurrentTimestamp()
                    };
                    existingGrades.push_back(newGrade);
                }
                success++;
            } catch (...) {
                failed++;
            }
        }

        dataManager->saveGrades(existingGrades);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /grades/batch-update", "成绩管理");
        }

        json response = {
            {"success", success},
            {"failed", failed}
        };
        return jsonResponse(response);
    }

    // 批量导入成绩（简化处理）
    crow::response batchImportGrades(const crow::request& req) {
        // 验证权限
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin", "teacher"})) {
            return errorResponse("Forbidden", "Admin or teacher only", 403);
        }

        // 解析请求体（JSON数组）
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        if (!body.is_array()) {
            return errorResponse("BadRequest", "Expected array of grades", 400);
        }

        auto students = dataManager->getStudents();
        auto courses = dataManager->getCourses();
        auto grades = dataManager->getGrades();
        
        int success = 0;
        int failed = 0;

        for (const auto& gradeData : body) {
            try {
                std::string studentId = gradeData["studentId"];
                std::string courseId = gradeData["courseId"];
                int score = gradeData["score"];
                std::string semester = gradeData["semester"];

                // 验证
                if (!validateScore(score)) {
                    failed++;
                    continue;
                }

                auto studentIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == studentId; });
                if (studentIt == students.end()) {
                    failed++;
                    continue;
                }

                auto courseIt = std::find_if(courses.begin(), courses.end(),
                    [&](const Course& c) { return c.courseId == courseId; });
                if (courseIt == courses.end()) {
                    failed++;
                    continue;
                }

                // 检查重复
                auto gradeIt = std::find_if(grades.begin(), grades.end(),
                    [&](const Grade& g) { 
                        return g.studentId == studentId && g.courseId == courseId && 
                               g.semester == semester;
                    });
                if (gradeIt != grades.end()) {
                    failed++;
                    continue;
                }

                Grade newGrade{
                    dataManager->generateId(),
                    studentId,
                    studentIt->name,
                    courseId,
                    courseIt->name,
                    score,
                    semester,
                    dataManager->getCurrentTimestamp(),
                    dataManager->getCurrentTimestamp()
                };
                grades.push_back(newGrade);
                success++;
            } catch (...) {
                failed++;
            }
        }

        dataManager->saveGrades(grades);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /grades/batch", "成绩管理");
        }

        json response = {
            {"success", success},
            {"failed", failed},
            {"message", "导入完成：成功" + std::to_string(success) + "条，失败" + std::to_string(failed) + "条"}
        };
        return jsonResponse(response);
    }

    // 导出成绩数据（简化处理，返回JSON）
    crow::response exportGrades(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 获取查询参数
        std::string studentId = "";
        std::string courseId = "";
        std::string classFilter = "";
        std::string format = "excel";

        auto grades = dataManager->getGrades();
        auto students = dataManager->getStudents();
        
        // 筛选
        std::vector<Grade> filtered;
        for (const auto& grade : grades) {
            if (!studentId.empty() && grade.studentId != studentId) continue;
            if (!courseId.empty() && grade.courseId != courseId) continue;
            
            if (!classFilter.empty()) {
                auto studentIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == grade.studentId; });
                if (studentIt == students.end() || studentIt->className != classFilter) continue;
            }
            
            filtered.push_back(grade);
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /grades/export", "成绩管理");
        }

        // 返回JSON数据（实际应该生成Excel/CSV文件）
        json result = json::array();
        for (const auto& grade : filtered) {
            result.push_back(grade);
        }

        return jsonResponse(result);
    }
};

#endif // GRADE_SERVICE_H