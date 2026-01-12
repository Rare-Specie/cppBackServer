# API 分页规范文档

## 概述

本文档描述了后端API的分页规范，所有返回列表数据的接口都应遵循此规范。

## 分页参数

### 请求参数

所有分页接口支持以下查询参数：

| 参数名 | 类型 | 默认值 | 最大值 | 说明 |
|--------|------|--------|--------|------|
| `page` | 整数/字符串 | 1 | - | 页码，1-based |
| `limit` | 整数/字符串 | 10 | 1000 | 每页条数 |

### 参数说明

1. **page**: 页码，从1开始计算
   - 支持字符串格式（如 "2"）和整数格式
   - 必须为正整数，负数或0会回退到默认值1
   - 非法格式会回退到默认值1

2. **limit**: 每页显示的数据条数
   - 支持字符串格式（如 "20"）和整数格式
   - 必须为正整数，负数或0会回退到默认值10
   - 超过最大值1000会被自动裁剪为1000
   - 非法格式会回退到默认值10

### 传递方式

分页参数可以通过以下两种方式传递：

1. **URL查询参数**（推荐）：
   ```
   GET /api/students?page=2&limit=20
   ```

2. **HTTP请求头**（备用）：
   ```
   X-Page: 2
   X-Limit: 20
   ```

## 返回格式

### 标准分页响应

```json
{
  "data": [...],
  "total": 100,
  "page": 2,
  "limit": 20,
  "totalPages": 5
}
```

### 字段说明

| 字段名 | 类型 | 说明 |
|--------|------|------|
| `data` | 数组 | 当前页的数据 |
| `total` | 整数 | 符合条件的总记录数 |
| `page` | 整数 | 当前页码 |
| `limit` | 整数 | 每页条数 |
| `totalPages` | 整数 | 总页数 |

### 示例

**请求**：
```
GET /api/students?page=2&limit=10
```

**响应**：
```json
{
  "data": [
    {
      "id": "11",
      "studentId": "2024011",
      "name": "学生11",
      "class": "计算机2401",
      "createdAt": "2026-01-12T10:00:00Z",
      "updatedAt": "2026-01-12T10:00:00Z"
    },
    ...
  ],
  "total": 50,
  "page": 2,
  "limit": 10,
  "totalPages": 5
}
```

## 过滤与分页组合

所有过滤参数应与分页参数联合生效，遵循 **"先过滤，后分页"** 的原则。

### 支持的过滤参数

| 接口 | 参数名 | 说明 |
|------|--------|------|
| GET /api/students | `class` | 按班级过滤 |
| GET /api/students | `search` | 按学号或姓名搜索 |
| GET /api/users | `role` | 按角色过滤 |
| GET /api/users | `search` | 按用户名或姓名搜索 |
| GET /api/courses | `search` | 按课程号或课程名搜索 |

### 组合示例

**请求**：
```
GET /api/students?page=3&limit=20&class=计算机2401&search=张三
```

**处理流程**：
1. 从所有学生中筛选班级为"计算机2401"的学生
2. 在筛选结果中搜索姓名或学号包含"张三"的学生
3. 对最终结果进行分页，返回第3页，每页20条

**响应**：
```json
{
  "data": [...],  // 第3页的20条符合条件的学生
  "total": 45,    // 总共有45名符合条件的学生
  "page": 3,
  "limit": 20,
  "totalPages": 3
}
```

## 字段选择

为支持按需返回数据，提供字段选择功能。

### 参数

| 参数名 | 说明 | 示例 |
|--------|------|------|
| `fields` | 指定返回的字段，逗号分隔 | `id,studentId,name,phone` |
| `full` | 返回完整数据（所有字段） | `true` |

### 使用方式

1. **选择特定字段**：
   ```
   GET /api/students?fields=id,studentId,name,phone
   ```

2. **请求完整数据**：
   ```
   GET /api/students?full=true
   ```

### 示例

**请求**：
```
GET /api/students?page=1&limit=5&fields=id,studentId,name
```

**响应**：
```json
{
  "data": [
    {"id": "1", "studentId": "2024001", "name": "张三"},
    {"id": "2", "studentId": "2024002", "name": "李四"},
    ...
  ],
  "total": 50,
  "page": 1,
  "limit": 5,
  "totalPages": 10
}
```

## 日期格式

所有日期时间字段统一使用 **ISO 8601** 格式：

```
YYYY-MM-DDTHH:MM:SSZ
```

例如：`2026-01-12T10:30:45Z`

### 兼容性

为保证向后兼容，系统会自动将旧格式的时间戳转换为ISO 8601格式：
- 旧格式：`Wed Jan 12 10:30:45 2026`
- 新格式：`2026-01-12T10:30:45Z`

## 错误处理

### 参数验证错误

当分页参数非法时，系统会：
1. 记录警告日志
2. 使用默认值
3. 正常返回数据（不会返回400错误）

**示例**：
```
GET /api/students?page=-1&limit=abc
```

**响应**（使用默认值 page=1, limit=10）：
```json
{
  "data": [...],
  "total": 50,
  "page": 1,
  "limit": 10,
  "totalPages": 5
}
```

### 认证/授权错误

| 错误码 | 说明 |
|--------|------|
| 401 | 缺少Token或Token无效 |
| 403 | 权限不足 |

**示例**：
```json
{
  "error": "Unauthorized",
  "message": "Missing token"
}
```

## 批量导入兼容性

### 支持的格式

**格式1：直接数组**（新推荐）
```json
POST /api/students/batch
[
  {"studentId": "2024001", "name": "张三", "class": "计算机2401"},
  {"studentId": "2024002", "name": "李四", "class": "计算机2401"}
]
```

