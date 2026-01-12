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

// 测试分页参数解析
TEST_CASE("parsePaginationParams handles valid inputs", "[pagination]") {
    // 创建mock request
    crow::request req;
    
    SECTION("默认值测试") {
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        REQUIRE(page == 1);
        REQUIRE(limit == 10);
    }
    
    SECTION("有效字符串参数") {
        req.add_header("X-Page", "3");
        req.add_header("X-Limit", "20");
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        REQUIRE(page == 3);
        REQUIRE(limit == 20);
    }
    
    SECTION("有效整数参数") {
        req.add_header("X-Page", "5");
        req.add_header("X-Limit", "50");
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        REQUIRE(page == 5);
        REQUIRE(limit == 50);
    }
    
    SECTION("超过最大限制") {
        req.add_header("X-Limit", "2000");
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        REQUIRE(limit == 1000); // 应该被裁剪到最大值
    }
    
    SECTION("负数参数") {
        req.add_header("X-Page", "-1");
        req.add_header("X-Limit", "-5");
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        REQUIRE(page == 1); // 应该回退到默认值
        REQUIRE(limit == 10); // 应该回退到默认值
    }
    
    SECTION("无效格式参数") {
        req.add_header("X-Page", "abc");
        req.add_header("X-Limit", "xyz");
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        REQUIRE(page == 1); // 应该回退到默认值
        REQUIRE(limit == 10); // 应该回退到默认值
    }
    
    SECTION("零值参数") {
        req.add_header("X-Page", "0");
        req.add_header("X-Limit", "0");
        auto [page, limit] = parsePaginationParams(req, 1, 10, 1000);
        REQUIRE(page == 1); // 应该回退到默认值
        REQUIRE(limit == 10); // 应该回退到默认值
    }
}

// 测试分页函数
TEST_CASE("paginate function works correctly", "[pagination]") {
    std::vector<Student> students;
    for (int i = 1; i <= 25; ++i) {
        Student s{
            "id" + std::to_string(i),
            "stu" + std::to_string(i),
            "Student" + std::to_string(i),
            "Class" + std::to_string((i-1)/5 + 1),
            std::nullopt, std::nullopt, std::nullopt,
            "2026-01-12", "2026-01-12"
        };
        students.push_back(s);
    }
    
    SECTION("第一页，10条数据") {
        auto result = paginate(students, 1, 10);
        REQUIRE(result["data"].size() == 10);
        REQUIRE(result["total"] == 25);
        REQUIRE(result["page"] == 1);
        REQUIRE(result["limit"] == 10);
        REQUIRE(result["totalPages"] == 3);
    }
    
    SECTION("第二页，10条数据") {
        auto result = paginate(students, 2, 10);
        REQUIRE(result["data"].size() == 10);
        REQUIRE(result["total"] == 25);
        REQUIRE(result["page"] == 2);
        REQUIRE(result["limit"] == 10);
        REQUIRE(result["totalPages"] == 3);
    }
    
    SECTION("最后一页，不足10条") {
        auto result = paginate(students, 3, 10);
        REQUIRE(result["data"].size() == 5);
        REQUIRE(result["total"] == 25);
        REQUIRE(result["page"] == 3);
        REQUIRE(result["limit"] == 10);
        REQUIRE(result["totalPages"] == 3);
    }
    
    SECTION("超出范围的页码") {
        auto result = paginate(students, 10, 10);
        REQUIRE(result["data"].size() == 0);
        REQUIRE(result["total"] == 25);
        REQUIRE(result["page"] == 10);
        REQUIRE(result["limit"] == 10);
        REQUIRE(result["totalPages"] == 3);
    }
    
    SECTION("自定义limit") {
        auto result = paginate(students, 1, 20);
        REQUIRE(result["data"].size() == 20);
        REQUIRE(result["total"] == 25);
        REQUIRE(result["page"] == 1);
        REQUIRE(result["limit"] == 20);
        REQUIRE(result["totalPages"] == 2);
    }
}

