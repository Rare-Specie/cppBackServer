#ifndef REPORT_SERVICE_H
#define REPORT_SERVICE_H

#include <string>
#include <vector>
#include <optional>
#include <crow.h>
#include "models.h"
#include "data_manager.h"
#include "auth.h"
#include "middleware.h"

class ReportService {
private:
    DataManager* dataManager;
    AuthManager* authManager;
    LogMiddleware* logger;

public:
    ReportService(DataManager* dm, AuthManager* am, LogMiddleware* log) 
        : dataManager(dm), authManager(am), logger(log) {}

    // 生成成绩单（简化处理，返回HTML）
    crow::response generateReportCard(const crow::request& req) {
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
        std::string classFilter = "";
        std::string semester = "";

        auto students = dataManager->getStudents();
        auto grades = dataManager->getGrades();
        auto courses = dataManager->getCourses();

        std::vector<json> reportData;

        if (!studentId.empty()) {
            // 单个学生成绩单
            auto studentIt = std::find_if(students.begin(), students.end(),
                [&](const Student& s) { return s.studentId == studentId; });
            
            if (studentIt == students.end()) {
                return errorResponse("NotFound", "Student not found", 404);
            }

            std::vector<Grade> studentGrades;
            for (const auto& grade : grades) {
                if (grade.studentId == studentId) {
                    if (!semester.empty() && grade.semester != semester) continue;
                    studentGrades.push_back(grade);
                }
            }

            json studentReport = {
                {"studentId", studentId},
                {"studentName", studentIt->name},
                {"className", studentIt->className},
                {"grades", json::array()}
            };

            for (const auto& grade : studentGrades) {
                studentReport["grades"].push_back({
                    {"courseId", grade.courseId},
                    {"courseName", grade.courseName},
                    {"score", grade.score},
                    {"semester", grade.semester.value_or("")}
                });
            }

            reportData.push_back(studentReport);
        } else if (!classFilter.empty()) {
            // 班级成绩单
            for (const auto& student : students) {
                if (student.className == classFilter) {
                    std::vector<Grade> studentGrades;
                    for (const auto& grade : grades) {
                        if (grade.studentId == student.studentId) {
                            if (!semester.empty() && grade.semester != semester) continue;
                            studentGrades.push_back(grade);
                        }
                    }

                    json studentReport = {
                        {"studentId", student.studentId},
                        {"studentName", student.name},
                        {"className", student.className},
                        {"grades", json::array()}
                    };

                    for (const auto& grade : studentGrades) {
                        studentReport["grades"].push_back({
                            {"courseId", grade.courseId},
                            {"courseName", grade.courseName},
                            {"score", grade.score},
                            {"semester", grade.semester.value_or("")}
                        });
                    }

                    reportData.push_back(studentReport);
                }
            }
        } else {
            return errorResponse("BadRequest", "studentId or class parameter is required", 400);
        }

        // 生成HTML（简化处理）
        std::string html = "<html><head><style>";
        html += "body { font-family: Arial, sans-serif; margin: 20px; }";
        html += "table { border-collapse: collapse; width: 100%; margin-top: 20px; }";
        html += "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }";
        html += "th { background-color: #4CAF50; color: white; }";
        html += "h1 { color: #333; }";
        html += ".student-info { margin-bottom: 20px; padding: 10px; background-color: #f5f5f5; }";
        html += "</style></head><body>";
        html += "<h1>学生成绩单</h1>";

        for (const auto& student : reportData) {
            html += "<div class='student-info'>";
            html += "<strong>学号:</strong> " + student["studentId"].get<std::string>() + "<br>";
            html += "<strong>姓名:</strong> " + student["studentName"].get<std::string>() + "<br>";
            html += "<strong>班级:</strong> " + student["className"].get<std::string>();
            html += "</div>";

            html += "<table><tr><th>课程编号</th><th>课程名称</th><th>成绩</th><th>学期</th></tr>";
            for (const auto& grade : student["grades"]) {
                html += "<tr>";
                html += "<td>" + grade["courseId"].get<std::string>() + "</td>";
                html += "<td>" + grade["courseName"].get<std::string>() + "</td>";
                html += "<td>" + std::to_string(grade["score"].get<int>()) + "</td>";
                html += "<td>" + grade["semester"].get<std::string>() + "</td>";
                html += "</tr>";
            }
            html += "</table><br>";
        }

        html += "</body></html>";

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /reports/report-card", "报表管理");
        }

        // 返回HTML
        crow::response res(200);
        res.set_header("Content-Type", "text/html");
        res.body = html;
        return res;
    }

    // 生成统计报表（调用统计服务）
    crow::response generateStatisticsReport(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 获取查询参数
        std::string type = req.get_header_value("X-Query-Type");
        std::string format = req.get_header_value("X-Query-Format");

        if (type.empty() || format.empty()) {
            return errorResponse("BadRequest", "type and format are required", 400);
        }

        // 这里简化处理，实际应该调用统计服务并生成文件
        // 返回JSON作为演示
        json result = {
            {"message", "统计报表生成成功"},
            {"type", type},
            {"format", format},
            {"note", "实际实现应生成PDF或Excel文件"}
        };

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /reports/statistics", "报表管理");
        }

        return jsonResponse(result);
    }

    // 打印准备
    crow::response printPrepare(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 解析请求体
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        if (!body.contains("type") || !body.contains("data")) {
            return errorResponse("BadRequest", "Missing type or data", 400);
        }

        std::string type = body["type"];
        json data = body["data"];

        // 生成HTML
        std::string html = "<html><head><style>";
        html += "body { font-family: Arial, sans-serif; margin: 20px; }";
        html += "table { border-collapse: collapse; width: 100%; }";
        html += "th, td { border: 1px solid #ddd; padding: 8px; }";
        html += "th { background-color: #4CAF50; color: white; }";
        html += "</style></head><body>";

        if (type == "report-card") {
            html += "<h1>成绩单</h1>";
            html += "<p>学生: " + data["studentName"].get<std::string>() + "</p>";
            html += "<table><tr><th>课程</th><th>成绩</th></tr>";
            for (const auto& grade : data["grades"]) {
                html += "<tr><td>" + grade["courseName"].get<std::string>() + "</td>";
                html += "<td>" + std::to_string(grade["score"].get<int>()) + "</td></tr>";
            }
            html += "</table>";
        } else if (type == "statistical") {
            html += "<h1>统计报表</h1>";
            html += "<pre>" + data.dump(2) + "</pre>";
        }

        html += "</body></html>";

        json result = {
            {"html", html}
        };

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /reports/print", "报表管理");
        }

        return jsonResponse(result);
    }

    // 批量打印
    crow::response batchPrint(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 解析请求体
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        if (!body.contains("type") || !body.contains("items")) {
            return errorResponse("BadRequest", "Missing type or items", 400);
        }

        std::string type = body["type"];
        auto items = body["items"];

        int success = 0;
        int failed = 0;

        for (const auto& item : items) {
            try {
                // 这里简化处理，实际应该生成对应的打印数据
                success++;
            } catch (...) {
                failed++;
            }
        }

        json result = {
            {"success", success},
            {"failed", failed}
        };

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "POST /reports/batch-print", "报表管理");
        }

        return jsonResponse(result);
    }
};

#endif // REPORT_SERVICE_H