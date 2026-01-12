#ifndef STATISTICS_SERVICE_H
#define STATISTICS_SERVICE_H

#include <string>
#include <vector>
#include <optional>
#include <crow.h>
#include <algorithm>
#include <numeric>
#include "models.h"
#include "data_manager.h"
#include "auth.h"
#include "middleware.h"

class StatisticsService {
private:
    DataManager* dataManager;
    AuthManager* authManager;
    LogMiddleware* logger;

public:
    StatisticsService(DataManager* dm, AuthManager* am, LogMiddleware* log) 
        : dataManager(dm), authManager(am), logger(log) {}

    // 获取统计概览
    crow::response getOverview(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        auto students = dataManager->getStudents();
        auto courses = dataManager->getCourses();
        auto grades = dataManager->getGrades();

        // 计算统计信息
        double avgScore = 0.0;
        double passRate = 0.0;
        int totalGrades = grades.size();

        if (totalGrades > 0) {
            int totalScore = 0;
            int passCount = 0;
            for (const auto& grade : grades) {
                totalScore += grade.score;
                if (grade.score >= 60) passCount++;
            }
            avgScore = static_cast<double>(totalScore) / totalGrades;
            passRate = (static_cast<double>(passCount) / totalGrades) * 100.0;
        }

        json result = {
            {"avgScore", avgScore},
            {"passRate", passRate},
            {"totalStudents", static_cast<int>(students.size())},
            {"totalCourses", static_cast<int>(courses.size())},
            {"totalGrades", totalGrades}
        };

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /statistics/overview", "统计分析");
        }