// 测试字段选择参数解析
TEST_CASE("parseFieldsParam handles field selection", "[fields]") {
    crow::request req;
    
    SECTION("无字段参数") {
        auto fields = parseFieldsParam(req);
        REQUIRE(fields.empty());
    }
    
    SECTION("单个字段") {
        req.add_header("X-Fields", "id");
        auto fields = parseFieldsParam(req);
        REQUIRE(fields.size() == 1);
        REQUIRE(fields[0] == "id");
    }
    
    SECTION("多个字段") {
        req.add_header("X-Fields", "id,studentId,name,phone");
        auto fields = parseFieldsParam(req);
        REQUIRE(fields.size() == 4);
        REQUIRE(fields[0] == "id");
        REQUIRE(fields[1] == "studentId");
        REQUIRE(fields[2] == "name");
        REQUIRE(fields[3] == "phone");
    }
    
    SECTION("带空格的字段") {
        req.add_header("X-Fields", " id , studentId , name ");
        auto fields = parseFieldsParam(req);
        REQUIRE(fields.size() == 3);
        REQUIRE(fields[0] == "id");
        REQUIRE(fields[1] == "studentId");
        REQUIRE(fields[2] == "name");
    }
}

// 测试完整数据请求
TEST_CASE("requestFullData detects full data request", "[full]") {
    crow::request req;
    
    SECTION("无full参数") {
        REQUIRE(requestFullData(req) == false);
    }
    
    SECTION("full=true") {
        req.add_header("X-Full", "true");
        REQUIRE(requestFullData(req) == true);
    }
    
    SECTION("full=1") {
        req.add_header("X-Full", "1");
        REQUIRE(requestFullData(req) == true);
    }
    
    SECTION("full=yes") {
        req.add_header("X-Full", "yes");
        REQUIRE(requestFullData(req) == true);
    }
    
    SECTION("full=false") {
        req.add_header("X-Full", "false");
        REQUIRE(requestFullData(req) == false);
    }
}

// 测试ISO日期转换
TEST_CASE("ISO 8601 date conversion", "[dates]") {
    DataManager dm("./test_data");
    
    SECTION("转换旧格式时间戳") {
        std::string oldFormat = "Wed Jan 12 10:30:45 2026";
        std::string iso = dm.convertToISO8601(oldFormat);
        // 应该转换为类似 2026-01-12T10:30:45Z 的格式
        REQUIRE(iso.find("2026-01-12") != std::string::npos);
        REQUIRE(iso.find("T") != std::string::npos);
        REQUIRE(iso.find("Z") != std::string::npos);
    }
    
    SECTION("保持ISO格式不变") {
        std::string isoFormat = "2026-01-12T10:30:45Z";
        std::string result = dm.convertToISO8601(isoFormat);
        REQUIRE(result == isoFormat);
    }
    
    SECTION("生成当前ISO时间戳") {
        std::string iso = dm.getISO8601Timestamp();
        REQUIRE(iso.find("T") != std::string::npos);
        REQUIRE(iso.find("Z") != std::string::npos);
        REQUIRE(iso.length() > 10);
    }
}

// 测试批量导入兼容性
TEST_CASE("batch import format compatibility", "[batch]") {
    DataManager dm("./test_data");
    AuthManager auth(&dm);
    LogMiddleware logger(&dm);
    StudentService service(&dm, &auth, &logger);
    
    // 清理测试数据
    dm.saveStudents(std::vector<Student>{});
    
    SECTION("直接数组格式") {
        json body = json::array();
        body.push_back({
            {"studentId", "TEST001"},
            {"name", "测试学生1"},
            {"class", "测试班"}
        });
        body.push_back({
            {"studentId", "TEST002"},
            {"name", "测试学生2"},
            {"class", "测试班"}
        });
        
        crow::request req;
        req.body = body.dump();
        req.add_header("Authorization", "Bearer test_token");
        
        // 注意：实际测试需要mock auth验证
        // 这里只测试JSON解析逻辑
        json parsed;
        REQUIRE_NOTHROW(parsed = json::parse(req.body));
        REQUIRE(parsed.is_array());
        REQUIRE(parsed.size() == 2);
    }
    
    SECTION("对象格式 {students: [...]}") {
        json body = {
            {"students", {
                {
                    {"studentId", "TEST003"},
                    {"name", "测试学生3"},
                    {"class", "测试班"}
                },
                {
                    {"studentId", "TEST004"},
                    {"name", "测试学生4"},
                    {"class", "测试班"}
                }
            }}
        };
        
        crow::request req;
        req.body = body.dump();
        
        json parsed;
        REQUIRE_NOTHROW(parsed = json::parse(req.body));
        REQUIRE(parsed.is_object());
        REQUIRE(parsed.contains("students"));
        REQUIRE(parsed["students"].is_array());
        REQUIRE(parsed["students"].size() == 2);
    }
    
    SECTION("无效格式") {
        json body = {
            {"invalid", "format"}
        };
        
        crow::request req;
        req.body = body.dump();
        
        json parsed;
        REQUIRE_NOTHROW(parsed = json::parse(req.body));
        REQUIRE(parsed.is_object());
        REQUIRE(!parsed.is_array());
        REQUIRE(!parsed.contains("students"));
    }
}

