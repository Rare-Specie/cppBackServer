# GEN5 API 参考（自动生成）

> 说明：本文件基于代码扫描自动生成，包含接口清单、请求与响应示例、分页/过滤规范与批量导入约定。

---

## 概览

- 基本路径：`http://<host>:21180/api`
- 授权：大多数接口需在请求头中带 `Authorization: Bearer <token>`。
- 错误响应格式：

```json
{ "error": "ErrorCode", "message": "Error message" }
```

- 分页响应标准：

```json
{
  "data": [...],
  "total": 123,
  "page": 1,
  "limit": 10,
  "totalPages": 13
}
```

- 常用请求头：
  - `X-Page`, `X-Limit`, `X-Fields`, `X-Full`, `X-Query-*`
- 日期格式：ISO 8601（示例：`2026-01-12T10:30:45Z`）

---

## 认证（Auth）

- POST `/api/auth/login`
  - 请求体：`{ "username": "..", "password": "..", "role": "admin|teacher|student" }`
  - 成功：200 `{ "token": "<jwt>", "user": { ... } }`

- POST `/api/auth/logout` （需要 Authorization）
- GET `/api/auth/verify` （需要 Authorization）

---

## 用户（User）

- GET `/api/user/profile` （Authorization） — 返回当前用户对象。
- PUT `/api/user/password` — `{ "oldPassword":"..","newPassword":".." }`。
- GET `/api/user/logs` — 分页、字段过滤支持（ISO 时间）。

### 管理员：用户管理
- GET `/api/users` — 支持 `X-Page`, `X-Limit`, `X-Query-Role`, `X-Query-Search`, `X-Fields`。
- POST `/api/users` — 创建用户（admin 权限）。
- PUT `/api/users/{id}`, DELETE `/api/users/{id}`。
- POST `/api/users/batch` — 批量导入（数组或 `{ users: [...] }`），返回 `{ success, failed, successItems, failedItems }`，状态码 201/207/400。
- DELETE `/api/users/batch` — 批量删除（传 `ids` 数组）。
- PUT `/api/users/{id}/reset-password` — 重置密码。

---

## 学生（Student）

- GET `/api/students` — 支持分页/过滤/字段选择：`X-Page`, `X-Limit`, `X-Query-Class`, `X-Query-Search`, `X-Fields`, `X-Full`。
- GET `/api/students/{id}` — 学生详情。
- POST `/api/students` — `admin|teacher` 权限，必填 `studentId,name,class`。
- PUT `/api/students/{id}`, DELETE `/api/students/{id}`。
- POST `/api/students/batch` — 支持数组或 `{ "students": [...] }`。返回 `successItems`/`failedItems`，状态码 201/207/400。
- GET `/api/students/export` — 导出学生数据（当前为 JSON）。
- GET `/api/students/{studentId}/grades` — 学生成绩概览（totalCourses, avgScore, passRate, recentGrades）。

---

## 课程（Course）

- GET `/api/courses` — 支持分页与 `X-Query-Search`。
- GET `/api/courses/{id}`，POST `/api/courses`（admin），PUT/DELETE `/api/courses/{id}`。
- GET `/api/courses/{courseId}/students` — 返回选课学生，支持 `X-Page`, `X-Limit`, `X-Fields`。
- POST `/api/courses/{courseId}/enroll` — `{"studentId":"..."}`（201）。
- DELETE `/api/courses/{courseId}/enroll/{studentId}`。

---

## 成绩（Grade）

- GET `/api/grades` — 支持分页/过滤/字段选择：`X-Query-StudentId`, `X-Query-CourseId`, `X-Query-Class`, `X-Query-Semester`；若请求者为 `role==student` 则强制返回其绑定 `studentId` 的记录。
- POST `/api/grades` — `admin|teacher`（必填 `studentId, courseId, score`，0-100）。
- PUT `/api/grades/{id}`, DELETE `/api/grades/{id}`。
- POST `/api/grades/batch` — 导入（数组或 `{ grades: [...] }`），返回 success/failed 列表，状态码 201/207/400。
- POST `/api/grades/batch-update` — 按 `courseId` 与 `semester` 批量更新或创建。
- GET `/api/grades/export` — 导出过滤后的成绩（含 ISO 时间）。
- GET `/api/grades/course/{courseId}` — 课程成绩列表（分页，字段过滤）。

---

## 统计（Statistics）

- GET `/api/statistics/overview`
- GET `/api/statistics/class` — 返回班级统计与 topStudents。
- GET `/api/statistics/course` — 需要 `courseId`。
- GET `/api/statistics/ranking` — 支持分页/字段选择。
- GET `/api/statistics/distribution` — 返回分数段分布。
- GET `/api/statistics/report` — 根据 `type`/`format` 生成报表（JSON 示例；生产应返回文件）。

---

## 报表（Reports）

- GET `/api/reports/report-card` — 生成成绩单（HTML，需 query 参数或 header 指定 studentId/class）。
- GET `/api/reports/statistics` — 使用 `X-Query-Type`, `X-Query-Format`。
- POST `/api/reports/print` — 准备打印 HTML。
- POST `/api/reports/batch-print` — 批量打印准备，返回 success/failed 计数。

---

## 系统（System）

- POST `/api/system/backup` — 创建备份（admin），返回 `Backup` 对象（201）。
- GET `/api/system/backups` — 列表。
- POST `/api/system/restore` — `{ "backupId":"..." }`。
- DELETE `/api/system/backups/{backupId}`。
- GET `/api/system/logs` — 支持 `X-Query-Level`, `X-Query-StartTime`, `X-Query-EndTime`, 分页与字段选择（ISO 时间）。
- GET `/api/system/export-logs` — 导出日志（当前为 JSON，含 createdAt ISO）。
- GET/PUT `/api/system/settings` — PUT 的 body 包括 backupInterval, logRetentionDays, maxLoginAttempts, sessionTimeout。
- POST `/api/system/clean-logs`。

---

## 健康检查
- GET `/api/health` — 返回 `{ "status":"ok", "message":"Server is running" }`。

---

## 分页规范（摘自 API_PAGINATION_DOCUMENTATION）

（以下为分页文档节选，已整合）

参见文件 `API_PAGINATION_DOCUMENTATION.md` 中的详细内容，包含分页参数说明、传递方式（URL 推荐，Header 兼容）、过滤与分页组合、字段选择、日期格式、错误处理、批量导入兼容性、接口列表、最佳实践及测试用例示例。

---

## 常见状态码速览
- 200 OK
- 201 Created
- 207 Multi-Status（部分成功）
- 400 Bad Request
- 401 Unauthorized
- 403 Forbidden
- 404 Not Found
- 409 Conflict
- 500 InternalError

---

## 示例（curl）

- 登录：
```bash
curl -X POST http://localhost:21180/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123","role":"admin"}'
```

- 获取学生（字段选择、分页）：
```bash
curl -X GET "http://localhost:21180/api/students?page=2&limit=20" \
  -H "Authorization: Bearer <token>" \
  -H "X-Fields: studentId,name,class"
```

- 批量导入（数组格式）：
```bash
curl -X POST http://localhost:21180/api/students/batch \
  -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
  -d '[{"studentId":"TEST001","name":"测试","class":"测试班"}]'
```

---

## 建议/下一步
- 建议将 `X-Query-*` 风格的过滤参数改为 URL 查询参数以符合 REST 习惯（我可以帮助完成修改）。
- 报表/导出建议实现为真实文件下载（CSV/Excel/PDF）。
- 若需，我可以生成 OpenAPI 草案或 Postman collection。

---

*生成时间：2026-01-12*
