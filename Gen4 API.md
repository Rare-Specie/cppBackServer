# Gen4 API 文档 ✅

**Base URL:** `http://localhost:21180/api`

---

## 概览 ✨
- 版本：Gen4
- 目标：在 Gen3 的基础上明确了用户与学生绑定机制（`User.studentId`），并保证学生登录后只能查看自己绑定的成绩；补充模型说明、错误行为、示例请求与迁移指南。

---

## 重要变更 🔔
- **新增**：`User` 对象支持可选字段 `studentId`（当 `role == "student"` 时有效）。
- **行为变更**：当当前用户为 `student` 且已绑定 `studentId`，访问 `GET /api/grades` 将自动只返回该学生的成绩；若未绑定则返回 403 错误。
- **兼容性**：对管理员/教师权限路径无影响，但批量导入/创建学生账号时可提供 `studentId` 字段以完成绑定。

---

## 认证（Auth） 🔐
- 使用 Bearer Token：`Authorization: Bearer <token>`。
- 登录：`POST /api/auth/login`（body: `{"username","password","role"}`）返回：`{"token":"...","user":{User}}`。
- Token：由服务内部生成并保存在 `data/tokens.json`（简易实现）。
- 权限检查通过 `AuthManager::hasPermission(token, requiredRoles)` 实现：`admin` 享有所有权限，`teacher` 与 `admin` 可管理课程/成绩，`student` 仅能查看自身数据。

---

## 模型（Models） 📦

- User
  - id (string)
  - username (string)
  - passwordHash (string)
  - role (string) // admin | teacher | student
  - name (string)
  - class (optional string)
  - studentId (optional string) // 仅当 role == "student" 时绑定到 Student.studentId
  - createdAt, updatedAt

- Student
  - id, studentId, name, class, gender?, phone?, email?, createdAt, updatedAt

- Course
  - id, courseId, name, credit, teacher?, description?, createdAt, updatedAt

- Grade
  - id, studentId, studentName, courseId, courseName, score, semester?, createdAt, updatedAt

- JWTToken
  - token, issuedAt, expiresAt, userId

---

## 错误与响应风格
- 成功响应：HTTP 200 / 201，JSON 格式。使用 `jsonResponse()` 统一返回。
- 错误响应：`errorResponse(error, message, code)`，例如：
  - 401 Unauthorized：缺少或无效 Token
  - 403 Forbidden：权限不足或学生未绑定 studentId
  - 400 BadRequest：参数缺失或格式错误
  - 404 NotFound：资源不存在
  - 409 Conflict：重复或冲突

---

## API 列表（端点摘要）

> 下面列出所有主要端点，包含方法、权限、主要请求参数与示例响应要点。

### 认证（Auth）
- POST /api/auth/login
  - body: {"username":"...","password":"...","role":"admin|teacher|student"}
  - 成功 200: {"token":"...","user":{User}}
- POST /api/auth/logout
  - header: Authorization
  - 成功 200: {"message":"Logged out successfully"}
- GET /api/auth/verify
  - header: Authorization
  - 成功 200: {"message":"Token valid"}

### 用户（Users） 👥
- GET /api/users (admin)
  - header: Authorization
  - 支持筛选：X-Query-Role, X-Query-Search
- POST /api/users (admin)
  - body: {"username","password","role","name","class"?,"studentId"?}
  - 当 role=="student" 且提供 studentId 时会校验该 studentId 存在并保存绑定
- PUT /api/users/:id (admin)
  - body 可包含：name, class, role, studentId（可设为 null 解除绑定）
- DELETE /api/users/:id (admin)
- POST /api/users/batch (admin)
  - body: {"users": [ ... ]}（每项可包含 studentId，若为学生则检查 studentId）

### 学生（Students） 🎓
- GET /api/students
- GET /api/students/:id
- POST /api/students (admin|teacher)
  - body: {"studentId","name","class","gender"? ...}
