#ifndef COURSE_SERVICE_H
#define COURSE_SERVICE_H

#include <string>
#include <vector>
#include <optional>
#include <crow.h>
#include "models.h"
#include "data_manager.h"
#include "auth.h"
#include "middleware.h"

class CourseService {
private:
    DataManager* dataManager;
    AuthManager* authManager;
    LogMiddleware* logger;

public:
    CourseService(DataManager* dm, AuthManager* am, LogMiddleware* log) 
        : dataManager(dm), authManager(am), logger(log) {}

    // 获取课程列表
    crow::response getCourses(const crow::request& req) {
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
        std::string search = req.get_header_value("X-Query-Search");

        auto courses = dataManager->getCourses();
        
        // 筛选（先过滤再分页）
        std::vector<Course> filtered;
        for (const auto& course : courses) {
            if (!search.empty()) {
                if (course.courseId.find(search) == std::string::npos &&
                    course.name.find(search) == std::string::npos) continue;
            }
            filtered.push_back(course);
        }

        // 分页（使用ISO日期格式）
        auto result = paginateWithISO(filtered, page, limit, 
            [this](const std::string& ts) { return dataManager->convertToISO8601(ts); });
        
        // 记录日志（包含分页参数）
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            std::string logMsg = "GET /courses | page=" + std::to_string(page) + 
                               ", limit=" + std::to_string(limit) + 
                               ", filtered=" + std::to_string(filtered.size());
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               logMsg, "课程管理");
        }

        return jsonResponse(result);
    }

    // 获取课程详情
    crow::response getCourse(const crow::request& req, const std::string& id) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        auto courses = dataManager->getCourses();
        auto it = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.id == id; });
        
        if (it == courses.end()) {
            return errorResponse("NotFound", "Course not found", 404);
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /courses/" + id, "课程管理");
        }

        return jsonResponse(*it);
    }

    // 添加课程
    crow::response createCourse(const crow::request& req) {
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
        if (!body.contains("courseId") || !body.contains("name") || !body.contains("credit")) {
            return errorResponse("BadRequest", "Missing required fields", 400);
        }

        std::string courseId = body["courseId"];
        std::string name = body["name"];
        int credit = body["credit"];
        
        std::optional<std::string> teacher;
        std::optional<std::string> description;

        if (body.contains("teacher") && !body["teacher"].is_null()) {
            teacher = body["teacher"];
        }
        if (body.contains("description") && !body["description"].is_null()) {
            description = body["description"];
        }

        // 检查课程编号是否已存在
        auto courses = dataManager->getCourses();
        auto it = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.courseId == courseId; });
        if (it != courses.end()) {
            return errorResponse("Conflict", "Course ID already exists", 409);
        }

        // 创建课程
        Course newCourse{
            dataManager->generateId(),
            courseId,
            name,
            credit,
            teacher,
            description,
            dataManager->getCurrentTimestamp(),
            dataManager->getCurrentTimestamp()
        };
        courses.push_back(newCourse);
        dataManager->saveCourses(courses);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /courses", "课程管理");
        }

        return jsonResponse(newCourse, 201);
    }

    // 更新课程
    crow::response updateCourse(const crow::request& req, const std::string& id) {
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

        auto courses = dataManager->getCourses();
        auto it = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.id == id; });
        
        if (it == courses.end()) {
            return errorResponse("NotFound", "Course not found", 404);
        }

        // 更新字段
        if (body.contains("name") && !body["name"].is_null()) {
            it->name = body["name"];
        }
        if (body.contains("credit") && !body["credit"].is_null()) {
            it->credit = body["credit"];
        }
        if (body.contains("teacher") && !body["teacher"].is_null()) {
            it->teacher = body["teacher"];
        }
        if (body.contains("description") && !body["description"].is_null()) {
            it->description = body["description"];
        }

        it->updatedAt = dataManager->getCurrentTimestamp();
        dataManager->saveCourses(courses);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "PUT /courses/" + id, "课程管理");
        }

        return jsonResponse(*it);
    }

    // 删除课程
    crow::response deleteCourse(const crow::request& req, const std::string& id) {
        // 验证权限（管理员）
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->hasPermission(token.substr(7), {"admin"})) {
            return errorResponse("Forbidden", "Admin only", 403);
        }

        auto courses = dataManager->getCourses();
        auto it = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.id == id; });
        
        if (it == courses.end()) {
            return errorResponse("NotFound", "Course not found", 404);
        }

        courses.erase(it);
        dataManager->saveCourses(courses);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "DELETE /courses/" + id, "课程管理");
        }

        return jsonResponse(std::string("Course deleted successfully"));
    }

    // 获取选课学生列表
    crow::response getCourseStudents(const crow::request& req, const std::string& courseId) {
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
        
        // 解析字段选择参数
        std::vector<std::string> fields = parseFieldsParam(req);

        // 检查课程是否存在
        auto courses = dataManager->getCourses();
        auto courseIt = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.id == courseId; });
        
        if (courseIt == courses.end()) {
            return errorResponse("NotFound", "Course not found", 404);
        }

        // 获取成绩数据，找出选修该课程的学生
        auto grades = dataManager->getGrades();
        auto students = dataManager->getStudents();
        
        std::vector<json> courseStudents;
        std::vector<std::string> processedStudentIds;

        for (const auto& grade : grades) {
            if (grade.courseId == courseId) {
                // 避免重复
                if (std::find(processedStudentIds.begin(), processedStudentIds.end(), grade.studentId) 
                    == processedStudentIds.end()) {
                    
                    // 查找学生信息
                    auto studentIt = std::find_if(students.begin(), students.end(),
                        [&](const Student& s) { return s.studentId == grade.studentId; });
                    
                    if (studentIt != students.end()) {
                        json studentInfo = {
                            {"studentId", grade.studentId},
                            {"name", studentIt->name},
                            {"class", studentIt->className},
                            {"score", grade.score}
                        };
                        courseStudents.push_back(studentInfo);
                        processedStudentIds.push_back(grade.studentId);
                    }
                }
            }
        }

        // 分页
        int total = courseStudents.size();
        int start = (page - 1) * limit;
        int end = std::min(start + limit, total);
        
        json result = json::array();
        if (start < total) {
            for (int i = start; i < end; i++) {
                json item = courseStudents[i];
                
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
            std::string logMsg = "GET /courses/" + courseId + "/students | page=" + std::to_string(page) + 
                               ", limit=" + std::to_string(limit) + 
                               ", total=" + std::to_string(total);
            if (!fields.empty()) {
                logMsg += ", fields=" + std::to_string(fields.size());
            }
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               logMsg, "课程管理");
        }

        return jsonResponse(response);
    }

    // 学生选课
    crow::response enrollStudent(const crow::request& req, const std::string& courseId) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        // 验证权限（管理员和教师可以选课）
        if (!authManager->hasPermission(token.substr(7), {"admin", "teacher"})) {
            return errorResponse("Forbidden", "Admin or Teacher only", 403);
        }

        // 解析请求体
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        if (!body.contains("studentId")) {
            return errorResponse("BadRequest", "Missing studentId", 400);
        }

        std::string studentId = body["studentId"];

        // 检查课程是否存在
        auto courses = dataManager->getCourses();
        auto courseIt = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.id == courseId; });
        
        if (courseIt == courses.end()) {
            return errorResponse("NotFound", "Course not found", 404);
        }

        // 检查学生是否存在
        auto students = dataManager->getStudents();
        auto studentIt = std::find_if(students.begin(), students.end(),
            [&](const Student& s) { return s.studentId == studentId; });
        
        if (studentIt == students.end()) {
            return errorResponse("NotFound", "Student not found", 404);
        }

        // 检查是否已经选课（通过成绩记录判断）
        auto grades = dataManager->getGrades();
        auto gradeIt = std::find_if(grades.begin(), grades.end(),
            [&](const Grade& g) { 
                return g.courseId == courseId && g.studentId == studentId; 
            });
        
        if (gradeIt != grades.end()) {
            return errorResponse("Conflict", "Student already enrolled in this course", 409);
        }

        // 创建选课记录（初始成绩为0，等待录入）
        Grade newGrade{
            dataManager->generateId(),
            studentId,
            studentIt->name,
            courseId,
            courseIt->name,
            0,  // 初始成绩为0
            std::nullopt,  // 可选的学期字段
            dataManager->getCurrentTimestamp(),
            dataManager->getCurrentTimestamp()
        };
        grades.push_back(newGrade);
        dataManager->saveGrades(grades);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /courses/" + courseId + "/enroll", "课程管理");
        }

        json response = {
            {"message", "Enrollment successful"},
            {"student", {
                {"studentId", studentId},
                {"name", studentIt->name},
                {"class", studentIt->className}
            }},
            {"course", {
                {"courseId", courseId},
                {"name", courseIt->name}
            }}
        };

        return jsonResponse(response, 201);
    }

    // 取消选课
    crow::response unenrollStudent(const crow::request& req, const std::string& courseId, const std::string& studentId) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        // 验证权限（管理员和教师可以取消选课）
        if (!authManager->hasPermission(token.substr(7), {"admin", "teacher"})) {
            return errorResponse("Forbidden", "Admin or Teacher only", 403);
        }

        // 检查课程是否存在
        auto courses = dataManager->getCourses();
        auto courseIt = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.id == courseId; });
        
        if (courseIt == courses.end()) {
            return errorResponse("NotFound", "Course not found", 404);
        }

        // 检查学生是否存在
        auto students = dataManager->getStudents();
        auto studentIt = std::find_if(students.begin(), students.end(),
            [&](const Student& s) { return s.studentId == studentId; });
        
        if (studentIt == students.end()) {
            return errorResponse("NotFound", "Student not found", 404);
        }

        // 查找选课记录
        auto grades = dataManager->getGrades();
        auto gradeIt = std::find_if(grades.begin(), grades.end(),
            [&](const Grade& g) { 
                return g.courseId == courseId && g.studentId == studentId; 
            });
        
        if (gradeIt == grades.end()) {
            return errorResponse("NotFound", "Enrollment not found", 404);
        }

        // 删除选课记录
        grades.erase(gradeIt);
        dataManager->saveGrades(grades);

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "DELETE /courses/" + courseId + "/enroll/" + studentId, "课程管理");
        }

        json response = {
            {"message", "Unenrollment successful"},
            {"student", {
                {"studentId", studentId},
                {"name", studentIt->name}
            }},
            {"course", {
                {"courseId", courseId},
                {"name", courseIt->name}
            }}
        };

        return jsonResponse(response);
    }
};

#endif // COURSE_SERVICE_H