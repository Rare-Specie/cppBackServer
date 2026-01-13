# 学生成绩管理系统 API 文档

## 基本信息

- **服务器地址**: `http://localhost:21180`
- **API 基础路径**: `/api`
- **认证方式**: Bearer Token
- **数据格式**: JSON

## 认证相关

### 1. 用户登录
**POST** `/api/auth/login`

**请求体**:
```json
{
    "username": "string",
    "password": "string",
    "role": "admin|teacher|student"
}
```

**响应**:
- 成功 (200):
```json
{
    "token": "jwt_token_string",
    "user": {
        "id": "user_id",
        "username": "string",
        "role": "string",
        "name": "string",
        "class": "string",
        "studentId": "string",
        "createdAt": "ISO8601",
        "updatedAt": "ISO8601"
    }
}
```

- 失败 (401):
```json
{
    "error": "Unauthorized",
    "message": "Invalid credentials"
}
```

### 2. 用户登出
**POST** `/api/auth/logout`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
- 成功 (200):
```json
{
    "message": "Logged out successfully"
}
```

### 3. 验证Token
**GET** `/api/auth/verify`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
- 成功 (200):
```json
{
    "message": "Token valid"
}
```

## 用户管理

### 4. 获取当前用户信息
**GET** `/api/user/profile`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
- 成功 (200): 用户对象

### 5. 修改密码
**PUT** `/api/user/password`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "oldPassword": "string",
    "newPassword": "string"
}
```

### 6. 获取操作日志
**GET** `/api/user/logs`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
- 成功 (200): 日志数组

### 7. 获取用户列表（管理员）
**GET** `/api/users`

**请求头**:
```
Authorization: Bearer {token}
X-Page: 1          // 页码（可选，默认1）
X-Limit: 10        // 每页数量（可选，默认10，最大1000）
X-Query-Role:      // 角色过滤（可选）
X-Query-Search:    // 搜索关键字（可选）
```

**响应**:
```json
{
    "data": [用户对象数组],
    "total": 100,
    "page": 1,
    "limit": 10,
    "totalPages": 10
}
```

### 8. 创建用户（管理员）
**POST** `/api/users`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "username": "string",
    "password": "string",
    "role": "admin|teacher|student",
    "name": "string",
    "class": "string",        // 可选
    "studentId": "string"     // 仅当role=student时可选
}
```

**响应**:
- 成功 (201): 用户对象

### 9. 更新用户（管理员）
**PUT** `/api/users/{id}`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体** (部分字段可选):
```json
{
    "name": "string",
    "class": "string",
    "role": "admin|teacher|student",
    "studentId": "string"
}
```

### 10. 删除用户（管理员）
**DELETE** `/api/users/{id}`

**请求头**:
```
Authorization: Bearer {token}
```

### 11. 批量导入用户（管理员）
**POST** `/api/users/batch`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
// 格式1：直接数组
[
    {
        "username": "string",
        "password": "string",
        "role": "string",
        "name": "string"
    }
]

// 格式2：对象包装
{
    "users": [
        {
            "username": "string",
            "password": "string",
            "role": "string",
            "name": "string"
        }
    ]
}
```

**响应**:
```json
{
    "success": 5,
    "failed": 2,
    "errors": [
        {
            "index": 0,
            "error": "Username already exists"
        }
    ]
}
```

### 12. 批量删除用户（管理员）
**DELETE** `/api/users/batch`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "ids": ["user_id_1", "user_id_2"]
}
```

### 13. 重置密码（管理员）
**PUT** `/api/users/{id}/reset-password`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "newPassword": "string"
}
```

## 学生管理

### 14. 获取学生列表
**GET** `/api/students`

**请求头**:
```
Authorization: Bearer {token}
X-Page: 1
X-Limit: 10
X-Query-Class:     // 班级过滤（可选）
X-Query-Search:    // 搜索关键字（可选）
X-Fields:          // 字段选择（可选，逗号分隔）
```

**响应**:
```json
{
    "data": [
        {
            "id": "string",
            "studentId": "string",
            "name": "string",
            "class": "string",
            "gender": "string",
            "phone": "string",
            "email": "string",
            "createdAt": "ISO8601",
            "updatedAt": "ISO8601"
        }
    ],
    "total": 100,
    "page": 1,
    "limit": 10,
    "totalPages": 10
}
```

### 15. 获取学生详情
**GET** `/api/students/{id}`

**请求头**:
```
Authorization: Bearer {token}
```

### 16. 添加学生
**POST** `/api/students`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "studentId": "string",
    "name": "string",
    "class": "string",
    "gender": "string",    // 可选
    "phone": "string",     // 可选
    "email": "string"      // 可选
}
```

