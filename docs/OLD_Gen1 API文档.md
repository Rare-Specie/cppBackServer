# 学生成绩管理系统 API 参考文档

## 概述

本文档描述了学生成绩管理系统的后端 API 接口规范。系统采用 RESTful 风格设计，所有接口统一以 `/api` 为前缀，使用 JSON 格式进行数据交换。

## 基础信息

- **Base URL**: `/api`
- **认证方式**: Bearer Token (JWT)
- **数据格式**: JSON
- **字符编码**: UTF-8

## 认证相关

### 1. 用户登录

**接口**: `POST /auth/login`

**请求参数**:
```json
{
  "username": "string",    // 用户名
  "password": "string",    // 密码
  "role": "admin|teacher|student"  // 角色
}
```

**响应示例**:
```json
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "user": {
    "id": "123",
    "username": "admin",
    "role": "admin",
    "name": "管理员",
    "class": "计算机2401"
  }
}
```

**错误响应**:
- 400: 参数错误
- 401: 用户名或密码错误
- 500: 服务器错误

---

### 2. 用户登出

**接口**: `POST /auth/logout`

**认证**: 需要

**响应**: 200 OK

---

### 3. 验证 Token

**接口**: `GET /auth/verify`

**认证**: 需要

**响应**: 200 OK (Token有效)

---

### 4. 获取用户信息

**接口**: `GET /user/profile`

**认证**: 需要

**响应示例**:
```json
{
  "id": "123",
  "username": "admin",
  "role": "admin",
  "name": "管理员",
  "class": "计算机2401",
  "createdAt": "2024-01-01T12:00:00Z",
  "updatedAt": "2024-01-01T12:00:00Z"
}
```

---

### 5. 修改密码

**接口**: `PUT /user/password`

**认证**: 需要

**请求参数**:
```json
{
  "oldPassword": "string",
  "newPassword": "string"
}
```

**响应**: 200 OK

---

### 6. 获取操作日志

**接口**: `GET /user/logs`

**认证**: 需要

**查询参数**:
- `page`: 页码 (默认: 1)
- `limit`: 每页数量 (默认: 10)
- `startTime`: 开始时间 (可选)
- `endTime`: 结束时间 (可选)

**响应示例**:
```json
{
  "data": [
    {
      "id": "1",
      "userId": "123",
      "username": "admin",
      "action": "登录",
      "module": "认证",
      "ip": "192.168.1.100",
      "createdAt": "2024-01-01T12:00:00Z"
    }
  ],
  "total": 100,
  "page": 1,
  "limit": 10
}
```

---

## 学生管理

### 1. 获取学生列表

**接口**: `GET /students`

**认证**: 需要

**查询参数**:
- `class`: 班级筛选 (可选)
- `search`: 搜索关键词 (学号/姓名) (可选)
- `page`: 页码 (默认: 1)
- `limit`: 每页数量 (默认: 10)

**响应示例**:
```json
{
  "data": [
    {
      "id": "1",
      "studentId": "2024001",
      "name": "张三",
      "class": "计算机2401",
      "gender": "male",
      "phone": "13800138000",
      "email": "zhangsan@example.com",
      "createdAt": "2024-01-01T12:00:00Z",
      "updatedAt": "2024-01-01T12:00:00Z"
    }
  ],
  "total": 50,
  "page": 1,
  "limit": 10
}
```

---

### 2. 获取学生详情

**接口**: `GET /students/{id}`

**认证**: 需要

**响应**: 单个学生对象

---

### 3. 添加学生

**接口**: `POST /students`

**认证**: 需要 (管理员/教师)

**请求参数**:
```json
{
  "studentId": "2024001",
  "name": "张三",
  "class": "计算机2401",
  "gender": "male",
  "phone": "13800138000",
  "email": "zhangsan@example.com"
}
```

**响应**: 201 Created + 学生对象

---

### 4. 更新学生

**接口**: `PUT /students/{id}`

**认证**: 需要 (管理员/教师)

**请求参数** (部分更新):
```json
{
  "name": "张三",
  "class": "计算机2402"
}
```