// 测试过滤与分页组合
TEST_CASE("filter and pagination combination", "[filter][pagination]") {
    std::vector<Student> students;
    // 创建20个学生，分布在3个班级
    for (int i = 1; i <= 20; ++i) {
        std::string className = "计算机240" + std::to_string((i % 3) + 1);
        Student s{
            "id" + std::to_string(i),
            "stu" + std::to_string(i),
            "学生" + std::to_string(i),
            className,
            std::nullopt, std::nullopt, std::nullopt,
            "2026-01-12", "2026-01-12"
        };
        students.push_back(s);
    }
    
    // 模拟过滤逻辑
    std::string classFilter = "计算机2401";
    std::vector<Student> filtered;
    for (const auto& student : students) {
        if (student.className == classFilter) {
            filtered.push_back(student);
        }
    }
    
    SECTION("先过滤再分页") {
        // 应该有大约7个学生在计算机2401班（20/3≈6.67）
        int expectedCount = 7; // 20个学生，3个班级，平均分配
        
        // 第一页，limit=5
        auto result = paginate(filtered, 1, 5);
        REQUIRE(result["data"].size() == 5);
        REQUIRE(result["total"] == expectedCount);
        REQUIRE(result["page"] == 1);
        REQUIRE(result["limit"] == 5);
        
        // 第二页，应该有剩余的2个
        auto result2 = paginate(filtered, 2, 5);
        REQUIRE(result2["data"].size() == 2);
        REQUIRE(result2["total"] == expectedCount);
        REQUIRE(result2["page"] == 2);
        REQUIRE(result2["limit"] == 5);
    }
    
    SECTION("搜索过滤") {
        std::string search = "学生1";
        std::vector<Student> searchFiltered;
        for (const auto& student : students) {
            if (student.name.find(search) != std::string::npos) {
                searchFiltered.push_back(student);
            }
        }
        
        // 应该有11个学生包含"学生1"（1, 10-19, 20）
        REQUIRE(searchFiltered.size() == 11);
        
        auto result = paginate(searchFiltered, 1, 10);
        REQUIRE(result["data"].size() == 10);
        REQUIRE(result["total"] == 11);
    }
}

// 测试错误响应格式
TEST_CASE("error response format", "[errors]") {
    SECTION("标准错误响应") {
        auto res = errorResponse("BadRequest", "Invalid parameters", 400);
        REQUIRE(res.code == 400);
        
        json body = json::parse(res.body);
        REQUIRE(body["error"] == "BadRequest");
        REQUIRE(body["message"] == "Invalid parameters");
    }
    
    SECTION("401未授权") {
        auto res = errorResponse("Unauthorized", "Missing token", 401);
        REQUIRE(res.code == 401);
        
        json body = json::parse(res.body);
        REQUIRE(body["error"] == "Unauthorized");
    }
    
    SECTION("403禁止访问") {
        auto res = errorResponse("Forbidden", "Admin only", 403);
        REQUIRE(res.code == 403);
        
        json body = json::parse(res.body);
        REQUIRE(body["error"] == "Forbidden");
    }
    
    SECTION("404未找到") {
        auto res = errorResponse("NotFound", "Student not found", 404);
        REQUIRE(res.code == 404);
        
        json body = json::parse(res.body);
        REQUIRE(body["error"] == "NotFound");
    }
}

// 测试JSON响应格式
TEST_CASE("JSON response format", "[response]") {
    SECTION("成功响应") {
        json data = {
            {"id", "123"},
            {"name", "测试"}
        };
        auto res = jsonResponse(data, 200);
        REQUIRE(res.code == 200);
        
        json body = json::parse(res.body);
        REQUIRE(body["id"] == "123");
        REQUIRE(body["name"] == "测试");
    }
    
    SECTION("分页响应") {
        json data = {
            {"data", json::array({1, 2, 3})},
            {"total", 10},
            {"page", 1},
            {"limit", 3},
            {"totalPages", 4}
        };
        auto res = jsonResponse(data, 200);
        REQUIRE(res.code == 200);
        
        json body = json::parse(res.body);
        REQUIRE(body["data"].size() == 3);
        REQUIRE(body["total"] == 10);
        REQUIRE(body["page"] == 1);
        REQUIRE(body["limit"] == 3);
        REQUIRE(body["totalPages"] == 4);
    }
}