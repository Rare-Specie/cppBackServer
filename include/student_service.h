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

        // 解析分页参数（支持字符串和整数）
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        
        // 获取过滤参数
        std::string classFilter = req.get_header_value("X-Query-Class");
        std::string search = req.get_header_value("X-Query-Search");
        
        // 解析字段选择参数
        bool fullData = requestFullData(req);
        std::vector<std::string> fields = parseFieldsParam(req);
        
        // 支持URL查询参数（Crow框架中通过url_params）
        // 注意：这里简化处理，实际应该使用crow::request::url_params
        // 由于Crow框架的限制，我们通过header传递参数，但代码结构支持扩展到URL参数

        auto students = dataManager->getStudents();
        
        // 筛选（先过滤再分页）
        std::vector<Student> filtered;
        for (const auto& student : students) {
            if (!classFilter.empty() && student.className != classFilter) continue;
            if (!search.empty()) {
                if (student.studentId.find(search) == std::string::npos &&
                    student.name.find(search) == std::string::npos) continue;
            }
            filtered.push_back(student);
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
            std::string logMsg = "GET /students | page=" + std::to_string(page) + 
                               ", limit=" + std::to_string(limit) + 
                               ", filtered=" + std::to_string(filtered.size());
            if (!fields.empty()) {
                logMsg += ", fields=" + std::to_string(fields.size());
            }
            if (fullData) {
                logMsg += ", full=true";
            }
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               logMsg, "学生管理");
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

    // 批量导入学生（支持两种格式：数组和{students: [...]})
    crow::response batchImportStudents(const crow::request& req) {
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

        // 支持两种格式：直接数组或 {students: [...]}
        json studentsArray;
        if (body.is_array()) {
            studentsArray = body;
        } else if (body.is_object() && body.contains("students") && body["students"].is_array()) {
            studentsArray = body["students"];
        } else {
            return errorResponse("BadRequest", "Expected array of students or {students: [...]}", 400);
        }

        auto existingStudents = dataManager->getStudents();
        json successItems = json::array();
        json failedItems = json::array();

        for (size_t i = 0; i < studentsArray.size(); ++i) {
            const auto& studentData = studentsArray[i];
            json errorDetails = json::object();
            errorDetails["index"] = i;
            
            try {
                // 验证必填字段
                if (!studentData.contains("studentId") || studentData["studentId"].is_null()) {
                    errorDetails["error"] = "Missing required field: studentId";
                    failedItems.push_back(errorDetails);
                    continue;
                }
                if (!studentData.contains("name") || studentData["name"].is_null()) {
                    errorDetails["error"] = "Missing required field: name";
                    failedItems.push_back(errorDetails);
                    continue;
                }
                if (!studentData.contains("class") || studentData["class"].is_null()) {
                    errorDetails["error"] = "Missing required field: class";
                    failedItems.push_back(errorDetails);
                    continue;
                }

                std::string studentId = studentData["studentId"];
                std::string name = studentData["name"];
                std::string className = studentData["class"];

                // 检查重复
                auto it = std::find_if(existingStudents.begin(), existingStudents.end(),
                    [&](const Student& s) { return s.studentId == studentId; });
                if (it != existingStudents.end()) {
                    errorDetails["error"] = "Student ID already exists: " + studentId;
                    failedItems.push_back(errorDetails);
                    continue;
                }

                // 验证可选字段格式
                if (studentData.contains("phone") && !studentData["phone"].is_null()) {
                    std::string phone = studentData["phone"];
                    if (!validatePhone(phone)) {
                        errorDetails["error"] = "Invalid phone format: " + phone;
                        failedItems.push_back(errorDetails);
                        continue;
                    }
                }
                if (studentData.contains("email") && !studentData["email"].is_null()) {
                    std::string email = studentData["email"];
                    if (!validateEmail(email)) {
                        errorDetails["error"] = "Invalid email format: " + email;
                        failedItems.push_back(errorDetails);
                        continue;
                    }
                }

                // 创建学生
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
                existingStudents.push_back(newStudent);
                
                // 记录成功项
                json successItem = json::object();
                successItem["index"] = i;
                successItem["studentId"] = studentId;
                successItem["name"] = name;
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
            dataManager->saveStudents(existingStudents);
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            std::string logMsg = "POST /students/batch | total=" + std::to_string(studentsArray.size()) +
                               ", success=" + std::to_string(successItems.size()) +
                               ", failed=" + std::to_string(failedItems.size());
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               logMsg, "学生管理");
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