**响应**: 200 OK + 学生对象

---

### 5. 删除学生

**接口**: `DELETE /students/{id}`

**认证**: 需要 (管理员)

**响应**: 200 OK

---

### 6. 批量导入学生

**接口**: `POST /students/batch`

**认证**: 需要 (管理员)

**请求**: multipart/form-data, 字段名: `file`

**支持格式**: Excel (.xlsx, .xls) 或 CSV (.csv)

**响应示例**:
```json
{
  "success": 10,
  "failed": 2,
  "message": "导入完成：成功10条，失败2条"
}
```

---

### 7. 导出学生数据

**接口**: `GET /students/export`

**认证**: 需要

**查询参数**:
- `format`: `excel` 或 `csv` (默认: excel)

**响应**: 二进制文件流

---

### 8. 获取学生成绩概览

**接口**: `GET /students/{studentId}/grades`

**认证**: 需要

**响应示例**:
```json
{
  "totalCourses": 8,
  "avgScore": 85.5,
  "passRate": 95.0,
  "totalScore": 684,
  "recentGrades": [
    {
      "courseName": "程序设计基础",
      "score": 85,
      "semester": "2024-2025学年第一学期"
    }
  ]
}
```

---

## 课程管理

### 1. 获取课程列表

**接口**: `GET /courses`

**认证**: 需要

**查询参数**:
- `search`: 搜索关键词 (课程编号/名称) (可选)
- `page`: 页码 (默认: 1)
- `limit`: 每页数量 (默认: 10)

**响应示例**:
```json
{
  "data": [
    {
      "id": "1",
      "courseId": "CS101",
      "name": "程序设计基础",
      "credit": 3,
      "teacher": "李老师",
      "description": "C++程序设计基础",
      "createdAt": "2024-01-01T12:00:00Z",
      "updatedAt": "2024-01-01T12:00:00Z"
    }
  ],
  "total": 30,
  "page": 1,
  "limit": 10
}
```

---

### 2. 获取课程详情

**接口**: `GET /courses/{id}`

**认证**: 需要

**响应**: 单个课程对象

---

### 3. 添加课程

**接口**: `POST /courses`

**认证**: 需要 (管理员)

**请求参数**:
```json
{
  "courseId": "CS101",
  "name": "程序设计基础",
  "credit": 3,
  "teacher": "李老师",
  "description": "C++程序设计基础"
}
```

**响应**: 201 Created + 课程对象

---

### 4. 更新课程

**接口**: `PUT /courses/{id}`

**认证**: 需要 (管理员)

**请求参数** (部分更新):
```json
{
  "name": "程序设计基础",
  "teacher": "王老师"
}
```

**响应**: 200 OK + 课程对象

---

### 5. 删除课程

**接口**: `DELETE /courses/{id}`

**认证**: 需要 (管理员)

**响应**: 200 OK

---

### 6. 获取选课学生列表

**接口**: `GET /courses/{courseId}/students`

**认证**: 需要

**响应示例**:
```json
[
  {
    "studentId": "2024001",
    "name": "张三",
    "class": "计算机2401",
    "score": 85
  }
]
```

---

## 成绩管理

### 1. 获取成绩列表

**接口**: `GET /grades`

**认证**: 需要

**查询参数**:
- `studentId`: 学生ID (可选)
- `courseId`: 课程ID (可选)
- `class`: 班级 (可选)
- `startTime`: 开始时间 (可选)
- `endTime`: 结束时间 (可选)
- `page`: 页码 (默认: 1)
- `limit`: 每页数量 (默认: 10)

**响应示例**:
```json
{
  "data": [
    {
      "id": "1",
      "studentId": "2024001",
      "studentName": "张三",
      "courseId": "CS101",
      "courseName": "程序设计基础",
      "score": 85,
      "semester": "2024-2025学年第一学期",
      "createdAt": "2024-01-01T12:00:00Z",
      "updatedAt": "2024-01-01T12:00:00Z"
    }
  ],
  "total": 100,
  "page": 1,
  "limit": 10
}
```

---

### 2. 录入成绩

**接口**: `POST /grades`

