#!/bin/bash

# 测试学生登录后查看自己成绩
BASE_URL="http://localhost:21180/api"

# 1. 管理员登录
LOGIN_RESPONSE=$(curl -s -X POST "$BASE_URL/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "admin123", "role": "admin"}')
TOKEN=$(echo $LOGIN_RESPONSE | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
if [ -z "$TOKEN" ]; then
  echo "Admin login failed"
  exit 1
fi

# 2. 创建学生
STUDENT_RESPONSE=$(curl -s -X POST "$BASE_URL/students" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"studentId":"S2001","name":"测试学生2","class":"计算机2401"}')
STUDENT_ID=$(echo $STUDENT_RESPONSE | grep -o '"id":"[^"]*"' | cut -d'"' -f4)
if [ -z "$STUDENT_ID" ]; then
  echo "Create student failed: $STUDENT_RESPONSE"
  exit 1
fi

# 3. 创建课程
COURSE_RESPONSE=$(curl -s -X POST "$BASE_URL/courses" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"courseId":"TEST200","name":"测试课程2","credit":2}')
COURSE_ID=$(echo $COURSE_RESPONSE | grep -o '"id":"[^"]*"' | cut -d'"' -f4)
if [ -z "$COURSE_ID" ]; then
  echo "Create course failed: $COURSE_RESPONSE"
  exit 1
fi

# 4. 录入成绩（admin）
GRADE_RESPONSE=$(curl -s -X POST "$BASE_URL/grades" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"studentId":"S2001","courseId":"TEST200","score":92}')

if [ -z "$(echo $GRADE_RESPONSE | grep -o '"id":"')" ]; then
  echo "Create grade failed: $GRADE_RESPONSE"
  exit 1
fi

# 5. 创建学生账号并绑定 studentId
USER_RESPONSE=$(curl -s -X POST "$BASE_URL/users" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"username":"student2","password":"student123","role":"student","name":"学生二","studentId":"S2001"}')

if [ -z "$(echo $USER_RESPONSE | grep -o '"id":"')" ]; then
  echo "Create user failed: $USER_RESPONSE"
  exit 1
fi

# 6. 学生登录
ST_LOGIN=$(curl -s -X POST "$BASE_URL/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"username":"student2","password":"student123","role":"student"}')
ST_TOKEN=$(echo $ST_LOGIN | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
if [ -z "$ST_TOKEN" ]; then
  echo "Student login failed: $ST_LOGIN"
  exit 1
fi

# 7. 学生获取自己的成绩
MY_GRADES=$(curl -s -X GET "$BASE_URL/grades" \
  -H "Authorization: Bearer $ST_TOKEN")

echo "学生查看成绩响应: $MY_GRADES"

# 清理（可选）
# 删除用户
# DELETE /users/:id
USER_ID=$(echo $USER_RESPONSE | grep -o '"id":"[^"]*"' | cut -d'"' -f4)
if [ -n "$USER_ID" ]; then
  curl -s -X DELETE "$BASE_URL/users/$USER_ID" -H "Authorization: Bearer $TOKEN" > /dev/null
fi
# 删除成绩
GRADE_ID=$(echo $GRADE_RESPONSE | grep -o '"id":"[^"]*"' | cut -d'"' -f4)
if [ -n "$GRADE_ID" ]; then
  curl -s -X DELETE "$BASE_URL/grades/$GRADE_ID" -H "Authorization: Bearer $TOKEN" > /dev/null
fi
# 删除课程
if [ -n "$COURSE_ID" ]; then
  curl -s -X DELETE "$BASE_URL/courses/$COURSE_ID" -H "Authorization: Bearer $TOKEN" > /dev/null
fi
# 删除学生
if [ -n "$STUDENT_ID" ]; then
  curl -s -X DELETE "$BASE_URL/students/$STUDENT_ID" -H "Authorization: Bearer $TOKEN" > /dev/null
fi

echo "测试完成"