### 17. 更新学生
**PUT** `/api/students/{id}`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体** (部分字段可选):
```json
{
    "name": "string",
    "class": "string",
    "gender": "string",
    "phone": "string",
    "email": "string"
}
```

### 18. 删除学生
**DELETE** `/api/students/{id}`

**请求头**:
```
Authorization: Bearer {token}
```

### 19. 批量导入学生
**POST** `/api/students/batch`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
// 格式1：直接数组
[
    {
        "studentId": "string",
        "name": "string",
        "class": "string"
    }
]

// 格式2：对象包装
{
    "students": [...]
}
```

**响应**:
```json
{
    "success": 10,
    "failed": 2,
    "errors": [
        {
            "index": 1,
            "error": "StudentId already exists"
        }
    ]
}
```

### 20. 导出学生数据
**GET** `/api/students/export`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
- 成功 (200): JSON数组格式的学生数据

### 21. 获取学生成绩概览
**GET** `/api/students/{id}/grades`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
```json
{
    "totalCourses": 5,
    "avgScore": 85.6,
    "passRate": 100.0,
    "totalScore": 428,
    "recentGrades": [
        {
            "courseName": "数学",
            "score": 90
        }
    ]
}
```

## 课程管理

### 22. 获取课程列表
**GET** `/api/courses`

**请求头**:
```
Authorization: Bearer {token}
X-Page: 1
X-Limit: 10
X-Query-Search:    // 搜索关键字（可选）
```

**响应**:
```json
{
    "data": [
        {
            "id": "string",
            "courseId": "string",
            "name": "string",
            "credit": 3,
            "teacher": "string",
            "description": "string",
            "createdAt": "ISO8601",
            "updatedAt": "ISO8601"
        }
    ],
    "total": 50,
    "page": 1,
    "limit": 10,
    "totalPages": 5
}
```

### 23. 获取课程详情
**GET** `/api/courses/{id}`

**请求头**:
```
Authorization: Bearer {token}
```

### 24. 添加课程
**POST** `/api/courses`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "courseId": "string",
    "name": "string",
    "credit": 3,
    "teacher": "string",    // 可选
    "description": "string" // 可选
}
```

### 25. 更新课程
**PUT** `/api/courses/{id}`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体** (部分字段可选):
```json
{
    "name": "string",
    "credit": 4,
    "teacher": "string",
    "description": "string"
}
```

### 26. 删除课程
**DELETE** `/api/courses/{id}`

**请求头**:
```
Authorization: Bearer {token}
```

### 27. 获取选课学生列表
**GET** `/api/courses/{id}/students`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
```json
{
    "course": {
        "courseId": "string",
        "name": "string"
    },
    "students": [
        {
            "studentId": "string",
            "name": "string",
            "class": "string"
        }
    ]
}
```

### 28. 学生选课
**POST** `/api/courses/{id}/enroll`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "studentId": "string"
}
```

### 29. 取消选课
**DELETE** `/api/courses/{id}/enroll/{studentId}`

**请求头**:
```
Authorization: Bearer {token}
```

## 成绩管理

### 30. 获取成绩列表
**GET** `/api/grades`

**请求头**:
```
Authorization: Bearer {token}
X-Page: 1
X-Limit: 10
X-Query-StudentId:    // 学生ID过滤（可选）
X-Query-CourseId:     // 课程ID过滤（可选）
X-Query-Class:        // 班级过滤（可选）
X-Fields:             // 字段选择（可选）
```

**响应**:
```json
{
    "data": [
        {
            "id": "string",
            "studentId": "string",
            "studentName": "string",
            "courseId": "string",
            "courseName": "string",
            "score": 85,
            "createdAt": "ISO8601",
            "updatedAt": "ISO8601"
        }
    ],
    "total": 100,
    "page": 1,
    "limit": 10,
    "totalPages": 10
}
```

### 31. 录入成绩
**POST** `/api/grades`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "studentId": "string",
    "courseId": "string",
    "score": 85
}
```

### 32. 更新成绩
**PUT** `/api/grades/{id}`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体** (部分字段可选):
```json
{
    "score": 90
}
```

### 33. 删除成绩
**DELETE** `/api/grades/{id}`

**请求头**:
```
Authorization: Bearer {token}
```