**认证**: 需要 (管理员/教师)

**请求参数**:
```json
{
  "studentId": "2024001",
  "courseId": "CS101",
  "score": 85,
  "semester": "2024-2025学年第一学期"
}
```

**响应**: 201 Created + 成绩对象

---

### 3. 更新成绩

**接口**: `PUT /grades/{id}`

**认证**: 需要 (管理员/教师)

**请求参数** (部分更新):
```json
{
  "score": 90
}
```

**响应**: 200 OK + 成绩对象

---

### 4. 删除成绩

**接口**: `DELETE /grades/{id}`

**认证**: 需要 (管理员/教师)

**响应**: 200 OK

---

### 5. 批量导入成绩

**接口**: `POST /grades/batch`

**认证**: 需要 (管理员/教师)

**请求**: multipart/form-data, 字段名: `file`

**支持格式**: Excel (.xlsx, .xls) 或 CSV (.csv)

**响应示例**:
```json
{
  "success": 20,
  "failed": 3,
  "message": "导入完成：成功20条，失败3条"
}
```

---

### 6. 导出成绩数据

**接口**: `GET /grades/export`

**认证**: 需要

**查询参数**:
- `studentId`: 学生ID (可选)
- `courseId`: 课程ID (可选)
- `class`: 班级 (可选)
- `startTime`: 开始时间 (可选)
- `endTime`: 结束时间 (可选)
- `format`: `excel` 或 `csv` (默认: excel)

**响应**: 二进制文件流

---

### 7. 获取课程成绩列表

**接口**: `GET /grades/course/{courseId}`

**认证**: 需要

**查询参数**:
- `semester`: 学期 (可选)

**响应示例**:
```json
[
  {
    "studentId": "2024001",
    "name": "张三",
    "class": "计算机2401",
    "score": 85,
    "gradeId": "123"
  }
]
```

---

### 8. 批量更新成绩

**接口**: `POST /grades/batch-update`

**认证**: 需要 (管理员/教师)

**请求参数**:
```json
{
  "courseId": "CS101",
  "semester": "2024-2025学年第一学期",
  "grades": [
    {
      "studentId": "2024001",
      "score": 85
    },
    {
      "studentId": "2024002",
      "score": 90
    }
  ]
}
```

**响应示例**:
```json
{
  "success": 18,
  "failed": 2
}
```

---

## 统计分析

### 1. 获取统计概览

**接口**: `GET /statistics/overview`

**认证**: 需要

**响应示例**:
```json
{
  "avgScore": 76.8,
  "passRate": 85.5,
  "totalStudents": 1200,
  "totalCourses": 45,
  "totalGrades": 8500
}
```

---

### 2. 按班级统计

**接口**: `GET /statistics/class`

**认证**: 需要

**查询参数**:
- `class`: 班级 (可选)
- `courseId`: 课程ID (可选)

**响应示例**:
```json
[
  {
    "class": "计算机2401",
    "avgScore": 82.5,
    "passRate": 94.3,
    "totalStudents": 35,
    "topStudents": [
      {
        "studentId": "2024001",
        "name": "张三",
        "score": 98
      }
    ]
  }
]
```

---

### 3. 按课程统计

**接口**: `GET /statistics/course`

**认证**: 需要

**查询参数**:
- `courseId`: 课程ID (必填)

**响应示例**:
```json
{
  "courseId": "CS101",
  "courseName": "程序设计基础",
  "avgScore": 78.8,
  "passRate": 88.5,
  "totalStudents": 40,
  "highestScore": 96,
  "lowestScore": 45
}
```

---

### 4. 获取排名列表

**接口**: `GET /statistics/ranking`

**认证**: 需要

**查询参数**:
- `class`: 班级 (可选)
- `courseId`: 课程ID (可选)
- `limit`: 显示数量 (默认: 10)

**响应示例**:
```json
[
  {
    "rank": 1,
    "studentId": "2024001",
    "name": "张三",
    "class": "计算机2401",
    "totalScore": 685,
    "avgScore": 85.6,
    "courseCount": 8
  }
]
```