        return jsonResponse(result);
    }

    // 按班级统计
    crow::response getClassStatistics(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 获取查询参数
        std::string classFilter = "";
        std::string courseId = "";

        auto students = dataManager->getStudents();
        auto grades = dataManager->getGrades();

        // 获取所有班级
        std::vector<std::string> classes;
        for (const auto& student : students) {
            if (std::find(classes.begin(), classes.end(), student.className) == classes.end()) {
                classes.push_back(student.className);
            }
        }

        // 筛选特定班级
        if (!classFilter.empty()) {
            classes = {classFilter};
        }

        json result = json::array();

        for (const auto& className : classes) {
            // 获取该班级所有学生
            std::vector<std::string> studentIds;
            for (const auto& student : students) {
                if (student.className == className) {
                    studentIds.push_back(student.studentId);
                }
            }

            // 获取该班级所有成绩
            std::vector<Grade> classGrades;
            for (const auto& grade : grades) {
                if (std::find(studentIds.begin(), studentIds.end(), grade.studentId) != studentIds.end()) {
                    if (courseId.empty() || grade.courseId == courseId) {
                        classGrades.push_back(grade);
                    }
                }
            }

            if (classGrades.empty()) continue;

            // 计算统计
            int totalScore = 0;
            int passCount = 0;
            for (const auto& grade : classGrades) {
                totalScore += grade.score;
                if (grade.score >= 60) passCount++;
            }

            double avgScore = static_cast<double>(totalScore) / classGrades.size();
            double passRate = (static_cast<double>(passCount) / classGrades.size()) * 100.0;

            // 获取前3名
            std::vector<std::pair<std::string, int>> studentScores;
            for (const auto& grade : classGrades) {
                studentScores.push_back({grade.studentId, grade.score});
            }
            
            // 按成绩排序
            std::sort(studentScores.begin(), studentScores.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });

            // 去重，只保留每个学生最高分
            std::vector<std::pair<std::string, int>> topScores;
            std::vector<std::string> processed;
            for (const auto& score : studentScores) {
                if (std::find(processed.begin(), processed.end(), score.first) == processed.end()) {
                    topScores.push_back(score);
                    processed.push_back(score.first);
                    if (topScores.size() >= 3) break;
                }
            }

            // 构建topStudents
            json topStudents = json::array();
            for (const auto& score : topScores) {
                auto studentIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == score.first; });
                if (studentIt != students.end()) {
                    topStudents.push_back({
                        {"studentId", score.first},
                        {"name", studentIt->name},
                        {"score", score.second}
                    });
                }
            }

            result.push_back({
                {"class", className},
                {"avgScore", avgScore},
                {"passRate", passRate},
                {"totalStudents", static_cast<int>(studentIds.size())},
                {"topStudents", topStudents}
            });
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /statistics/class", "统计分析");
        }

        return jsonResponse(result);
    }

    // 按课程统计
    crow::response getCourseStatistics(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 获取查询参数
        std::string courseId = "";

        if (courseId.empty()) {
            return errorResponse("BadRequest", "courseId is required", 400);
        }

        // 检查课程是否存在
        auto courses = dataManager->getCourses();
        auto courseIt = std::find_if(courses.begin(), courses.end(),
            [&](const Course& c) { return c.courseId == courseId; });
        
        if (courseIt == courses.end()) {
            return errorResponse("NotFound", "Course not found", 404);
        }

        auto grades = dataManager->getGrades();
        auto students = dataManager->getStudents();

        // 获取该课程所有成绩
        std::vector<Grade> courseGrades;
        for (const auto& grade : grades) {
            if (grade.courseId == courseId) {
                courseGrades.push_back(grade);
            }
        }

        if (courseGrades.empty()) {
            json result = {
                {"courseId", courseId},
                {"courseName", courseIt->name},
                {"avgScore", 0.0},
                {"passRate", 0.0},
                {"totalStudents", 0},
                {"highestScore", 0},
                {"lowestScore", 0}
            };
            return jsonResponse(result);
        }

        // 计算统计
        int totalScore = 0;
        int passCount = 0;
        int highestScore = 0;
        int lowestScore = 100;

        for (const auto& grade : courseGrades) {
            totalScore += grade.score;
            if (grade.score >= 60) passCount++;
            if (grade.score > highestScore) highestScore = grade.score;
            if (grade.score < lowestScore) lowestScore = grade.score;
        }

        double avgScore = static_cast<double>(totalScore) / courseGrades.size();
        double passRate = (static_cast<double>(passCount) / courseGrades.size()) * 100.0;

        // 统计去重后的学生数量
        std::vector<std::string> studentIds;
        for (const auto& grade : courseGrades) {
            if (std::find(studentIds.begin(), studentIds.end(), grade.studentId) == studentIds.end()) {
                studentIds.push_back(grade.studentId);
            }
        }

        json result = {
            {"courseId", courseId},
            {"courseName", courseIt->name},
            {"avgScore", avgScore},
            {"passRate", passRate},
            {"totalStudents", static_cast<int>(studentIds.size())},
            {"highestScore", highestScore},
            {"lowestScore", lowestScore}
        };

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /statistics/course", "统计分析");
        }

        return jsonResponse(result);
    }

    // 获取排名列表
    crow::response getRanking(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 获取查询参数
        std::string classFilter = "";
        std::string courseId = "";
        int limit = 10;

        auto students = dataManager->getStudents();
        auto grades = dataManager->getGrades();

        // 计算每个学生的总成绩和平均分
        std::map<std::string, std::vector<int>> studentScores;
        
        for (const auto& grade : grades) {
            // 筛选课程
            if (!courseId.empty() && grade.courseId != courseId) continue;
            
            // 筛选班级
            if (!classFilter.empty()) {
                auto studentIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == grade.studentId; });
                if (studentIt == students.end() || studentIt->className != classFilter) continue;
            }

            studentScores[grade.studentId].push_back(grade.score);
        }

        // 计算排名数据
        struct RankData {
            std::string studentId;
            std::string name;
            std::string className;
            double avgScore;
            int totalScore;
            int courseCount;
        };

        std::vector<RankData> rankings;

        for (const auto& [studentId, scores] : studentScores) {
            if (scores.empty()) continue;

            int totalScore = std::accumulate(scores.begin(), scores.end(), 0);
            double avgScore = static_cast<double>(totalScore) / scores.size();

            // 查找学生信息
            auto studentIt = std::find_if(students.begin(), students.end(),
                [&](const Student& s) { return s.studentId == studentId; });
            
            if (studentIt != students.end()) {
                rankings.push_back({
                    studentId,
                    studentIt->name,
                    studentIt->className,
                    avgScore,
                    totalScore,
                    static_cast<int>(scores.size())
                });
            }
        }

        // 按平均分排序
        std::sort(rankings.begin(), rankings.end(),
            [](const RankData& a, const RankData& b) { return a.avgScore > b.avgScore; });

        // 限制数量
        if (rankings.size() > limit) {
            rankings.resize(limit);
        }

        // 构建结果
        json result = json::array();
        for (size_t i = 0; i < rankings.size(); i++) {
            result.push_back({
                {"rank", static_cast<int>(i + 1)},
                {"studentId", rankings[i].studentId},
                {"name", rankings[i].name},
                {"class", rankings[i].className},
                {"totalScore", rankings[i].totalScore},
                {"avgScore", rankings[i].avgScore},
                {"courseCount", rankings[i].courseCount}
            });
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /statistics/ranking", "统计分析");
        }

        return jsonResponse(result);
    }

    // 获取成绩分布
    crow::response getDistribution(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 获取查询参数
        std::string courseId = "";
        std::string classFilter = "";

        auto grades = dataManager->getGrades();
        auto students = dataManager->getStudents();

        // 筛选
        std::vector<Grade> filtered;
        for (const auto& grade : grades) {
            if (!courseId.empty() && grade.courseId != courseId) continue;
            
            if (!classFilter.empty()) {
                auto studentIt = std::find_if(students.begin(), students.end(),
                    [&](const Student& s) { return s.studentId == grade.studentId; });
                if (studentIt == students.end() || studentIt->className != classFilter) continue;
            }

            filtered.push_back(grade);
        }

        // 定义分数段
        struct ScoreRange {
            std::string range;
            int min;
            int max;
        };

        std::vector<ScoreRange> ranges = {
            {"90-100", 90, 100},
            {"80-89", 80, 89},
            {"70-79", 70, 79},
            {"60-69", 60, 69},
            {"0-59", 0, 59}
        };

        int total = filtered.size();
        json result = json::array();

        for (const auto& range : ranges) {
            int count = 0;
            for (const auto& grade : filtered) {
                if (grade.score >= range.min && grade.score <= range.max) {
                    count++;
                }
            }

            double percentage = total > 0 ? (static_cast<double>(count) / total) * 100.0 : 0.0;

            result.push_back({
                {"range", range.range},
                {"count", count},
                {"percentage", percentage}
            });
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /statistics/distribution", "统计分析");
        }

        return jsonResponse(result);
    }

    // 生成统计报表（简化处理，返回JSON）
    crow::response generateReport(const crow::request& req) {
        // 验证Token
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }
        
        if (!authManager->verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        // 获取查询参数
        std::string type = "";
        std::string format = "";
        std::string classFilter = "";
        std::string courseId = "";
        std::string studentId = "";
        std::string semester = "";

        if (type.empty() || format.empty()) {
            return errorResponse("BadRequest", "type and format are required", 400);
        }

        // 根据类型生成不同报表
        json result;

        if (type == "overall") {
            // 总体统计
            auto students = dataManager->getStudents();
            auto courses = dataManager->getCourses();
            auto grades = dataManager->getGrades();

            double avgScore = 0.0;
            double passRate = 0.0;
            int totalGrades = grades.size();

            if (totalGrades > 0) {
                int totalScore = 0;
                int passCount = 0;
                for (const auto& grade : grades) {
                    totalScore += grade.score;
                    if (grade.score >= 60) passCount++;
                }
                avgScore = static_cast<double>(totalScore) / totalGrades;
                passRate = (static_cast<double>(passCount) / totalGrades) * 100.0;
            }

            result = {
                {"type", "overall"},
                {"format", format},
                {"data", {
                    {"avgScore", avgScore},
                    {"passRate", passRate},
                    {"totalStudents", static_cast<int>(students.size())},
                    {"totalCourses", static_cast<int>(courses.size())},
                    {"totalGrades", totalGrades}
                }}
            };
        } else if (type == "class") {
            // 班级统计
            if (classFilter.empty()) {
                return errorResponse("BadRequest", "class parameter is required for class report", 400);
            }
            
            // 调用班级统计方法
            auto response = getClassStatistics(req);
            result = {
                {"type", "class"},
                {"format", format},
                {"data", json::parse(response.body)}
            };
        } else if (type == "course") {
            // 课程统计
            if (courseId.empty()) {
                return errorResponse("BadRequest", "courseId parameter is required for course report", 400);
            }
            
            // 调用课程统计方法
            auto response = getCourseStatistics(req);
            result = {
                {"type", "course"},
                {"format", format},
                {"data", json::parse(response.body)}
            };
        } else if (type == "student") {
            // 学生统计
            if (studentId.empty()) {
                return errorResponse("BadRequest", "studentId parameter is required for student report", 400);
            }
            
            // 获取学生成绩概览
            auto students = dataManager->getStudents();
            auto studentIt = std::find_if(students.begin(), students.end(),
                [&](const Student& s) { return s.studentId == studentId; });
            
            if (studentIt == students.end()) {
                return errorResponse("NotFound", "Student not found", 404);
            }

            auto grades = dataManager->getGrades();
            std::vector<Grade> studentGrades;
            for (const auto& grade : grades) {
                if (grade.studentId == studentId) {
                    studentGrades.push_back(grade);
                }
            }

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

            result = {
                {"type", "student"},
                {"format", format},
                {"data", {
                    {"studentId", studentId},
                    {"studentName", studentIt->name},
                    {"className", studentIt->className},
                    {"totalCourses", totalCourses},
                    {"avgScore", avgScore},
                    {"passRate", passRate},
                    {"totalScore", totalScore}
                }}
            };
        } else {
            return errorResponse("BadRequest", "Invalid type", 400);
        }

        // 记录日志
        auto currentUser = authManager->getCurrentUser(token.substr(7));
        if (currentUser.has_value()) {
            logger->logOperation(currentUser.value().id, currentUser.value().username,
                               "GET /statistics/report", "统计分析");
        }

        // 注意：实际应该返回二进制文件流（PDF/Excel）
        // 这里返回JSON作为演示
        return jsonResponse(result);
    }
};

#endif // STATISTICS_SERVICE_H