# API 文档 ✅

**Base URL:** `http://localhost:21180/api`

---

## 认证（Auth） 🔐

- All endpoints that require authentication use the `Authorization` header:
  - `Authorization: Bearer <token>`
- 登录接口会返回 `token`，后续请求需要使用该 token。

### POST /auth/login
- 描述: 用户登录，返回 JWT token 与用户信息
- 请求体 (JSON):
```json
{
  "username": "admin",
  "password": "123456",
  "role": "admin" // admin|teacher|student
}
```
- 成功响应 200:
```json
{
  "token": "<token>",
  "user": { /* User 对象 */ }
}
```
- 常见错误:
  - 400 Invalid JSON / Missing required fields
  - 401 Invalid credentials

### POST /auth/logout
- 描述: 注销（从头部读取 `Authorization`）
- 请求头: `Authorization: Bearer <token>`
- 成功响应 200:
```json
{"message": "Logged out successfully"}
```
- 错误: 401 Missing token

### GET /auth/verify
- 描述: 验证 token 是否有效
- 请求头: `Authorization: Bearer <token>`
- 成功响应 200: `{"message": "Token valid"}`
- 错误: 401 Missing/Invalid token

---

## 用户管理（Users） 👥

> 说明: 管理类接口多数需要 `admin` 权限。所有用户对象参考 `User` model。

> 新增: `User` 对象支持可选字段 `studentId`，当 `role` 为 `student` 时可绑定到对应的 `Student.studentId`，绑定后学生登录即可查看自己的成绩。

### GET /users
- 描述: 获取用户列表（管理员）
- 认证: `Authorization: Bearer <token>`（需要 role=`admin`）
- 可选筛选（实现中以请求头 `X-Query-Role` / `X-Query-Search` 模拟 URL 查询参数）:
  - `X-Query-Role: admin|teacher|student`
  - `X-Query-Search: <关键词>`
- 返回 200 分页对象:
```json
{
  "data": [ /* array of User */ ],
  "total": 42,
  "page": 1,
  "limit": 10
}
```

### POST /users
- 描述: 创建用户（管理员）
- 权限: admin
- 请求体:
```json
{
  "username": "jdoe",
  "password": "secret",
  "role": "teacher",
  "name": "John Doe",
  "class": "Class A" // 可选
}
```
- 成功响应 201: 返回创建的 `User` 对象
- 错误: 400 Missing fields / 409 Username already exists

### PUT /users/:id
- 描述: 更新用户（管理员）
- 请求体（部分更新）:
```json
{
  "name": "New Name",
  "class": "Class B",
  "role": "teacher"
}
```
- 成功响应 200: 返回更新后的 `User`
- 错误: 401/403/404

### DELETE /users/:id
- 描述: 删除用户（管理员），不能删除自己
- 成功响应 200: `{"message": "User deleted successfully"}`
- 错误: 409 Cannot delete yourself, 404 Not found

### POST /users/batch (导入)
- 请求体: `{"users": [ {...}, {...} ]}`
- 返回统计 {"success": n, "failed": m}

### DELETE /users/batch
- 请求体: `{"ids": ["id1","id2"]}`
- 返回统计 {"success": n, "failed": m}

### PUT /users/:id/reset-password
- 请求体: `{ "newPassword": "..." }`
- 成功: 200 message
- 错误: 400 / 404

### GET /user/profile
- 描述: 获取当前登录用户信息
- 认证: 必需

### PUT /user/password
- 描述: 用户自行修改密码
- 请求体: `{ "oldPassword": "..", "newPassword": ".." }`
- 返回: 200 或 400/401

### GET /user/logs
- 描述: 获取当前用户的操作日志
- 返回分页结构

---

## 学生管理（Students） 🎓

### GET /students
- 认证: 必需
- 支持分页、筛选（代码里简化为 header 参数）
- 返回分页对象

### GET /students/:id
- 描述: 学生详情
- 返回 200 User 对象或 404

### POST /students
- 描述: 创建学生（admin/teacher）
- 请求体示例:
```json
{
  "studentId": "S12345",
  "name": "张三",
  "class": "Class A",
  "gender": "male", // 可选
  "phone": "13800138000", // 可选
  "email": "a@example.com" // 可选
}
```
- 成功 201 返回创建的学生对象
- 错误: 400 / 409

### PUT /students/:id
- 描述: 更新学生（admin/teacher）
- 请求体为可选字段的 JSON

### DELETE /students/:id
- 描述: 删除学生（admin）

### POST /students/batch
- 描述: 批量导入学生（数组形式）

### GET /students/export
- 描述: 导出学生数据（目前返回 JSON 数组）

### GET /students/:id/grades
- 描述: 获取学生成绩概览（总课程数、平均分、通过率、最近成绩）
- 返回 JSON 结构包含 `totalCourses`, `avgScore`, `passRate`, `recentGrades` 等

---

## 课程管理（Courses） 📚

### GET /courses
- 认证: 必需
- 返回分页课程列表

### GET /courses/:id
- 课程详情

### POST /courses (admin)
- 请求体:
```json
{
  "courseId": "C101",
  "name": "Mathematics",
  "credit": 3,
  "teacher": "Teacher A", // 可选
  "description": "..." // 可选
}
```
- 成功 201 返回新课程