---

### 5. 获取成绩分布

**接口**: `GET /statistics/distribution`

**认证**: 需要

**查询参数**:
- `courseId`: 课程ID (可选)
- `class`: 班级 (可选)

**响应示例**:
```json
[
  {
    "range": "90-100",
    "count": 120,
    "percentage": 12.0
  },
  {
    "range": "80-89",
    "count": 280,
    "percentage": 28.0
  }
]
```

---

### 6. 生成统计报表

**接口**: `GET /statistics/report`

**认证**: 需要

**查询参数**:
- `type`: `class` | `course` | `student` | `overall` (必填)
- `format`: `pdf` | `excel` (必填)
- `class`: 班级 (可选)
- `courseId`: 课程ID (可选)
- `studentId`: 学生ID (可选)
- `semester`: 学期 (可选)
- `startTime`: 开始时间 (可选)
- `endTime`: 结束时间 (可选)

**响应**: 二进制文件流 (PDF 或 Excel)

---

## 报表管理

### 1. 生成成绩单

**接口**: `GET /reports/report-card`

**认证**: 需要

**查询参数**:
- `studentId`: 学生ID (可选)
- `class`: 班级 (可选)
- `semester`: 学期 (可选)

**响应**: PDF 文件流

---

### 2. 生成统计报表

**接口**: `GET /reports/statistics`

**认证**: 需要

**查询参数**:
- `type`: `class` | `course` | `student` | `overall` (必填)
- `format`: `pdf` | `excel` (必填)
- `class`: 班级 (可选)
- `courseId`: 课程ID (可选)
- `studentId`: 学生ID (可选)
- `semester`: 学期 (可选)
- `startTime`: 开始时间 (可选)
- `endTime`: 结束时间 (可选)

**响应**: 二进制文件流

---

### 3. 打印准备

**接口**: `POST /reports/print`

**认证**: 需要

**请求参数**:
```json
{
  "type": "report-card|statistical",
  "data": {
    // 具体的打印数据
  }
}
```

**响应示例**:
```json
{
  "html": "<html>...</html>"
}
```

---

### 4. 批量打印

**接口**: `POST /reports/batch-print`

**认证**: 需要

**请求参数**:
```json
{
  "type": "class|student",
  "items": [
    {
      "id": "2024001",
      "semester": "2024-2025学年第一学期"
    }
  ]
}
```

**响应示例**:
```json
{
  "success": 10,
  "failed": 2
}
```

---

## 用户管理

### 1. 获取用户列表

**接口**: `GET /users`

**认证**: 需要 (管理员)

**查询参数**:
- `role`: `admin` | `teacher` | `student` (可选)
- `search`: 搜索关键词 (用户名/姓名) (可选)
- `page`: 页码 (默认: 1)
- `limit`: 每页数量 (默认: 10)

**响应示例**:
```json
{
  "data": [
    {
      "id": "1",
      "username": "admin",
      "role": "admin",
      "name": "管理员",
      "class": "计算机2401",
      "createdAt": "2024-01-01T12:00:00Z",
      "updatedAt": "2024-01-01T12:00:00Z"
    }
  ],
  "total": 50,
  "page": 1,
  "limit": 10
}
```

---

### 2. 创建用户

**接口**: `POST /users`

**认证**: 需要 (管理员)

**请求参数**:
```json
{
  "username": "teacher001",
  "password": "pass123",
  "role": "teacher",
  "name": "李老师",
  "class": "计算机2401"
}
```

**响应**: 201 Created + 用户对象

---

### 3. 更新用户

**接口**: `PUT /users/{id}`

**认证**: 需要 (管理员)

**请求参数** (部分更新):
```json
{
  "name": "李老师",
  "class": "计算机2402"
}
```

**响应**: 200 OK + 用户对象

---

### 4. 删除用户

**接口**: `DELETE /users/{id}`

**认证**: 需要 (管理员)

**响应**: 200 OK

---

### 5. 批量导入用户

**接口**: `POST /users/batch`

**认证**: 需要 (管理员)

