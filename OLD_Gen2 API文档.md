# 学生成绩管理系统 API 文档

## 项目概述

这是一个基于 C++ 和 Crow 框架开发的学生成绩管理系统后端服务。系统提供了完整的用户管理、学生管理、课程管理、成绩管理、统计分析、报表生成和系统管理功能。

**服务地址**: `http://localhost:21180/api`

**默认管理员账号**:
- 用户名: `admin`
- 密码: `admin123`
- 角色: `admin`

**默认教师账号**:
- 用户名: `teacher`
- 密码: `teacher123`
- 角色: `teacher`

**默认学生账号**:
- 用户名: `student`
- 密码: `student123`
- 角色: `student`

## 认证机制

所有需要认证的 API 请求必须在 HTTP Header 中包含 `Authorization` 字段，格式为：
```
Authorization: Bearer <token>
```

Token 通过登录接口获取，有效期为 24 小时。

## 通用响应格式

### 成功响应
```json
{
  "data": {...},
  "message": "操作成功"
}
```

### 错误响应
```json
{
  "error": "错误类型",
  "message": "错误描述"
}
```

### 分页响应
```json
{
  "data": [...],
  "total": 100,
  "page": 1,
  "limit": 10
}
```

## API 接口列表

### 一、认证相关

#### 1. 用户登录
- **路径**: `POST /api/auth/login`
- **描述**: 用户登录，获取 Token
- **请求体**:
```json
{
  "username": "admin",
  "password": "admin123",
  "role": "admin"
}
```
- **响应**:
```json
{
  "token": "xxx",
  "user": {
    "id": "xxx",
    "username": "admin",
    "role": "admin",
    "name": "管理员"
  }
}
```

#### 2. 用户登出
- **路径**: `POST /api/auth/logout`
- **描述**: 用户登出，使 Token 失效
- **Header**: `Authorization: Bearer <token>`
- **响应**: `{"message": "Logged out successfully"}`

#### 3. 验证 Token
- **路径**: `GET /api/auth/verify`
- **描述**: 验证 Token 是否有效
- **Header**: `Authorization: Bearer <token>`
- **响应**: `{"message": "Token valid"}`

### 二、用户管理

#### 4. 获取当前用户信息
- **路径**: `GET /api/user/profile`
- **描述**: 获取当前登录用户的信息
- **Header**: `Authorization: Bearer <token>`
- **响应**:
```json
{
  "id": "xxx",
  "username": "admin",
  "role": "admin",
  "name": "管理员",
  "class": "计算机2401"
}
```

#### 5. 修改密码
- **路径**: `PUT /api/user/password`
- **描述**: 修改当前用户密码
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "oldPassword": "old123",
  "newPassword": "new123"
}
```
- **响应**: `{"message": "Password changed successfully"}`

#### 6. 获取用户操作日志
- **路径**: `GET /api/user/logs`
- **描述**: 获取当前用户的操作日志
- **Header**: `Authorization: Bearer <token>`
- **响应**: 分页格式，包含操作日志数组

#### 7. 获取用户列表（管理员）
- **路径**: `GET /api/users`
- **描述**: 获取所有用户列表（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **响应**: 分页格式，包含用户数组

#### 8. 创建用户（管理员）
- **路径**: `POST /api/users`
- **描述**: 创建新用户
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "username": "newuser",
  "password": "password123",
  "role": "teacher",
  "name": "张老师",
  "class": "计算机2401"
}
```
- **响应**: 创建的用户信息

#### 9. 更新用户（管理员）
- **路径**: `PUT /api/users/<id>`
- **描述**: 更新用户信息
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "name": "新姓名",
  "role": "student",
  "class": "计算机2402"
}
```
- **响应**: 更新后的用户信息

#### 10. 删除用户（管理员）
- **路径**: `DELETE /api/users/<id>`
- **描述**: 删除用户
- **Header**: `Authorization: Bearer <token>`
- **响应**: `{"message": "User deleted successfully"}`

#### 11. 批量导入用户（管理员）
- **路径**: `POST /api/users/batch`
- **描述**: 批量导入用户
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "users": [
    {"username": "user1", "password": "pass1", "role": "student", "name": "学生1"},
    {"username": "user2", "password": "pass2", "role": "student", "name": "学生2"}
  ]
}
```
- **响应**: `{"success": 2, "failed": 0, "message": "..."}`

