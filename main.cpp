// 学生成绩管理系统后端 - C++ + Crow
#include "crow.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>

// 引入自定义头文件
#include "include/models.h"
#include "include/data_manager.h"
#include "include/auth.h"
#include "include/middleware.h"
#include "include/user_service.h"
#include "include/student_service.h"
#include "include/course_service.h"
#include "include/grade_service.h"
#include "include/statistics_service.h"
#include "include/report_service.h"
#include "include/system_service.h"

using json = nlohmann::json;

int main() {
    // 创建Crow应用实例
    crow::SimpleApp app;

    // 初始化数据管理器
    DataManager dataManager("./data");
    
    // 初始化认证管理器
    AuthManager authManager(&dataManager);
    
    // 初始化日志中间件
    LogMiddleware logger(&dataManager);
    
    // 初始化各个服务
    UserService userService(&dataManager, &authManager, &logger);
    StudentService studentService(&dataManager, &authManager, &logger);
    CourseService courseService(&dataManager, &authManager, &logger);
    GradeService gradeService(&dataManager, &authManager, &logger);
    StatisticsService statisticsService(&dataManager, &authManager, &logger);
    ReportService reportService(&dataManager, &authManager, &logger);
    SystemService systemService(&dataManager, &authManager, &logger);

    // 设置CORS头（Crow框架中不需要显式调用）

    // ==================== 认证相关路由 ====================

    // 1. 用户登录
    CROW_ROUTE(app, "/api/auth/login").methods("POST"_method)
    ([&](const crow::request& req) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            return errorResponse("BadRequest", "Invalid JSON", 400);
        }

        if (!body.contains("username") || !body.contains("password") || !body.contains("role")) {
            return errorResponse("BadRequest", "Missing required fields", 400);
        }

        std::string username = body["username"];
        std::string password = body["password"];
        std::string role = body["role"];

        auto result = authManager.login(username, password, role);
        if (!result.has_value()) {
            return errorResponse("Unauthorized", "Invalid credentials", 401);
        }

        json response = {
            {"token", result->first},
            {"user", result->second}
        };

        return jsonResponse(response);
    });

    // 2. 用户登出
    CROW_ROUTE(app, "/api/auth/logout").methods("POST"_method)
    ([&](const crow::request& req) {
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }

        authManager.logout(token.substr(7));
        return jsonResponse(std::string("Logged out successfully"));
    });

    // 3. 验证Token
    CROW_ROUTE(app, "/api/auth/verify").methods("GET"_method)
    ([&](const crow::request& req) {
        auto token = req.get_header_value("Authorization");
        if (token.empty() || token.substr(0, 7) != "Bearer ") {
            return errorResponse("Unauthorized", "Missing token", 401);
        }

        if (!authManager.verifyToken(token.substr(7))) {
            return errorResponse("Unauthorized", "Invalid token", 401);
        }

        return jsonResponse(std::string("Token valid"));
    });

    // ==================== 用户相关路由 ====================

    // 4. 获取用户信息
    CROW_ROUTE(app, "/api/user/profile").methods("GET"_method)
    ([&](const crow::request& req) {
        return userService.getCurrentUserProfile(req);
    });

    // 5. 修改密码
    CROW_ROUTE(app, "/api/user/password").methods("PUT"_method)
    ([&](const crow::request& req) {
        return userService.changePassword(req);
    });

    // 6. 获取操作日志
    CROW_ROUTE(app, "/api/user/logs").methods("GET"_method)
    ([&](const crow::request& req) {
        return userService.getUserLogs(req);
    });

    // 7. 获取用户列表（管理员）
    CROW_ROUTE(app, "/api/users").methods("GET"_method)
    ([&](const crow::request& req) {
        return userService.getUsers(req);
    });

    // 8. 创建用户（管理员）
    CROW_ROUTE(app, "/api/users").methods("POST"_method)
    ([&](const crow::request& req) {
        return userService.createUser(req);
    });

    // 9. 更新用户（管理员）
    CROW_ROUTE(app, "/api/users/<string>").methods("PUT"_method)
    ([&](const crow::request& req, const std::string& id) {
        return userService.updateUser(req, id);
    });

    // 10. 删除用户（管理员）
    CROW_ROUTE(app, "/api/users/<string>").methods("DELETE"_method)
    ([&](const crow::request& req, const std::string& id) {
        return userService.deleteUser(req, id);
    });

    // 11. 批量导入用户（管理员）
    CROW_ROUTE(app, "/api/users/batch").methods("POST"_method)
    ([&](const crow::request& req) {
        return userService.batchImportUsers(req);
    });

    // 12. 批量删除用户（管理员）
    CROW_ROUTE(app, "/api/users/batch").methods("DELETE"_method)
    ([&](const crow::request& req) {
        return userService.batchDeleteUsers(req);
    });

    // 13. 重置密码（管理员）
    CROW_ROUTE(app, "/api/users/<string>/reset-password").methods("PUT"_method)
    ([&](const crow::request& req, const std::string& id) {
        return userService.resetPassword(req, id);
    });

    // ==================== 学生相关路由 ====================

    // 14. 获取学生列表
    CROW_ROUTE(app, "/api/students").methods("GET"_method)
    ([&](const crow::request& req) {
        return studentService.getStudents(req);
    });

    // 15. 获取学生详情
    CROW_ROUTE(app, "/api/students/<string>").methods("GET"_method)
    ([&](const crow::request& req, const std::string& id) {
        return studentService.getStudent(req, id);
    });

    // 16. 添加学生
    CROW_ROUTE(app, "/api/students").methods("POST"_method)
    ([&](const crow::request& req) {
        return studentService.createStudent(req);
    });

    // 17. 更新学生
    CROW_ROUTE(app, "/api/students/<string>").methods("PUT"_method)
    ([&](const crow::request& req, const std::string& id) {
        return studentService.updateStudent(req, id);
    });

    // 18. 删除学生
    CROW_ROUTE(app, "/api/students/<string>").methods("DELETE"_method)
    ([&](const crow::request& req, const std::string& id) {
        return studentService.deleteStudent(req, id);
    });

    // 19. 批量导入学生
    CROW_ROUTE(app, "/api/students/batch").methods("POST"_method)
    ([&](const crow::request& req) {
        return studentService.batchImportStudents(req);
    });

    // 20. 导出学生数据
    CROW_ROUTE(app, "/api/students/export").methods("GET"_method)
    ([&](const crow::request& req) {
        return studentService.exportStudents(req);
    });

    // 21. 获取学生成绩概览
    CROW_ROUTE(app, "/api/students/<string>/grades").methods("GET"_method)
    ([&](const crow::request& req, const std::string& studentId) {
        return studentService.getStudentGrades(req, studentId);
    });

    // ==================== 课程相关路由 ====================

    // 22. 获取课程列表
    CROW_ROUTE(app, "/api/courses").methods("GET"_method)
    ([&](const crow::request& req) {
        return courseService.getCourses(req);
    });

    // 23. 获取课程详情
    CROW_ROUTE(app, "/api/courses/<string>").methods("GET"_method)
    ([&](const crow::request& req, const std::string& id) {
        return courseService.getCourse(req, id);
    });

    // 24. 添加课程
    CROW_ROUTE(app, "/api/courses").methods("POST"_method)
    ([&](const crow::request& req) {
        return courseService.createCourse(req);
    });

    // 25. 更新课程
    CROW_ROUTE(app, "/api/courses/<string>").methods("PUT"_method)
    ([&](const crow::request& req, const std::string& id) {
        return courseService.updateCourse(req, id);
    });

    // 26. 删除课程
    CROW_ROUTE(app, "/api/courses/<string>").methods("DELETE"_method)
    ([&](const crow::request& req, const std::string& id) {
        return courseService.deleteCourse(req, id);
    });

    // 27. 获取选课学生列表
    CROW_ROUTE(app, "/api/courses/<string>/students").methods("GET"_method)
    ([&](const crow::request& req, const std::string& courseId) {
        return courseService.getCourseStudents(req, courseId);
    });

    // ==================== 成绩相关路由 ====================

    // 28. 获取成绩列表
    CROW_ROUTE(app, "/api/grades").methods("GET"_method)
    ([&](const crow::request& req) {
        return gradeService.getGrades(req);
    });

    // 29. 录入成绩
    CROW_ROUTE(app, "/api/grades").methods("POST"_method)
    ([&](const crow::request& req) {
        return gradeService.createGrade(req);
    });

    // 30. 更新成绩
    CROW_ROUTE(app, "/api/grades/<string>").methods("PUT"_method)
    ([&](const crow::request& req, const std::string& id) {
        return gradeService.updateGrade(req, id);
    });

    // 31. 删除成绩
    CROW_ROUTE(app, "/api/grades/<string>").methods("DELETE"_method)
    ([&](const crow::request& req, const std::string& id) {
        return gradeService.deleteGrade(req, id);
    });

    // 32. 批量导入成绩
    CROW_ROUTE(app, "/api/grades/batch").methods("POST"_method)
    ([&](const crow::request& req) {
        return gradeService.batchImportGrades(req);
    });

    // 33. 导出成绩数据
    CROW_ROUTE(app, "/api/grades/export").methods("GET"_method)
    ([&](const crow::request& req) {
        return gradeService.exportGrades(req);
    });

    // 34. 获取课程成绩列表
    CROW_ROUTE(app, "/api/grades/course/<string>").methods("GET"_method)
    ([&](const crow::request& req, const std::string& courseId) {
        return gradeService.getCourseGrades(req, courseId);
    });

    // 35. 批量更新成绩
    CROW_ROUTE(app, "/api/grades/batch-update").methods("POST"_method)
    ([&](const crow::request& req) {
        return gradeService.batchUpdateGrades(req);
    });

    // ==================== 统计分析路由 ====================

    // 36. 获取统计概览
    CROW_ROUTE(app, "/api/statistics/overview").methods("GET"_method)
    ([&](const crow::request& req) {
        return statisticsService.getOverview(req);
    });

    // 37. 按班级统计
    CROW_ROUTE(app, "/api/statistics/class").methods("GET"_method)
    ([&](const crow::request& req) {
        return statisticsService.getClassStatistics(req);
    });

    // 38. 按课程统计
    CROW_ROUTE(app, "/api/statistics/course").methods("GET"_method)
    ([&](const crow::request& req) {
        return statisticsService.getCourseStatistics(req);
    });

    // 39. 获取排名列表
    CROW_ROUTE(app, "/api/statistics/ranking").methods("GET"_method)
    ([&](const crow::request& req) {
        return statisticsService.getRanking(req);
    });

    // 40. 获取成绩分布
    CROW_ROUTE(app, "/api/statistics/distribution").methods("GET"_method)
    ([&](const crow::request& req) {
        return statisticsService.getDistribution(req);
    });

    // 41. 生成统计报表
    CROW_ROUTE(app, "/api/statistics/report").methods("GET"_method)
    ([&](const crow::request& req) {
        return statisticsService.generateReport(req);
    });

    // ==================== 报表管理路由 ====================

    // 42. 生成成绩单
    CROW_ROUTE(app, "/api/reports/report-card").methods("GET"_method)
    ([&](const crow::request& req) {
        return reportService.generateReportCard(req);
    });

    // 43. 生成统计报表
    CROW_ROUTE(app, "/api/reports/statistics").methods("GET"_method)
    ([&](const crow::request& req) {
        return reportService.generateStatisticsReport(req);
    });

    // 44. 打印准备
    CROW_ROUTE(app, "/api/reports/print").methods("POST"_method)
    ([&](const crow::request& req) {
        return reportService.printPrepare(req);
    });

    // 45. 批量打印
    CROW_ROUTE(app, "/api/reports/batch-print").methods("POST"_method)
    ([&](const crow::request& req) {
        return reportService.batchPrint(req);
    });

    // ==================== 系统管理路由 ====================

    // 46. 创建备份
    CROW_ROUTE(app, "/api/system/backup").methods("POST"_method)
    ([&](const crow::request& req) {
        return systemService.createBackup(req);
    });

    // 47. 获取备份列表
    CROW_ROUTE(app, "/api/system/backups").methods("GET"_method)
    ([&](const crow::request& req) {
        return systemService.getBackups(req);
    });

    // 48. 恢复备份
    CROW_ROUTE(app, "/api/system/restore").methods("POST"_method)
    ([&](const crow::request& req) {
        return systemService.restoreBackup(req);
    });

    // 49. 删除备份
    CROW_ROUTE(app, "/api/system/backups/<string>").methods("DELETE"_method)
    ([&](const crow::request& req, const std::string& backupId) {
        return systemService.deleteBackup(req, backupId);
    });

    // 50. 获取系统日志
    CROW_ROUTE(app, "/api/system/logs").methods("GET"_method)
    ([&](const crow::request& req) {
        return systemService.getSystemLogs(req);
    });

    // 51. 获取系统设置
    CROW_ROUTE(app, "/api/system/settings").methods("GET"_method)
    ([&](const crow::request& req) {
        return systemService.getSettings(req);
    });

    // 52. 更新系统设置
    CROW_ROUTE(app, "/api/system/settings").methods("PUT"_method)
    ([&](const crow::request& req) {
        return systemService.updateSettings(req);
    });

    // 53. 清理日志
    CROW_ROUTE(app, "/api/system/clean-logs").methods("POST"_method)
    ([&](const crow::request& req) {
        return systemService.cleanLogs(req);
    });

    // 54. 导出日志
    CROW_ROUTE(app, "/api/system/export-logs").methods("GET"_method)
    ([&](const crow::request& req) {
        return systemService.exportLogs(req);
    });

    // ==================== 测试路由 ====================

    // 测试路由
    CROW_ROUTE(app, "/api/health").methods("GET"_method)
    ([]() {
        json result = {{"status", "ok"}, {"message", "Server is running"}};
        return crow::response(result.dump());
    });

    // 启动服务
    std::cout << "Starting server on port 21180..." << std::endl;
    std::cout << "API Base URL: http://localhost:21180/api" << std::endl;
    std::cout << "Default admin: username=admin, role=admin" << std::endl;
    
    app.port(21180).multithreaded().run();

    return 0;
}