### PUT /courses/:id (admin)
- 更新课程字段

### DELETE /courses/:id (admin)
- 删除课程

### GET /courses/:id/students
- 描述: 返回选修该课程的学生列表（含 score）

### POST /courses/:id/enroll (admin|teacher)
- 请求体: `{ "studentId": "S12345" }`
- 成功 201 返回 enrollment 相关信息

### DELETE /courses/:id/enroll/:studentId (admin|teacher)
- 描述: 取消选课

---

## 成绩管理（Grades） 📝

### GET /grades
- 认证: 必需
- 支持按 `studentId` / `courseId` / `class` 筛选
- 如果当前用户为学生且已绑定 `studentId`，则自动返回该学生的成绩（学生不能访问其他学生的成绩）
- 返回分页

### POST /grades (admin|teacher)
- 请求体:
```json
{
  "studentId": "S12345",
  "courseId": "C101",
  "score": 88,
  "semester": "2025-2026-1" // 可选
}
```
- 成功 201 返回创建的 Grade 对象
- 错误: 400 / 404 / 409

### PUT /grades/:id (admin|teacher)
- 请求体: `{ "score": 95 }`
- 返回更新后的 Grade

### DELETE /grades/:id (admin|teacher)
- 描述: 删除某条成绩记录

### POST /grades/batch
- 批量导入成绩（数组形式）

### POST /grades/batch-update (admin|teacher)
- 用于按课程+学期批量更新/创建成绩
- 请求体示例:
```json
{
  "courseId": "C101",
  "semester": "2025-2026-1",
  "grades": [ {"studentId":"S1","score":90}, {"studentId":"S2","score":85} ]
}
```

### GET /grades/export
- 导出成绩数据（目前返回 JSON 数组）

### GET /grades/course/:courseId
- 返回该课程学生成绩列表

---

## 统计分析（Statistics） 📊

### GET /statistics/overview
- 返回总体统计: `avgScore`, `passRate`, `totalStudents`, `totalCourses`, `totalGrades`

### GET /statistics/class
- 按班级统计，返回每班 `avgScore`, `passRate`, `topStudents`(前三)

### GET /statistics/course
- 根据 `courseId` 返回课程统计（注意：实现中要求 `courseId` 非空）

### GET /statistics/ranking
- 返回排名列表，可筛选班级或课程

### GET /statistics/distribution
- 返回成绩分布（区间统计）

### GET /statistics/report
- 生成统计报表（type 和 format 必需，示例：type=overall|class|course|student; format=json）

---

## 报表管理（Reports） 📑

### GET /reports/report-card
- 参数: `studentId` 或 `class`（必需其中之一）
- 返回 HTML 成绩单（Content-Type: text/html）

### GET /reports/statistics
- 参数: `type` 和 `format`（必需）
- 返回 JSON（演示）

### POST /reports/print
- 请求体: `{ "type": "report-card", "data": { ... } }`
- 返回打印准备的 HTML（封装在 JSON 中）

### POST /reports/batch-print
- 请求体: `{ "type": "...", "items": [...] }` 返回导出统计

---

## 系统管理（System） ⚙️

> 大多数接口仅限 `admin`。

### POST /system/backup (admin)
- 创建备份，返回 Backup 对象 (201)

### GET /system/backups (admin)
- 列表备份

### POST /system/restore (admin)
- 请求体: `{ "backupId": "..." }`
- 恢复指定备份

### DELETE /system/backups/:id (admin)
- 删除备份

### GET /system/logs (admin)
- 返回系统日志（分页）

### GET /system/settings (admin)
- 返回系统设置

### PUT /system/settings (admin)
- 请求体示例:
```json
{
  "backupInterval": 24,
  "logRetentionDays": 30,
  "maxLoginAttempts": 5,
  "sessionTimeout": 24
}
```

### POST /system/clean-logs (admin)
- 清理日志

### GET /system/export-logs (admin)
- 导出日志（目前返回 JSON array）

---

## 健康检查

### GET /health
- 返回 `{ "status": "ok", "message": "Server is running" }`

---

## 错误格式及通用说明 ⚠️
- 错误响应默认格式:
```json
{ "error": "<ErrorType>", "message": "<Human readable message>" }
```
- 常见 HTTP 状态码: 200, 201, 400 (BadRequest), 401 (Unauthorized), 403 (Forbidden), 404 (NotFound), 409 (Conflict), 500 (InternalError)

---

## 说明与注意事项 💡
- 当前实现中有些查询参数为简化实现，使用了自定义 Header（例如 `X-Query-Role`）来模拟 URL query 参数；前端调用时建议使用标准 query string，如果需要我可以把文档和代码中的查询参数使用方式统一并补充示例。
- 部分导出/打印/报表接口仅作示例（返回 JSON 或 HTML），生产应实现文件生成（Excel/PDF）与文件流返回。

---

如果你希望我把该文档合并进现有的 `Gen2 API文档.md`、生成 OpenAPI (Swagger) 规范，或添加更多示例（curl/JS/TS），告诉我你偏好的格式和范围，我会继续完善。 ✅