#### 12. 批量删除用户（管理员）
- **路径**: `DELETE /api/users/batch`
- **描述**: 批量删除用户
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "ids": ["id1", "id2"]
}
```
- **响应**: `{"success": 2, "failed": 0}`

#### 13. 重置密码（管理员）
- **路径**: `PUT /api/users/<id>/reset-password`
- **描述**: 重置指定用户的密码
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "newPassword": "newpass123"
}
```
- **响应**: `{"message": "Password reset successfully"}`

### 三、学生管理

#### 14. 获取学生列表
- **路径**: `GET /api/students`
- **描述**: 获取学生列表
- **Header**: `Authorization: Bearer <token>`
- **响应**: 分页格式，包含学生数组

#### 15. 获取学生详情
- **路径**: `GET /api/students/<id>`
- **描述**: 获取指定学生的详细信息
- **Header**: `Authorization: Bearer <token>`
- **响应**: 学生信息对象

#### 16. 添加学生
- **路径**: `POST /api/students`
- **描述**: 添加新学生（管理员/教师权限）
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "studentId": "2024001",
  "name": "李明",
  "class": "计算机2401",
  "gender": "男",
  "phone": "13800138000",
  "email": "liming@example.com"
}
```
- **响应**: 创建的学生信息

#### 17. 更新学生
- **路径**: `PUT /api/students/<id>`
- **描述**: 更新学生信息（管理员/教师权限）
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "name": "李明",
  "class": "计算机2402",
  "phone": "13800138001"
}
```
- **响应**: 更新后的学生信息

#### 18. 删除学生
- **路径**: `DELETE /api/students/<id>`
- **描述**: 删除学生（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **响应**: `{"message": "Student deleted successfully"}`

#### 19. 批量导入学生
- **路径**: `POST /api/students/batch`
- **描述**: 批量导入学生数据
- **Header**: `Authorization: Bearer <token>`
- **请求体**: 学生数组
```json
[
  {"studentId": "2024001", "name": "学生1", "class": "计算机2401"},
  {"studentId": "2024002", "name": "学生2", "class": "计算机2401"}
]
```
- **响应**: `{"success": 2, "failed": 0, "message": "..."}`

#### 20. 导出学生数据
- **路径**: `GET /api/students/export`
- **描述**: 导出学生数据（返回 JSON）
- **Header**: `Authorization: Bearer <token>`
- **响应**: 学生数组

#### 21. 获取学生成绩概览
- **路径**: `GET /api/students/<studentId>/grades`
- **描述**: 获取指定学生的成绩概览
- **Header**: `Authorization: Bearer <token>`
- **响应**:
```json
{
  "totalCourses": 5,
  "avgScore": 85.5,
  "passRate": 100.0,
  "totalScore": 427,
  "recentGrades": [
    {"courseName": "数学", "score": 90, "semester": "2024-1"}
  ]
}
```

### 四、课程管理

#### 22. 获取课程列表
- **路径**: `GET /api/courses`
- **描述**: 获取课程列表
- **Header**: `Authorization: Bearer <token>`
- **响应**: 分页格式，包含课程数组

#### 23. 获取课程详情
- **路径**: `GET /api/courses/<id>`
- **描述**: 获取指定课程的详细信息
- **Header**: `Authorization: Bearer <token>`
- **响应**: 课程信息对象

#### 24. 添加课程
- **路径**: `POST /api/courses`
- **描述**: 添加新课程（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "courseId": "CS101",
  "name": "计算机基础",
  "credit": 3,
  "teacher": "张老师",
  "description": "计算机科学导论"
}
```
- **响应**: 创建的课程信息

#### 25. 更新课程
- **路径**: `PUT /api/courses/<id>`
- **描述**: 更新课程信息（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "name": "计算机基础（修订版）",
  "credit": 4
}
```
- **响应**: 更新后的课程信息

