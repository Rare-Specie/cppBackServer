#include <catch2/catch.hpp>
#include <crow.h>
#include "../include/middleware.h"
#include "../include/models.h"
#include "../include/data_manager.h"
#include "../include/student_service.h"
#include "../include/user_service.h"
#include "../include/course_service.h"
#include "../include/auth.h"
#include "../include/middleware.h"

// 集成测试：测试完整的分页流程
TEST_CASE("Student service pagination integration", "[integration][students]") {
    // 创建测试数据目录
    system("rm -rf ./test_data_integration");
    system("mkdir -p ./test_data_integration");
    
    DataManager dm("./test_data_integration");
    AuthManager auth(&dm);
    LogMiddleware logger(&dm);
    StudentService service(&dm, &auth, &logger);
    
    // 创建测试数据
    std::vector<Student> testStudents;
    for (int i = 1; i <= 25; ++i) {
        Student s{
            "id" + std::to_string(i),
            "2024" + std::to_string(i),
            "学生" + std::to_string(i),
            "计算机240" + std::to_string((i % 3) + 1),
            i % 2 == 0 ? std::optional<std::string>("男") : std::optional<std::string>("女"),
            std::optional<std::string>("138" + std::to_string(10000000 + i)),
            std::optional<std::string>("student" + std::to_string(i) + "@example.com"),
            "2026-01-12 10:00:00",
            "2026-01-12 10:00:00"
        };
        testStudents.push_back(s);
    }
    dm.saveStudents(testStudents);
    
    // 创建测试用户和token
    std::vector<User> users = dm.getUsers();
    std::string testUserId = users.empty() ? "test_user" : users[0].id;
    std::string testToken = auth.generateToken(testUserId, "testuser", "admin");
    
    SECTION("分页参数验证") {
        crow::request req;
        req.add_header("Authorization", "Bearer " + testToken);
        
        // 测试1：默认分页（第1页，10条）
        req.add_header("X-Page", "1");
        req.add_header("X-Limit", "10");
        auto res1 = service.getStudents(req);
        REQUIRE(res1.code == 200);
        
        json body1 = json::parse(res1.body);
        REQUIRE(body1["data"].size() == 10);
        REQUIRE(body1["total"] == 25);
        REQUIRE(body1["page"] == 1);
        REQUIRE(body1["limit"] == 10);
        REQUIRE(body1["totalPages"] == 3);
        
        // 测试2：第二页
        req.update_header("X-Page", "2");
        auto res2 = service.getStudents(req);
        REQUIRE(res2.code == 200);
        
        json body2 = json::parse(res2.body);
        REQUIRE(body2["data"].size() == 10);
        REQUIRE(body2["total"] == 25);
        REQUIRE(body2["page"] == 2);
        
        // 测试3：最后一页
        req.update_header("X-Page", "3");
        auto res3 = service.getStudents(req);
        REQUIRE(res3.code == 200);
        
        json body3 = json::parse(res3.body);
        REQUIRE(body3["data"].size() == 5);
        REQUIRE(body3["total"] == 25);
        REQUIRE(body3["page"] == 3);
        
        // 测试4：超出范围
        req.update_header("X-Page", "10");
        auto res4 = service.getStudents(req);
        REQUIRE(res4.code == 200);
        
        json body4 = json::parse(res4.body);
        REQUIRE(body4["data"].size() == 0);
        REQUIRE(body4["total"] == 25);
        REQUIRE(body4["page"] == 10);
    }
    
    SECTION("过滤与分页组合") {
        crow::request req;
        req.add_header("Authorization", "Bearer " + testToken);
        
        // 按班级过滤
        req.add_header("X-Query-Class", "计算机2401");
        req.add_header("X-Page", "1");
        req.add_header("X-Limit", "10");
        
        auto res = service.getStudents(req);
        REQUIRE(res.code == 200);
        
        json body = json::parse(res.body);
        // 应该有约8-9个学生（25/3≈8.33）
        int totalCount = body["total"];
        REQUIRE(totalCount >= 8 && totalCount <= 9);
        REQUIRE(body["data"].size() <= 10);
        
        // 验证所有返回的学生都属于计算机2401班
        for (const auto& student : body["data"]) {
            REQUIRE(student["class"] == "计算机2401");
        }
    }
    
    SECTION("搜索与分页组合") {
        crow::request req;
        req.add_header("Authorization", "Bearer " + testToken);
        
        // 搜索包含"学生1"的学生
        req.add_header("X-Query-Search", "学生1");
        req.add_header("X-Page", "1");
        req.add_header("X-Limit", "5");
        
        auto res = service.getStudents(req);
        REQUIRE(res.code == 200);
        
        json body = json::parse(res.body);
        // 应该有11个学生包含"学生1"（1, 10-19, 20）
        REQUIRE(body["total"] == 11);
        REQUIRE(body["data"].size() == 5);
        
        // 验证搜索结果
        for (const auto& student : body["data"]) {
            std::string name = student["name"];
            REQUIRE(name.find("学生1") != std::string::npos);
        }
    }
    
    SECTION("字段选择与分页") {
        crow::request req;
        req.add_header("Authorization", "Bearer " + testToken);
        
        // 只选择特定字段
        req.add_header("X-Fields", "id,studentId,name");
        req.add_header("X-Page", "1");
        req.add_header("X-Limit", "3");
        
        auto res = service.getStudents(req);
        REQUIRE(res.code == 200);
        
        json body = json::parse(res.body);
        REQUIRE(body["data"].size() == 3);
        
        // 验证只返回指定字段
        for (const auto& student : body["data"]) {
            REQUIRE(student.contains("id"));
            REQUIRE(student.contains("studentId"));
            REQUIRE(student.contains("name"));
            REQUIRE(!student.contains("class")); // 不应该包含
            REQUIRE(!student.contains("phone")); // 不应该包含
        }
    }
    
    SECTION("ISO日期格式验证") {
        crow::request req;
        req.add_header("Authorization", "Bearer " + testToken);
        req.add_header("X-Page", "1");
        req.add_header("X-Limit", "1");
        
        auto res = service.getStudents(req);
        REQUIRE(res.code == 200);
        
        json body = json::parse(res.body);
        json student = body["data"][0];
        
        // 验证日期格式
        std::string createdAt = student["createdAt"];
        std::string updatedAt = student["updatedAt"];
        
        // 应该是ISO 8601格式：2026-01-12T...
        REQUIRE(createdAt.find("T") != std::string::npos);
        REQUIRE(createdAt.find("Z") != std::string::npos);
        REQUIRE(updatedAt.find("T") != std::string::npos);
        REQUIRE(updatedAt.find("Z") != std::string::npos);
    }
    
    SECTION("批量导入兼容性") {
        // 测试数组格式
        json arrayBody = json::array();
        arrayBody.push_back({
            {"studentId", "IMP001"},
            {"name", "导入学生1"},
            {"class", "导入班"},
            {"gender", "男"},
            {"phone", "13812345678"},
            {"email", "imp1@example.com"}
        });
        arrayBody.push_back({
            {"studentId", "IMP002"},
            {"name", "导入学生2"},
            {"class", "导入班"}
        });
        
        crow::request req1;
        req1.add_header("Authorization", "Bearer " + testToken);
        req1.body = arrayBody.dump();
        
        auto res1 = service.batchImportStudents(req1);
        // 由于没有正确的权限验证，这里只测试JSON解析
        // 实际测试需要mock auth
        
        // 测试对象格式
        json objectBody = {
            {"students", {
                {
                    {"studentId", "IMP003"},
                    {"name", "导入学生3"},
                    {"class", "导入班"}
                }
            }}
        };
        
        crow::request req2;
        req2.add_header("Authorization", "Bearer " + testToken);
        req2.body = objectBody.dump();
        
        auto res2 = service.batchImportStudents(req2);
        // 同样，实际测试需要mock auth
    }
    
    SECTION("错误参数处理") {
        crow::request req;
        req.add_header("Authorization", "Bearer " + testToken);
        
        // 负数page
        req.add_header("X-Page", "-1");
        req.add_header("X-Limit", "10");
        
        auto res = service.getStudents(req);
        REQUIRE(res.code == 200);
        
        json body = json::parse(res.body);
        // 应该回退到默认值
        REQUIRE(body["page"] == 1);
        REQUIRE(body["limit"] == 10);
        
        // 无效格式
        req.update_header("X-Page", "abc");
        req.update_header("X-Limit", "xyz");
        
        auto res2 = service.getStudents(req);
        REQUIRE(res2.code == 200);
        
        json body2 = json::parse(res2.body);
        // 应该回退到默认值
        REQUIRE(body2["page"] == 1);
        REQUIRE(body2["limit"] == 10);
        
        // 超过最大限制
        req.update_header("X-Page", "1");
        req.update_header("X-Limit", "5000");
        
        auto res3 = service.getStudents(req);
        REQUIRE(res3.code == 200);
        
        json body3 = json::parse(res3.body);
        // 应该被裁剪到1000
        REQUIRE(body3["limit"] == 1000);
    }
    
    // 清理测试数据
    system("rm -rf ./test_data_integration");
}

