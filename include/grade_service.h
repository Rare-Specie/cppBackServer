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

        // 解析分页参数（支持字符串和整数）
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        
        // 获取过滤参数
        std::string studentId = req.get_header_value("X-Query-StudentId");
        std::string courseId = req.get_header_value("X-Query-CourseId");
        std::string classFilter = req.get_header_value("X-Query-Class");
        std::string semester = req.get_header_value("X-Query-Semester");
        
        // 解析字段选择参数
        bool fullData = requestFullData(req);
        std::vector<std::string> fields = parseFieldsParam(req);

        // 如果是学生角色，则强制使用其绑定的 studentId，确保只能看到自己的成绩
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            if (currentUser.value().role == "student") {
                if (currentUser.value().studentId.has_value()) {
                    studentId = currentUser.value().studentId.value();
                } else {
                    return errorResponse("Forbidden", "Student account not bound to a student record", 403);
                }
            }
        }

        auto grades = dataManager->getGrades();
        auto students = dataManager->getStudents();
        
        // 筛选（先过滤再分页）
        std::vector<Grade> filtered;
        for (const auto& grade : grades) {
            if (!studentId.empty() && grade.studentId != studentId) continue;
            if (!courseId.empty() && grade.courseId != courseId) continue;
            if (!semester.empty() && grade.semester.value_or("") != semester) continue;
            
            if (!classFilter.empty()) {
                // 查找学生班级
                auto studentIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == grade.studentId; });
                if (studentIt == students.end() || studentIt->className != classFilter) continue;
            }
            
            filtered.push_back(grade);
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
        if (currentUser.has_value()) {
            std::string logMsg = "GET /grades | page=" + std::to_string(page) + 
                               ", limit=" + std::to_string(limit) + 
                               ", filtered=" + std::to_string(filtered.size());
            if (!fields.empty()) {
                logMsg += ", fields=" + std::to_string(fields.size());
            }
            if (fullData) {
                logMsg += ", full=true";
            }
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               logMsg, "成绩管理");
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

        // 解析分页参数（支持字符串和整数）
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        
        // 获取过滤参数
        std::string semester = req.get_header_value("X-Query-Semester");
        
        // 解析字段选择参数
        std::vector<std::string> fields = parseFieldsParam(req);

        // 检查课程是否存在
        auto courses = dataManager->getCourses();
        auto courseIt = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.id == courseId; });
        
        if (courseIt == courses.end()) {
            return errorResponse("NotFound", "Course not found", 404);
        }

        auto grades = dataManager->getGrades();
        auto students = dataManager->getStudents();
        
        // 筛选
        std::vector<json> filtered;
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
                    filtered.push_back(item);
                }
            }
        }

        // 分页
        int total = filtered.size();
        int start = (page - 1) * limit;
        int end = std::min(start + limit, total);
        
        json result = json::array();
        if (start < total) {
            for (int i = start; i < end; i++) {
                json item = filtered[i];
                
                // 如果指定了fields，进行字段过滤
                if (!fields.empty()) {
                    json filteredItem;
                    for (const auto& field : fields) {
                        if (item.contains(field)) {
                            filteredItem[field] = item[field];
                        }
                    }
                    result.push_back(filteredItem);
                } else {
                    result.push_back(item);
                }
            }
        }
        
        // 包装成分页格式
        json response = {
            {"data", result},
            {"total", total},
            {"page", page},
            {"limit", limit},
            {"totalPages", (total + limit - 1) / limit}
        };

        // 记录日志（包含分页参数）
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            std::string logMsg = "GET /grades/course/" + courseId + " | page=" + std::to_string(page) + 
                               ", limit=" + std::to_string(limit) + 
                               ", total=" + std::to_string(total);
            if (!fields.empty()) {
                logMsg += ", fields=" + std::to_string(fields.size());
            }
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               logMsg, "成绩管理");
        }

        return jsonResponse(response);
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
        
        json successItems = json::array();
        json failedItems = json::array();

        for (size_t i = 0; i < gradesArray.size(); ++i) {
            const auto& gradeData = gradesArray[i];
            json errorDetails = json::object();
            errorDetails["index"] = i;
            
            try {
                // 验证必填字段
                if (!gradeData.contains("studentId") || gradeData["studentId"].is_null()) {
                    errorDetails["error"] = "Missing required field: studentId";
                    failedItems.push_back(errorDetails);
                    continue;
                }
                if (!gradeData.contains("score") || gradeData["score"].is_null()) {
                    errorDetails["error"] = "Missing required field: score";
                    failedItems.push_back(errorDetails);
                    continue;
                }

                std::string studentId = gradeData["studentId"];
                int score = gradeData["score"];

                // 验证成绩范围
                if (!validateScore(score)) {
                    errorDetails["error"] = "Score must be between 0 and 100: " + std::to_string(score);
                    failedItems.push_back(errorDetails);
                    continue;
                }

                // 验证学生是否存在
                auto studentIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == studentId; });
                if (studentIt == students.end()) {
                    errorDetails["error"] = "Student not found: " + studentId;
                    failedItems.push_back(errorDetails);
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
                
                // 记录成功项
                json successItem = json::object();
                successItem["index"] = i;
                successItem["studentId"] = studentId;
                successItem["score"] = score;
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
            dataManager->saveGrades(existingGrades);
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            std::string logMsg = "POST /grades/batch-update | total=" + std::to_string(gradesArray.size()) +
                               ", success=" + std::to_string(successItems.size()) +
                               ", failed=" + std::to_string(failedItems.size());
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               logMsg, "成绩管理");
        }

        json response = {
            {"success", successItems.size()},
            {"failed", failedItems.size()},
            {"successItems", successItems},
            {"failedItems", failedItems}
        };
        
        // 如果有失败项，返回207状态码（部分成功）
        if (failedItems.size() > 0 && successItems.size() == 0) {
            return jsonResponse(response, 400);
        } else if (failedItems.size() > 0) {
            return jsonResponse(response, 207);
        }
        return jsonResponse(response, 201);
    }

    // 批量导入成绩（支持两种格式）
    crow::response batchImportGrades(const crow::request& req) {
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

        // 支持两种格式：直接数组或 {grades: [...]}
        json gradesArray;
        if (body.is_array()) {
            gradesArray = body;
        } else if (body.is_object() && body.contains("grades") && body["grades"].is_array()) {
            gradesArray = body["grades"];
        } else {
            return errorResponse("BadRequest", "Expected array of grades or {grades: [...]}", 400);
        }

        auto students = dataManager->getStudents();
        auto courses = dataManager->getCourses();
        auto existingGrades = dataManager->getGrades();
        
        json successItems = json::array();
        json failedItems = json::array();

        for (size_t i = 0; i < gradesArray.size(); ++i) {
            const auto& gradeData = gradesArray[i];
            json errorDetails = json::object();
            errorDetails["index"] = i;
            
            try {
                // 验证必填字段
                if (!gradeData.contains("studentId") || gradeData["studentId"].is_null()) {
                    errorDetails["error"] = "Missing required field: studentId";
                    failedItems.push_back(errorDetails);
                    continue;
                }
                if (!gradeData.contains("courseId") || gradeData["courseId"].is_null()) {
                    errorDetails["error"] = "Missing required field: courseId";
                    failedItems.push_back(errorDetails);
                    continue;
                }
                if (!gradeData.contains("score") || gradeData["score"].is_null()) {
                    errorDetails["error"] = "Missing required field: score";
                    failedItems.push_back(errorDetails);
                    continue;
                }
                if (!gradeData.contains("semester") || gradeData["semester"].is_null()) {
                    errorDetails["error"] = "Missing required field: semester";
                    failedItems.push_back(errorDetails);
                    continue;
                }

                std::string studentId = gradeData["studentId"];
                std::string courseId = gradeData["courseId"];
                int score = gradeData["score"];
                std::string semester = gradeData["semester"];

                // 验证成绩范围
                if (!validateScore(score)) {
                    errorDetails["error"] = "Score must be between 0 and 100: " + std::to_string(score);
                    failedItems.push_back(errorDetails);
                    continue;
                }

                // 验证学生存在
                auto studentIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == studentId; });
                if (studentIt == students.end()) {
                    errorDetails["error"] = "Student not found: " + studentId;
                    failedItems.push_back(errorDetails);
                    continue;
                }

                // 验证课程存在
                auto courseIt = std::find_if(courses.begin(), courses.end(),
                    [&](const Course& c) { return c.courseId == courseId; });
                if (courseIt == courses.end()) {
                    errorDetails["error"] = "Course not found: " + courseId;
                    failedItems.push_back(errorDetails);
                    continue;
                }

                // 检查重复
                auto gradeIt = std::find_if(existingGrades.begin(), existingGrades.end(),
                    [&](const Grade& g) { 
                        return g.studentId == studentId && g.courseId == courseId && 
                               g.semester == semester;
                    });
                if (gradeIt != existingGrades.end()) {
                    errorDetails["error"] = "Grade already exists for student " + studentId + " in course " + courseId;
                    failedItems.push_back(errorDetails);
                    continue;
                }

                // 创建成绩
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
                
                // 记录成功项
                json successItem = json::object();
                successItem["index"] = i;
                successItem["studentId"] = studentId;
                successItem["courseId"] = courseId;
                successItem["score"] = score;
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
            dataManager->saveGrades(existingGrades);
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            std::string logMsg = "POST /grades/batch | total=" + std::to_string(gradesArray.size()) +
                               ", success=" + std::to_string(successItems.size()) +
                               ", failed=" + std::to_string(failedItems.size());
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               logMsg, "成绩管理");
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

        // 获取过滤参数
        std::string studentId = req.get_header_value("X-Query-StudentId");
        std::string courseId = req.get_header_value("X-Query-CourseId");
        std::string classFilter = req.get_header_value("X-Query-Class");
        std::string semester = req.get_header_value("X-Query-Semester");

        auto grades = dataManager->getGrades();
        auto students = dataManager->getStudents();
        
        // 筛选
        std::vector<Grade> filtered;
        for (const auto& grade : grades) {
            if (!studentId.empty() && grade.studentId != studentId) continue;
            if (!courseId.empty() && grade.courseId != courseId) continue;
            if (!semester.empty() && grade.semester.value_or("") != semester) continue;
            
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
            json item;
            to_json_iso(item, grade, [this](const std::string& ts) { 
                return dataManager->convertToISO8601(ts);
            });
            result.push_back(item);
        }

        return jsonResponse(result);
    }
};

#endif // GRADE_SERVICE_H