#### 26. 删除课程
- **路径**: `DELETE /api/courses/<id>`
- **描述**: 删除课程（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **响应**: `{"message": "Course deleted successfully"}`

#### 27. 获取选课学生列表
- **路径**: `GET /api/courses/<courseId>/students`
- **描述**: 获取选修指定课程的学生列表
- **Header**: `Authorization: Bearer <token>`
- **响应**: 学生数组，包含学号、姓名、班级和成绩

### 五、成绩管理

#### 28. 获取成绩列表
- **路径**: `GET /api/grades`
- **描述**: 获取成绩列表
- **Header**: `Authorization: Bearer <token>`
- **响应**: 分页格式，包含成绩数组

#### 29. 录入成绩
- **路径**: `POST /api/grades`
- **描述**: 录入成绩（管理员/教师权限）
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "studentId": "2024001",
  "courseId": "CS101",
  "score": 95,
  "semester": "2024-1"
}
```
- **响应**: 创建的成绩信息

#### 30. 更新成绩
- **路径**: `PUT /api/grades/<id>`
- **描述**: 更新成绩（管理员/教师权限）
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "score": 98
}
```
- **响应**: 更新后的成绩信息

#### 31. 删除成绩
- **路径**: `DELETE /api/grades/<id>`
- **描述**: 删除成绩（管理员/教师权限）
- **Header**: `Authorization: Bearer <token>`
- **响应**: `{"message": "Grade deleted successfully"}`

#### 32. 批量导入成绩
- **路径**: `POST /api/grades/batch`
- **描述**: 批量导入成绩
- **Header**: `Authorization: Bearer <token>`
- **请求体**: 成绩数组
```json
[
  {"studentId": "2024001", "courseId": "CS101", "score": 90, "semester": "2024-1"},
  {"studentId": "2024002", "courseId": "CS101", "score": 85, "semester": "2024-1"}
]
```
- **响应**: `{"success": 2, "failed": 0, "message": "..."}`

#### 33. 导出成绩数据
- **路径**: `GET /api/grades/export`
- **描述**: 导出成绩数据（返回 JSON）
- **Header**: `Authorization: Bearer <token>`
- **响应**: 成绩数组

#### 34. 获取课程成绩列表
- **路径**: `GET /api/grades/course/<courseId>`
- **描述**: 获取指定课程的所有成绩
- **Header**: `Authorization: Bearer <token>`
- **响应**: 成绩数组，包含学生信息和分数

#### 35. 批量更新成绩
- **路径**: `POST /api/grades/batch-update`
- **描述**: 批量更新指定课程的成绩
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "courseId": "CS101",
  "semester": "2024-1",
  "grades": [
    {"studentId": "2024001", "score": 95},
    {"studentId": "2024002", "score": 88}
  ]
}
```
- **响应**: `{"success": 2, "failed": 0}`

### 六、统计分析

#### 36. 获取统计概览
- **路径**: `GET /api/statistics/overview`
- **描述**: 获取系统统计概览
- **Header**: `Authorization: Bearer <token>`
- **响应**:
```json
{
  "avgScore": 85.5,
  "passRate": 92.3,
  "totalStudents": 100,
  "totalCourses": 10,
  "totalGrades": 500
}
```

#### 37. 按班级统计
- **路径**: `GET /api/statistics/class`
- **描述**: 按班级统计成绩
- **Header**: `Authorization: Bearer <token>`
- **响应**: 班级统计数组，包含平均分、及格率、前三名等

#### 38. 按课程统计
- **路径**: `GET /api/statistics/course`
- **描述**: 按课程统计成绩
- **Header**: `Authorization: Bearer <token>`
- **响应**: 课程统计信息，包含平均分、及格率、最高分、最低分等

#### 39. 获取排名列表
- **路径**: `GET /api/statistics/ranking`
- **描述**: 获取学生成绩排名
- **Header**: `Authorization: Bearer <token>`
- **响应**: 排名数组，包含学生信息和成绩

#### 40. 获取成绩分布
- **路径**: `GET /api/statistics/distribution`
- **描述**: 获取成绩分布情况
- **Header**: `Authorization: Bearer <token>`
- **响应**: 分数段分布数组，包含各分数段的人数和百分比

#### 41. 生成统计报表
- **路径**: `GET /api/statistics/report`
- **描述**: 生成统计报表
- **Header**: `Authorization: Bearer <token>`
- **响应**: 统计报表数据

### 七、报表管理

#### 42. 生成成绩单
- **路径**: `GET /api/reports/report-card`
- **描述**: 生成成绩单（返回 HTML）
- **Header**: `Authorization: Bearer <token>`
- **响应**: HTML 格式的成绩单

#### 43. 生成统计报表
- **路径**: `GET /api/reports/statistics`
- **描述**: 生成统计报表
- **Header**: `Authorization: Bearer <token>`
- **响应**: 报表生成结果

#### 44. 打印准备
- **路径**: `POST /api/reports/print`
- **描述**: 准备打印数据
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "type": "report-card",
  "data": {...}
}
```
- **响应**: 打印用的 HTML