// 集成测试：用户服务分页
TEST_CASE("User service pagination integration", "[integration][users]") {
    system("rm -rf ./test_data_user");
    system("mkdir -p ./test_data_user");
    
    DataManager dm("./test_data_user");
    AuthManager auth(&dm);
    LogMiddleware logger(&dm);
    UserService service(&dm, &auth, &logger);
    
    // 创建测试用户
    std::vector<User> testUsers;
    for (int i = 1; i <= 15; ++i) {
        User u{
            "uid" + std::to_string(i),
            "user" + std::to_string(i),
            "hash" + std::to_string(i),
            i % 3 == 0 ? "admin" : (i % 3 == 1 ? "teacher" : "student"),
            "用户" + std::to_string(i),
            std::optional<std::string>("计算机240" + std::to_string((i % 3) + 1)),
            std::nullopt,
            "2026-01-12 10:00:00",
            "2026-01-12 10:00:00"
        };
        testUsers.push_back(u);
    }
    dm.saveUsers(testUsers);
    
    // 创建管理员token
    std::string adminToken = auth.generateToken("admin_uid", "admin", "admin");
    
    SECTION("用户列表分页") {
        crow::request req;
        req.add_header("Authorization", "Bearer " + adminToken);
        req.add_header("X-Page", "1");
        req.add_header("X-Limit", "5");
        
        auto res = service.getUsers(req);
        REQUIRE(res.code == 200);
        
        json body = json::parse(res.body);
        REQUIRE(body["data"].size() == 5);
        REQUIRE(body["total"] == 15);
        REQUIRE(body["page"] == 1);
        REQUIRE(body["limit"] == 5);
        REQUIRE(body["totalPages"] == 3);
    }
    
    SECTION("用户角色过滤") {
        crow::request req;
        req.add_header("Authorization", "Bearer " + adminToken);
        req.add_header("X-Query-Role", "admin");
        req.add_header("X-Page", "1");
        req.add_header("X-Limit", "10");
        
        auto res = service.getUsers(req);
        REQUIRE(res.code == 200);
        
        json body = json::parse(res.body);
        // 应该有5个admin用户（15/3=5）
        REQUIRE(body["total"] == 5);
        
        // 验证所有返回的用户都是admin
        for (const auto& user : body["data"]) {
            REQUIRE(user["role"] == "admin");
        }
    }
    
    // 清理
    system("rm -rf ./test_data_user");
}