### 34. 批量导入成绩
**POST** `/api/grades/batch`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "courseId": "string",
    "grades": [
        {
            "studentId": "string",
            "score": 85
        }
    ]
}
```

**响应**:
```json
{
    "success": 30,
    "failed": 2,
    "errors": [
        {
            "index": 0,
            "error": "Student not found: S001"
        }
    ]
}
```

### 35. 导出成绩数据
**GET** `/api/grades/export`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
- 成功 (200): JSON数组格式的成绩数据

### 36. 获取课程成绩列表
**GET** `/api/grades/course/{courseId}`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
```json
{
    "course": {
        "courseId": "string",
        "name": "string"
    },
    "grades": [
        {
            "studentId": "string",
            "studentName": "string",
            "score": 85
        }
    ]
}
```

### 37. 批量更新成绩
**POST** `/api/grades/batch-update`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "courseId": "string",
    "grades": [
        {
            "studentId": "string",
            "score": 90
        }
    ]
}
```

## 统计分析

### 38. 获取统计概览
**GET** `/api/statistics/overview`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
```json
{
    "avgScore": 85.6,
    "passRate": 92.5,
    "totalStudents": 500,
    "totalCourses": 50,
    "totalGrades": 2500
}
```

### 39. 按班级统计
**GET** `/api/statistics/class`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
```json
{
    "data": [
        {
            "class": "计算机2021-1班",
            "studentCount": 45,
            "avgScore": 87.2,
            "passRate": 95.6,
            "topStudent": {
                "studentId": "S001",
                "name": "张三",
                "avgScore": 95.0
            }
        }
    ]
}
```

### 40. 按课程统计
**GET** `/api/statistics/course`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
```json
{
    "data": [
        {
            "courseId": "C001",
            "courseName": "高等数学",
            "studentCount": 100,
            "avgScore": 82.5,
            "passRate": 88.0,
            "maxScore": 98,
            "minScore": 45
        }
    ]
}
```

### 41. 获取排名列表
**GET** `/api/statistics/ranking`

**请求头**:
```
Authorization: Bearer {token}
X-Query-Class:    // 班级过滤（可选）
X-Query-Limit:    // 前N名（可选，默认10）
```

**响应**:
```json
{
    "data": [
        {
            "rank": 1,
            "studentId": "S001",
            "name": "张三",
            "class": "计算机2021-1班",
            "avgScore": 95.0,
            "totalScore": 475
        }
    ]
}
```

### 42. 获取成绩分布
**GET** `/api/statistics/distribution`

**请求头**:
```
Authorization: Bearer {token}
X-Query-CourseId:    // 课程过滤（可选）
X-Query-Class:       // 班级过滤（可选）
```

**响应**:
```json
{
    "data": [
        {
            "range": "90-100",
            "count": 15,
            "percentage": 30.0
        },
        {
            "range": "80-89",
            "count": 20,
            "percentage": 40.0
        }
    ]
}
```

### 43. 生成统计报表
**GET** `/api/statistics/report`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
```json
{
    "summary": {
        "totalStudents": 500,
        "totalCourses": 50,
        "avgScore": 85.6,
        "passRate": 92.5
    },
    "classStatistics": [...],
    "courseStatistics": [...],
    "topStudents": [...]
}
```

## 报表管理

### 44. 生成成绩单
**GET** `/api/reports/report-card`

**请求头**:
```
Authorization: Bearer {token}
```

**查询参数** (通过URL参数):
```
?studentId=S001          // 单个学生
或
?class=计算机2021-1班     // 整个班级
```

**响应**:
```json
{
    "data": [
        {
            "studentId": "S001",
            "studentName": "张三",
            "className": "计算机2021-1班",
            "grades": [
                {
                    "courseId": "C001",
                    "courseName": "高等数学",
                    "score": 90
                }
            ]
        }
    ]
}
```

### 45. 生成统计报表
**GET** `/api/reports/statistics`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
- 成功 (200): 统计报表数据

### 46. 打印准备
**POST** `/api/reports/print`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "type": "reportCard|statistics",
    "data": {...}
}
```

**响应**:
```json
{
    "printId": "print_123",
    "status": "ready",
    "pages": 5
}
```

### 47. 批量打印
**POST** `/api/reports/batch-print`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "type": "reportCard",
    "items": ["S001", "S002", "S003"]
}
```

## 系统管理

### 48. 创建备份
**POST** `/api/system/backup`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
```json
{
    "id": "backup_123",
    "name": "backup_1736832000.zip",
    "createdBy": "admin",
    "createdAt": "2025-01-14T10:00:00Z",
    "size": 1024000
}
```

### 49. 获取备份列表
**GET** `/api/system/backups`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
```json
[
    {
        "id": "backup_123",
        "name": "backup_1736832000.zip",
        "createdBy": "admin",
        "createdAt": "2025-01-14T10:00:00Z",
        "size": 1024000
    }
]
```