#### 45. 批量打印
- **路径**: `POST /api/reports/batch-print`
- **描述**: 批量打印准备
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "type": "report-card",
  "items": ["id1", "id2"]
}
```
- **响应**: `{"success": 2, "failed": 0}`

### 八、系统管理

#### 46. 创建备份
- **路径**: `POST /api/system/backup`
- **描述**: 创建数据备份（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **响应**: 备份信息对象

#### 47. 获取备份列表
- **路径**: `GET /api/system/backups`
- **描述**: 获取备份列表（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **响应**: 备份数组

#### 48. 恢复备份
- **路径**: `POST /api/system/restore`
- **描述**: 恢复数据备份（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "backupId": "xxx"
}
```
- **响应**: `{"message": "Backup restored successfully"}`

#### 49. 删除备份
- **路径**: `DELETE /api/system/backups/<backupId>`
- **描述**: 删除备份（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **响应**: `{"message": "Backup deleted successfully"}`

#### 50. 获取系统日志
- **路径**: `GET /api/system/logs`
- **描述**: 获取系统日志（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **响应**: 分页格式，包含系统日志数组

#### 51. 获取系统设置
- **路径**: `GET /api/system/settings`
- **描述**: 获取系统设置（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **响应**: 系统设置对象

#### 52. 更新系统设置
- **路径**: `PUT /api/system/settings`
- **描述**: 更新系统设置（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **请求体**:
```json
{
  "backupInterval": 7,
  "logRetentionDays": 30,
  "maxLoginAttempts": 5,
  "sessionTimeout": 30
}
```
- **响应**: `{"message": "Settings updated successfully"}`

#### 53. 清理日志
- **路径**: `POST /api/system/clean-logs`
- **描述**: 清理过期日志（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **响应**: `{"message": "Logs cleaned successfully"}`

#### 54. 导出日志
- **路径**: `GET /api/system/export-logs`
- **描述**: 导出系统日志（管理员权限）
- **Header**: `Authorization: Bearer <token>`
- **响应**: 日志数组（JSON 格式）

### 九、测试接口

#### 55. 健康检查
- **路径**: `GET /api/health`
- **描述**: 服务健康检查
- **响应**: `{"status": "ok", "message": "Server is running"}`

## 使用示例

### 1. 完整的登录和获取数据流程

```bash
# 1. 登录获取 Token
curl -X POST http://localhost:21180/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "admin123", "role": "admin"}'

# 响应: {"token": "xxx", "user": {...}}

# 2. 使用 Token 获取学生列表
curl -X GET http://localhost:21180/api/students \
  -H "Authorization: Bearer xxx"

# 3. 添加新学生
curl -X POST http://localhost:21180/api/students \
  -H "Authorization: Bearer xxx" \
  -H "Content-Type: application/json" \
  -d '{"studentId": "2024001", "name": "李明", "class": "计算机2401"}'

# 4. 录入成绩
curl -X POST http://localhost:21180/api/grades \
  -H "Authorization: Bearer xxx" \
  -H "Content-Type: application/json" \
  -d '{"studentId": "2024001", "courseId": "CS101", "score": 95, "semester": "2024-1"}'

# 5. 查看统计概览
curl -X GET http://localhost:21180/api/statistics/overview \
  -H "Authorization: Bearer xxx"
```