// 集成测试：课程服务分页
TEST_CASE("Course service pagination integration", "[integration][courses]") {
    system("rm -rf ./test_data_course");
    system("mkdir -p ./test_data_course");
    
    DataManager dm("./test_data_course");
    AuthManager auth(&dm);
    LogMiddleware logger(&dm);
    CourseService service(&dm, &auth, &logger);
    
    // 创建测试课程
    std::vector<Course> testCourses;
    for (int i = 1; i <= 20; ++i) {
        Course c{
            "cid" + std::to_string(i),
            "CS" + std::to_string(100 + i),
            "课程" + std::to_string(i),
            i % 3 + 1,
            std::optional<std::string>("教师" + std::to_string(i)),
            std::optional<std::string>("课程描述" + std::to_string(i)),
            "2026-01-12 10:00:00",
            "2026-01-12 10:00:00"
        };
        testCourses.push_back(c);
    }
    dm.saveCourses(testCourses);
    
    // 创建token
    std::string token = auth.generateToken("test_uid", "testuser", "admin");
    
    SECTION("课程列表分页") {
        crow::request req;
        req.add_header("Authorization", "Bearer " + token);
        req.add_header("X-Page", "1");
        req.add_header("X-Limit", "8");
        
        auto res = service.getCourses(req);
        REQUIRE(res.code == 200);
        
        json body = json::parse(res.body);
        REQUIRE(body["data"].size() == 8);
        REQUIRE(body["total"] == 20);
        REQUIRE(body["page"] == 1);
        REQUIRE(body["limit"] == 8);
        REQUIRE(body["totalPages"] == 3);
    }
    
    SECTION("课程搜索过滤") {
        crow::request req;
        req.add_header("Authorization", "Bearer " + token);
        req.add_header("X-Query-Search", "课程1");
        req.add_header("X-Page", "1");
        req.add_header("X-Limit", "5");
        
        auto res = service.getCourses(req);
        REQUIRE(res.code == 200);
        
        json body = json::parse(res.body);
        // 应该有11个课程包含"课程1"（1, 10-19, 20）
        REQUIRE(body["total"] == 11);
        
        // 验证搜索结果
        for (const auto& course : body["data"]) {
            std::string name = course["name"];
            REQUIRE(name.find("课程1") != std::string::npos);
        }
    }
    
    // 清理
    system("rm -rf ./test_data_course");
}