### 50. 恢复备份
**POST** `/api/system/restore`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "backupId": "backup_123"
}
```

### 51. 删除备份
**DELETE** `/api/system/backups/{backupId}`

**请求头**:
```
Authorization: Bearer {token}
```

### 52. 获取系统日志
**GET** `/api/system/logs`

**请求头**:
```
Authorization: Bearer {token}
X-Page: 1
X-Limit: 50
```

**响应**:
```json
{
    "data": [
        {
            "id": "log_123",
            "level": "INFO",
            "message": "System started",
            "timestamp": "2025-01-14T10:00:00Z"
        }
    ],
    "total": 100,
    "page": 1,
    "limit": 50,
    "totalPages": 2
}
```

### 53. 获取系统设置
**GET** `/api/system/settings`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
```json
{
    "backupInterval": 24,
    "logRetentionDays": 30,
    "maxUploadSize": 10485760
}
```

### 54. 更新系统设置
**PUT** `/api/system/settings`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "backupInterval": 24,
    "logRetentionDays": 30,
    "maxUploadSize": 10485760
}
```

### 55. 清理日志
**POST** `/api/system/clean-logs`

**请求头**:
```
Authorization: Bearer {token}
```

**请求体**:
```json
{
    "days": 30
}
```

### 56. 导出日志
**GET** `/api/system/export-logs`

**请求头**:
```
Authorization: Bearer {token}
```

**响应**:
- 成功 (200): 日志文件或JSON数据

## 通用响应格式

### 成功响应
```json
{
    "data": {...},
    "message": "操作成功"
}
```

### 分页响应
```json
{
    "data": [...],
    "total": 100,
    "page": 1,
    "limit": 10,
    "totalPages": 10
}
```

### 错误响应
```json
{
    "error": "错误类型",
    "message": "错误描述"
}
```

**常见错误类型**:
- `Unauthorized`: 未授权（401）
- `Forbidden`: 禁止访问（403）
- `NotFound`: 资源不存在（404）
- `BadRequest`: 请求错误（400）
- `Conflict`: 冲突（409）
- `InternalError`: 服务器内部错误（500）

## 权限说明

| 角色 | 权限范围 |
|------|----------|
| **admin** | 所有功能 |
| **teacher** | 查看所有数据，录入/更新成绩，查看学生/课程信息 |
| **student** | 仅查看自己的信息和成绩 |

## 数据验证规则

- **手机号**: 11位数字
- **邮箱**: 标准邮箱格式
- **成绩**: 0-100分
- **用户名**: 唯一，不为空
- **密码**: 不为空（建议6位以上）
- **学号/课程号**: 唯一，不为空

## 日期格式

所有日期时间均使用 **ISO 8601** 格式：
```
2025-01-14T10:00:00Z
```

## 使用示例

### 1. 完整登录流程
```bash
# 1. 登录获取Token
curl -X POST http://localhost:21180/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin","role":"admin"}'

# 2. 使用Token访问API
curl -X GET http://localhost:21180/api/students \
  -H "Authorization: Bearer your_token_here"
```

### 2. 分页查询
```bash
curl -X GET "http://localhost:21180/api/students" \
  -H "Authorization: Bearer your_token_here" \
  -H "X-Page: 1" \
  -H "X-Limit: 20" \
  -H "X-Query-Class: 计算机2021-1班"
```

### 3. 批量操作
```bash
# 批量导入学生
curl -X POST http://localhost:21180/api/students/batch \
  -H "Authorization: Bearer your_token_here" \
  -H "Content-Type: application/json" \
  -d '[
    {"studentId":"S001","name":"张三","class":"计算机2021-1班"},
    {"studentId":"S002","name":"李四","class":"计算机2021-1班"}
  ]'
```

## 注意事项

1. **认证**: 所有API（除登录外）都需要在Header中携带 `Authorization: Bearer {token}`
2. **权限**: 不同角色有不同的访问权限，请确保用户角色正确
3. **分页**: 默认分页参数为 page=1, limit=10，最大limit=1000
4. **日期**: 所有返回的日期时间均为ISO 8601格式
5. **批量操作**: 批量导入/删除时，会返回成功和失败的数量及错误详情
6. **学生绑定**: 学生角色用户需要在用户记录中绑定studentId才能查看自己的成绩
7. **数据保护**: 删除操作不可恢复，请谨慎使用
8. **日志记录**: 所有重要操作都会被记录到操作日志中

## 默认账号

- **用户名**: admin
- **密码**: admin
- **角色**: admin

**建议**: 首次使用后立即修改默认密码！

## 服务器启动

```bash
# 编译
./build.sh

# 运行
./main

# 服务器将在 http://localhost:21180 启动
```

---

*文档版本: v1.0*  
*最后更新: 2025-01-14*