**格式2：对象包装**（向后兼容）
```json
POST /api/students/batch
{
  "students": [
    {"studentId": "2024001", "name": "张三", "class": "计算机2401"},
    {"studentId": "2024002", "name": "李四", "class": "计算机2401"}
  ]
}
```

### 批量导入响应

**成功**（201）：
```json
{
  "success": 2,
  "failed": 0,
  "successItems": [
    {"index": 0, "studentId": "2024001", "name": "张三"},
    {"index": 1, "studentId": "2024002", "name": "李四"}
  ],
  "failedItems": [],
  "message": "导入完成：成功2条，失败0条"
}
```

**部分成功**（207）：
```json
{
  "success": 1,
  "failed": 1,
  "successItems": [...],
  "failedItems": [
    {"index": 1, "error": "Student ID already exists: 2024002"}
  ],
  "message": "导入完成：成功1条，失败1条"
}
```

**全部失败**（400）：
```json
{
  "success": 0,
  "failed": 2,
  "successItems": [],
  "failedItems": [
    {"index": 0, "error": "Missing required field: studentId"},
    {"index": 1, "error": "Invalid phone format: 123"}
  ],
  "message": "导入完成：成功0条，失败2条"
}
```

## 接口列表

### 学生管理

| 接口 | 方法 | 分页支持 | 过滤参数 | 字段选择 |
|------|------|----------|----------|----------|
| `/api/students` | GET | ✅ | `class`, `search` | ✅ |
| `/api/students` | POST | ❌ | - | - |
| `/api/students/:id` | GET | ❌ | - | - |
| `/api/students/:id` | PUT | ❌ | - | - |
| `/api/students/:id` | DELETE | ❌ | - | - |
| `/api/students/batch` | POST | ❌ | - | - |

### 用户管理

| 接口 | 方法 | 分页支持 | 过滤参数 | 字段选择 |
|------|------|----------|----------|----------|
| `/api/users` | GET | ✅ | `role`, `search` | ✅ |
| `/api/users` | POST | ❌ | - | - |
| `/api/users/:id` | GET | ❌ | - | - |
| `/api/users/:id` | PUT | ❌ | - | - |
| `/api/users/:id` | DELETE | ❌ | - | - |

### 课程管理

| 接口 | 方法 | 分页支持 | 过滤参数 | 字段选择 |
|------|------|----------|----------|----------|
| `/api/courses` | GET | ✅ | `search` | ✅ |
| `/api/courses` | POST | ❌ | - | - |
| `/api/courses/:id` | GET | ❌ | - | - |
| `/api/courses/:id` | PUT | ❌ | - | - |
| `/api/courses/:id` | DELETE | ❌ | - | - |

### 成绩管理

| 接口 | 方法 | 分页支持 | 过滤参数 | 字段选择 |
|------|------|----------|----------|----------|
| `/api/grades` | GET | ✅ | `studentId`, `courseId`, `semester` | ✅ |
| `/api/grades` | POST | ❌ | - | - |
| `/api/grades/:id` | GET | ❌ | - | - |
| `/api/grades/:id` | PUT | ❌ | - | - |
| `/api/grades/:id` | DELETE | ❌ | - | - |

## 最佳实践

### 1. 前端实现建议

```javascript
// 分页查询示例
async function getStudents(page = 1, limit = 10, filters = {}) {
  const params = new URLSearchParams({
    page: page.toString(),
    limit: limit.toString(),
    ...filters
  });
  
  const response = await fetch(`/api/students?${params}`, {
    headers: {
      'Authorization': `Bearer ${token}`,
      'Content-Type': 'application/json'
    }
  });
  
  return response.json();
}

// 使用示例
const result = await getStudents(2, 20, { class: '计算机2401', search: '张三' });
console.log(`显示 ${result.data.length} 条数据，共 ${result.totalPages} 页`);
```

### 2. 大数据量优化

- 建议 `limit` 不超过 100
- 对于超大数据量，考虑使用游标分页（cursor-based pagination）
- 在数据库层面建立合适的索引

### 3. 错误处理

```javascript
try {
  const result = await getStudents(page, limit, filters);
  // 处理成功响应
} catch (error) {
  if (error.status === 401) {
    // 重新登录
  } else if (error.status === 403) {
    // 提示权限不足
  } else {
    // 其他错误处理
  }
}
```

## 测试用例

### 单元测试

```bash
# 运行分页相关测试
./run_tests --filter="pagination"
./run_tests --filter="fields"
./run_tests --filter="batch"
```

### 集成测试

```bash
# 运行完整集成测试
./run_tests --filter="integration"
```

### 手动测试

```bash
# 1. 测试默认分页
curl -H "Authorization: Bearer $TOKEN" http://localhost:8080/api/students

# 2. 测试自定义分页
curl -H "Authorization: Bearer $TOKEN" "http://localhost:8080/api/students?page=2&limit=5"

# 3. 测试过滤+分页
curl -H "Authorization: Bearer $TOKEN" "http://localhost:8080/api/students?class=计算机2401&search=张三&page=1&limit=10"

# 4. 测试字段选择
curl -H "Authorization: Bearer $TOKEN" "http://localhost:8080/api/students?fields=id,studentId,name&page=1&limit=5"

# 5. 测试批量导入（数组格式）
curl -X POST -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
  -d '[{"studentId":"TEST001","name":"测试","class":"测试班"}]' \
  http://localhost:8080/api/students/batch

# 6. 测试批量导入（对象格式）
curl -X POST -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
  -d '{"students":[{"studentId":"TEST002","name":"测试2","class":"测试班"}]}' \
  http://localhost:8080/api/students/batch
```

## 更新日志

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-01-12 | v1.0 | 初始版本，实现分页、过滤、字段选择、批量导入兼容性 |