**请求参数**:
```json
{
  "users": [
    {
      "username": "teacher001",
      "password": "pass123",
      "role": "teacher",
      "name": "李老师",
      "class": "计算机2401"
    }
  ]
}
```

**响应示例**:
```json
{
  "success": 10,
  "failed": 2,
  "message": "导入完成：成功10条，失败2条"
}
```

---

### 6. 批量删除用户

**接口**: `DELETE /users/batch`

**认证**: 需要 (管理员)

**请求参数**:
```json
{
  "ids": ["1", "2", "3"]
}
```

**响应示例**:
```json
{
  "success": 3,
  "failed": 0
}
```

---

### 7. 重置密码

**接口**: `PUT /users/{id}/reset-password`

**认证**: 需要 (管理员)

**请求参数**:
```json
{
  "newPassword": "newpass123"
}
```

**响应**: 200 OK

---

## 系统管理

### 1. 创建备份

**接口**: `POST /system/backup`

**认证**: 需要 (管理员)

**响应示例**:
```json
{
  "id": "1",
  "name": "backup_20240101_120000.zip"
}
```

---

### 2. 获取备份列表

**接口**: `GET /system/backups`

**认证**: 需要 (管理员)

**响应示例**:
```json
[
  {
    "id": "1",
    "name": "backup_20240101_120000.zip",
    "size": 1048576,
    "createdAt": "2024-01-01T12:00:00Z",
    "createdBy": "admin"
  }
]
```

---

### 3. 恢复备份

**接口**: `POST /system/restore`

**认证**: 需要 (管理员)

**请求参数**:
```json
{
  "backupId": "1"
}
```

**响应**: 200 OK

---

### 4. 删除备份

**接口**: `DELETE /system/backups/{backupId}`

**认证**: 需要 (管理员)

**响应**: 200 OK

---

### 5. 获取系统日志

**接口**: `GET /system/logs`

**认证**: 需要 (管理员)

**查询参数**:
- `page`: 页码 (默认: 1)
- `limit`: 每页数量 (默认: 10)
- `level`: 日志级别 (`info` | `warning` | `error`) (可选)
- `startTime`: 开始时间 (可选)
- `endTime`: 结束时间 (可选)

**响应示例**:
```json
{
  "data": [
    {
      "id": "1",
      "level": "info",
      "message": "用户登录成功",
      "module": "认证",
      "ip": "192.168.1.100",
      "createdAt": "2024-01-01T12:00:00Z"
    }
  ],
  "total": 100,
  "page": 1,
  "limit": 10
}
```

---

### 6. 获取系统设置

**接口**: `GET /system/settings`

**认证**: 需要 (管理员)

**响应示例**:
```json
{
  "backupInterval": 7,
  "logRetentionDays": 30,
  "maxLoginAttempts": 5,
  "sessionTimeout": 30
}
```

---

### 7. 更新系统设置

**接口**: `PUT /system/settings`

**认证**: 需要 (管理员)

**请求参数**:
```json
{
  "backupInterval": 7,
  "logRetentionDays": 30,
  "maxLoginAttempts": 5,
  "sessionTimeout": 30
}
```

**响应**: 200 OK

---

### 8. 清理日志

**接口**: `POST /system/clean-logs`

**认证**: 需要 (管理员)

**响应**: 200 OK

---

### 9. 导出日志

**接口**: `GET /system/export-logs`

**认证**: 需要 (管理员)

**查询参数**:
- `level`: 日志级别 (可选)
- `startTime`: 开始时间 (可选)
- `endTime`: 结束时间 (可选)

**响应**: CSV 文件流

---

## 错误处理

### 通用错误响应格式

```json
{
  "error": "错误类型",
  "message": "详细的错误信息",
  "details": "额外的错误详情（可选）"
}
```

### 常见错误码

| 状态码 | 错误类型 | 说明 |
|--------|----------|------|
| 400 | Bad Request | 请求参数错误 |
| 401 | Unauthorized | 未认证或 Token 过期 |
| 403 | Forbidden | 权限不足 |
| 404 | Not Found | 资源不存在 |
| 409 | Conflict | 数据冲突（如用户名已存在） |
| 422 | Unprocessable Entity | 请求格式正确但语义错误 |
| 500 | Internal Server Error | 服务器内部错误 |