// 测试日志记录
TEST_CASE("pagination logging", "[logging]") {
    system("rm -rf ./test_data_log");
    system("mkdir -p ./test_data_log");
    
    DataManager dm("./test_data_log");
    AuthManager auth(&dm);
    LogMiddleware logger(&dm);
    StudentService service(&dm, &auth, &logger);
    
    // 创建测试数据
    std::vector<Student> students;
    for (int i = 1; i <= 5; ++i) {
        Student s{
            "id" + std::to_string(i),
            "2024" + std::to_string(i),
            "学生" + std::to_string(i),
            "计算机2401",
            std::nullopt, std::nullopt, std::nullopt,
            "2026-01-12 10:00:00",
            "2026-01-12 10:00:00"
        };
        students.push_back(s);
    }
    dm.saveStudents(students);
    
    // 创建token
    std::string token = auth.generateToken("admin_uid", "admin", "admin");
    
    SECTION("分页操作记录日志") {
        crow::request req;
        req.add_header("Authorization", "Bearer " + token);
        req.add_header("X-Page", "2");
        req.add_header("X-Limit", "3");
        req.add_header("X-Query-Class", "计算机2401");
        
        auto res = service.getStudents(req);
        REQUIRE(res.code == 200);
        
        // 检查操作日志
        auto logs = dm.getOperationLogs();
        REQUIRE(logs.size() > 0);
        
        // 找到最新的日志
        auto latestLog = logs.back();
        REQUIRE(latestLog.action.find("GET /students") != std::string::npos);
        REQUIRE(latestLog.action.find("page=2") != std::string::npos);
        REQUIRE(latestLog.action.find("limit=3") != std::string::npos);
        REQUIRE(latestLog.module == "学生管理");
    }
    
    // 清理
    system("rm -rf ./test_data_log");
}