### 2. 使用 JavaScript/TypeScript 调用

```javascript
// 登录
const login = async (username, password, role) => {
  const response = await fetch('http://localhost:21180/api/auth/login', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ username, password, role })
  });
  const data = await response.json();
  return data.token; // 保存 token
};

// 获取数据（需要 token）
const getStudents = async (token) => {
  const response = await fetch('http://localhost:21180/api/students', {
    headers: { 'Authorization': `Bearer ${token}` }
  });
  return await response.json();
};

// 添加数据
const addStudent = async (token, studentData) => {
  const response = await fetch('http://localhost:21180/api/students', {
    method: 'POST',
    headers: {
      'Authorization': `Bearer ${token}`,
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(studentData)
  });
  return await response.json();
};
```

## 权限说明

| 角色 | 权限范围 |
|------|----------|
| **admin** | 所有功能 |
| **teacher** | 查看所有数据，录入/修改成绩，管理学生信息 |
| **student** | 查看自己的信息和成绩 |

## 数据模型

### 用户 (User)
```json
{
  "id": "唯一ID",
  "username": "用户名",
  "role": "admin/teacher/student",
  "name": "姓名",
  "class": "班级",
  "createdAt": "创建时间",
  "updatedAt": "更新时间"
}
```

### 学生 (Student)
```json
{
  "id": "唯一ID",
  "studentId": "学号",
  "name": "姓名",
  "class": "班级",
  "gender": "性别",
  "phone": "电话",
  "email": "邮箱",
  "createdAt": "创建时间",
  "updatedAt": "更新时间"
}
```

### 课程 (Course)
```json
{
  "id": "唯一ID",
  "courseId": "课程编号",
  "name": "课程名称",
  "credit": "学分",
  "teacher": "教师",
  "description": "描述",
  "createdAt": "创建时间",
  "updatedAt": "更新时间"
}
```

### 成绩 (Grade)
```json
{
  "id": "唯一ID",
  "studentId": "学号",
  "studentName": "学生姓名",
  "courseId": "课程编号",
  "courseName": "课程名称",
  "score": "分数",
  "semester": "学期",
  "createdAt": "创建时间",
  "updatedAt": "更新时间"
}
```

## 注意事项

1. **Token 过期时间**: 24 小时
2. **密码存储**: 使用 SHA256 哈希存储
3. **数据持久化**: 使用 JSON 文件存储
4. **并发控制**: 使用互斥锁保证数据一致性
5. **日志记录**: 所有操作都会记录到操作日志和系统日志
6. **备份机制**: 支持手动备份和恢复
7. **数据验证**: 包括邮箱格式、手机号格式、成绩范围等验证

## 错误码说明

| 状态码 | 说明 |
|--------|------|
| 200 | 成功 |
| 201 | 创建成功 |
| 400 | 请求参数错误 |
| 401 | 未授权（Token缺失或无效） |
| 403 | 禁止访问（权限不足） |
| 404 | 资源不存在 |
| 409 | 冲突（数据已存在） |
| 500 | 服务器内部错误 |

## 部署说明

1. 确保系统已安装 C++ 编译器（支持 C++17）
2. 安装依赖库：
   - Crow (Web 框架)
   - nlohmann/json (JSON 库)
   - OpenSSL (加密库)
3. 编译项目：
   ```bash
   clang++ -std=c++17 -I/opt/homebrew/include -I/opt/homebrew/Cellar/crow/1.3.0/include main.cpp -o main -lssl -lcrypto
   ```
4. 运行服务：
   ```bash
   ./main
   ```
5. 服务将在 `http://localhost:21180` 启动

## 数据文件说明

所有数据存储在 `./data/` 目录下：
- `users.json` - 用户数据
- `students.json` - 学生数据
- `courses.json` - 课程数据
- `grades.json` - 成绩数据
- `operation_logs.json` - 操作日志
- `system_logs.json` - 系统日志
- `settings.json` - 系统设置
- `tokens.json` - Token 数据
- `backups.json` - 备份信息
- `backups/` - 备份文件目录