---

## 数据模型

### 用户 (User)
```typescript
{
  id: string
  username: string
  role: 'admin' | 'teacher' | 'student'
  name: string
  class?: string
  createdAt?: string
  updatedAt?: string
}
```

### 学生 (Student)
```typescript
{
  id: string
  studentId: string
  name: string
  class: string
  gender?: 'male' | 'female'
  phone?: string
  email?: string
  createdAt?: string
  updatedAt?: string
}
```

### 课程 (Course)
```typescript
{
  id: string
  courseId: string
  name: string
  credit: number
  teacher?: string
  description?: string
  createdAt?: string
  updatedAt?: string
}
```

### 成绩 (Grade)
```typescript
{
  id: string
  studentId: string
  studentName: string
  courseId: string
  courseName: string
  score: number
  semester?: string
  createdAt?: string
  updatedAt?: string
}
```

### 操作日志 (OperationLog)
```typescript
{
  id: string
  userId: string
  username: string
  action: string
  module: string
  ip?: string
  createdAt: string
}
```

### 系统日志 (SystemLog)
```typescript
{
  id: string
  level: 'info' | 'warning' | 'error'
  message: string
  module: string
  ip?: string
  createdAt: string
}
```

### 备份 (Backup)
```typescript
{
  id: string
  name: string
  size: number
  createdAt: string
  createdBy: string
}
```

### 系统设置 (SystemSettings)
```typescript
{
  backupInterval: number
  logRetentionDays: number
  maxLoginAttempts: number
  sessionTimeout: number
}
```

---

## 认证流程

1. **登录获取 Token**
   ```
   POST /api/auth/login
   -> 返回 token 和用户信息
   ```

2. **后续请求携带 Token**
   ```
   GET /api/students
   Header: Authorization: Bearer <token>
   ```

3. **Token 过期处理**
   - 401 响应时清除本地 Token
   - 跳转到登录页面

---

## 分页规范

所有列表接口都支持分页，使用以下参数：

- `page`: 页码，从 1 开始
- `limit`: 每页数量

**响应格式**:
```json
{
  "data": [...],
  "total": 100,  // 总记录数
  "page": 1,     // 当前页码
  "limit": 10    // 每页数量
}
```

---

## 搜索和筛选

### 模糊搜索
支持对多个字段进行模糊匹配，如：
- 学生：学号、姓名
- 课程：课程编号、课程名称
- 用户：用户名、姓名

### 精确筛选
- 班级筛选
- 角色筛选
- 时间范围筛选

---

## 文件导出

所有导出接口返回二进制流，前端需要：
1. 创建 Blob 对象
2. 创建下载链接
3. 触发下载

**示例代码**:
```javascript
const blob = new Blob([response.data], { 
  type: 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet' 
})
const url = window.URL.createObjectURL(blob)
const link = document.createElement('a')
link.href = url
link.download = '导出文件.xlsx'
link.click()
window.URL.revokeObjectURL(url)
```

---

## 注意事项

1. **权限控制**
   - 管理员：所有功能
   - 教师：成绩录入、查询、统计
   - 学生：个人信息、成绩查询、统计概览

2. **数据验证**
   - 成绩范围：0-100
   - 密码长度：至少6位
   - 手机号格式：1开头的11位数字
   - 邮箱格式：标准邮箱格式

3. **并发控制**
   - 批量操作建议分批处理
   - 大数据量导出建议使用异步任务

4. **安全性**
   - 所有敏感操作需要二次确认
   - 密码使用 bcrypt 加密存储
   - Token 有过期时间
   - 敏感接口需要权限验证

5. **性能优化**
   - 大数据量查询使用分页
   - 复杂统计使用缓存
   - 文件导出使用流式处理

---

## 更新日志

### v1.0.0 (2026-01-09)
- 初始版本发布
- 完整的用户管理功能
- 学生、课程、成绩管理
- 统计分析功能
- 报表生成和打印
- 系统管理功能