- PUT /api/students/:id (admin|teacher)
- DELETE /api/students/:id (admin)
- POST /api/students/batch (admin)
- GET /api/students/export
- GET /api/students/:id/grades — 返回学生成绩概览

### 课程（Courses） 📚
- GET /api/courses
- GET /api/courses/:id
- POST /api/courses (admin)
- PUT /api/courses/:id (admin)
- DELETE /api/courses/:id (admin)
- GET /api/courses/:id/students — 返回该课程的选课学生（含 score）
- POST /api/courses/:id/enroll (admin|teacher)
  - body: {"studentId"}
- DELETE /api/courses/:id/enroll/:studentId (admin|teacher)

### 成绩（Grades） 📝
- GET /api/grades
  - 认证必需。
  - 支持 query: studentId, courseId, class（实现中以 header/简化方式模拟 URL 查询参数）。
  - **若当前用户为 student 且绑定 studentId，则只返回该 studentId 的成绩（强制）。**
- POST /api/grades (admin|teacher)
  - body: {"studentId","courseId","score","semester"?}
- PUT /api/grades/:id (admin|teacher)
- DELETE /api/grades/:id (admin|teacher)
- POST /api/grades/batch
- POST /api/grades/batch-update (admin|teacher)
- GET /api/grades/export
- GET /api/grades/course/:courseId

### 统计（Statistics） 📊
- GET /api/statistics/overview
- GET /api/statistics/class
- GET /api/statistics/course (requires courseId)
- GET /api/statistics/ranking
- GET /api/statistics/distribution
- GET /api/statistics/report (type & format required)

### 报表（Reports） 📑
- GET /api/reports/report-card?studentId=... 或 ?class=...
  - 返回 HTML 成绩单
- GET /api/reports/statistics?type=...&format=...
- POST /api/reports/print
- POST /api/reports/batch-print

---

## 示例请求（快速示例）

- 创建 student 并绑定用户（管理员）：

curl -X POST "http://localhost:21180/api/users" \
  -H "Authorization: Bearer <admin-token>" \
  -H "Content-Type: application/json" \
  -d '{"username":"student2","password":"student123","role":"student","name":"学生二","studentId":"S2001"}'

- 学生登录并查看成绩：

curl -X POST "http://localhost:21180/api/auth/login" -d '{"username":"student2","password":"student123","role":"student"}'
# 获取 token 后：
curl -X GET "http://localhost:21180/api/grades" -H "Authorization: Bearer <student-token>"

> 注意：若学生未绑定 `studentId`，上述 GET 会返回 403（学生账号未绑定学生记录）。

---

## 迁移与兼容说明 🛠️
- 兼容性策略：如果当前 `User` 数据库中已有 student 用户但未设置 `studentId`，系统仍允许其登录，但访问学生专属接口（如 GET /api/grades）时会返回 403，需要管理员或导入脚本为该用户设置 `studentId`。
- 推荐迁移步骤：
  1. 使用学生名单（studentId、username 关联）运行导入脚本，将 `studentId` 写入对应的 `User` 对象。
  2. 在用户管理 UI 或导入流程中增加 `studentId` 映射列，便于批量绑定。

---

## 测试脚本
- `test_student_grades.sh`：已加入仓库，用于：管理员创建学生、课程、录入成绩、创建绑定用户、学生登录并验证 `GET /api/grades` 返回成绩的流程。

---

## 后续改进建议 💡
- 使用标准 JWT 库替换当前简易 Token 实现（支持签名、刷新、角色声明等）。
- 支持更丰富的查询参数（分页、排序、按学期过滤等）在 GET /api/grades 中。
- 为批量导入提供 CSV 文件支持与更详细的导入报告（错误行、原因）。

---

If you'd like, I can now:
- Add more example request/response JSON snippets per endpoint ✅
- Add OpenAPI/Swagger spec (YAML/JSON) for Gen4 ✅
- Implement a migration script to backfill `User.studentId` from a CSV ✅

请选择你希望接下来我做的项。