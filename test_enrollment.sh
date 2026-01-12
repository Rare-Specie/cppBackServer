#!/bin/bash

# 测试学生选课功能的脚本
# 需要先启动后端服务

BASE_URL="http://localhost:21180/api"
TOKEN=""

echo "=== 学生选课功能测试 ==="

# 1. 登录获取Token
echo "1. 正在登录..."
LOGIN_RESPONSE=$(curl -s -X POST "$BASE_URL/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "admin123", "role": "admin"}')

TOKEN=$(echo $LOGIN_RESPONSE | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
echo "登录成功，Token: ${TOKEN:0:20}..."

# 2. 创建测试课程
echo "2. 创建测试课程..."
COURSE_RESPONSE=$(curl -s -X POST "$BASE_URL/courses" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "courseId": "TEST101",
    "name": "测试课程",
    "credit": 3,
    "teacher": "张老师",
    "description": "用于测试选课功能的课程"
  }')

COURSE_ID=$(echo $COURSE_RESPONSE | grep -o '"id":"[^"]*"' | cut -d'"' -f4)
echo "创建课程成功，ID: $COURSE_ID"

# 3. 创建测试学生
echo "3. 创建测试学生..."
STUDENT_RESPONSE=$(curl -s -X POST "$BASE_URL/students" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "studentId": "2024999",
    "name": "测试学生",
    "class": "计算机2401",
    "gender": "男"
  }')

STUDENT_ID=$(echo $STUDENT_RESPONSE | grep -o '"id":"[^"]*"' | cut -d'"' -f4)
echo "创建学生成功，ID: $STUDENT_ID"

# 4. 学生选课
echo "4. 学生选课..."
ENROLL_RESPONSE=$(curl -s -X POST "$BASE_URL/courses/$COURSE_ID/enroll" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"studentId": "2024999"}')

echo "选课响应: $ENROLL_RESPONSE"

# 5. 查看选课学生列表
echo "5. 查看选课学生列表..."
STUDENTS_RESPONSE=$(curl -s -X GET "$BASE_URL/courses/$COURSE_ID/students" \
  -H "Authorization: Bearer $TOKEN")

echo "选课学生列表: $STUDENTS_RESPONSE"

# 6. 取消选课
echo "6. 取消选课..."
UNENROLL_RESPONSE=$(curl -s -X DELETE "$BASE_URL/courses/$COURSE_ID/enroll/2024999" \
  -H "Authorization: Bearer $TOKEN")

echo "取消选课响应: $UNENROLL_RESPONSE"

# 7. 再次查看选课学生列表（应该为空）
echo "7. 再次查看选课学生列表..."
STUDENTS_RESPONSE2=$(curl -s -X GET "$BASE_URL/courses/$COURSE_ID/students" \
  -H "Authorization: Bearer $TOKEN")

echo "选课学生列表（取消后）: $STUDENTS_RESPONSE2"

# 8. 清理测试数据
echo "8. 清理测试数据..."
# 删除课程
curl -s -X DELETE "$BASE_URL/courses/$COURSE_ID" \
  -H "Authorization: Bearer $TOKEN" > /dev/null

# 删除学生
curl -s -X DELETE "$BASE_URL/students/$STUDENT_ID" \
  -H "Authorization: Bearer $TOKEN" > /dev/null

echo "=== 测试完成 ==="