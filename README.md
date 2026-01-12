# 学生成绩管理系统后端

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Crow](https://img.shields.io/badge/Framework-Crow-green.svg)](https://github.com/CrowCpp/Crow)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-macOS%20%7C%20Windows-lightgrey.svg)]()

一个基于 C++ 和 Crow 框架开发的高性能学生成绩管理系统后端服务。支持完整的用户管理、学生管理、课程管理、成绩管理、统计分析、报表生成和系统管理功能。
## Winnows编译release在/build_windows中

## ✨ 特性

- 🚀 **高性能**: C++ 编写，性能卓越
- 🔒 **安全认证**: JWT Token 认证，SHA256 密码加密
- 📊 **完整功能**: 从用户管理到报表生成的全套功能
- 🌐 **RESTful API**: 标准的 REST API 接口
- 💾 **数据持久化**: JSON 文件存储，易于备份和迁移
- 🖥️ **跨平台**: 支持 macOS 和 Windows
- 📦 **易于部署**: 静态链接，依赖少

## 📁 项目结构

```
cppBackServer/
├── main.cpp                 # 主程序入口
├── build.sh                 # macOS 编译脚本
├── build_windows.sh         # Windows 交叉编译脚本
├── verify_windows_build.sh  # Windows 版本验证脚本
├── API文档.md               # 详细的 API 文档
├── WINDOWS_BUILD.md         # Windows 编译指南
├── WINDOWS_CROSS_COMPILE_SUMMARY.md  # 交叉编译总结
├── data/                    # 数据目录
│   ├── users.json          # 用户数据
│   ├── students.json       # 学生数据
│   ├── courses.json        # 课程数据
│   ├── grades.json         # 成绩数据
│   ├── operation_logs.json # 操作日志
│   ├── system_logs.json    # 系统日志
│   ├── backups.json        # 备份信息
│   ├── settings.json       # 系统设置
│   └── tokens.json         # Token 数据
├── include/                 # 头文件目录
│   ├── auth.h              # 认证管理
│   ├── data_manager.h      # 数据管理
│   ├── middleware.h        # 中间件
│   ├── models.h            # 数据模型
│   ├── user_service.h      # 用户服务
│   ├── student_service.h   # 学生服务
│   ├── course_service.h    # 课程服务
│   ├── grade_service.h     # 成绩服务
│   ├── statistics_service.h # 统计服务
│   ├── report_service.h    # 报表服务
│   └── system_service.h    # 系统服务
└── build_windows/          # Windows 编译输出
    ├── main.exe            # Windows 可执行文件
    └── README_Windows.txt  # Windows 使用说明
```

## 🚀 快速开始

### 前置要求

#### macOS
```bash
# 安装 Homebrew (如果未安装)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install crow nlohmann-json openssl
```

#### Windows
```bash
# 安装 mingw-w64 (使用 Homebrew)
brew install mingw-w64
```

### 编译运行

#### macOS 版本
```bash
# 1. 克隆或下载项目
cd cppBackServer

# 2. 编译
./build.sh

# 3. 运行
./main
```

#### Windows 版本
```bash
# 1. 克隆或下载项目
cd cppBackServer

# 2. 编译 Windows 版本
./build_windows.sh

# 3. 验证编译结果
./verify_windows_build.sh

# 4. 将 build_windows/ 目录复制到 Windows 系统运行
```

## 🔧 默认账号

系统首次运行时会自动创建默认账号：

| 角色 | 用户名 | 密码 | 权限 |
|------|--------|------|------|
| 管理员 | `admin` | `admin123` | 全部权限 |
| 教师 | `teacher` | `teacher123` | 学生、课程、成绩管理 |
| 学生 | `student` | `student123` | 查看个人信息和成绩 |

## 🌐 API 使用

### 服务地址
```
http://localhost:21180/api
```

### 认证方式
所有需要认证的请求必须在 Header 中包含：
```
Authorization: Bearer <token>
```

### 快速测试

#### 1. 登录获取 Token
```bash
curl -X POST http://localhost:21180/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123","role":"admin"}'
```

#### 2. 获取用户列表 (需要管理员权限)
```bash
curl -X GET http://localhost:21180/api/users \
  -H "Authorization: Bearer <your_token>"
```

#### 3. 获取学生列表
```bash
curl -X GET http://localhost:21180/api/students \
  -H "Authorization: Bearer <your_token>"
```

## 📚 详细文档

- **[API 文档](API文档.md)** - 完整的 API 接口说明
- **[Windows 编译指南](WINDOWS_BUILD.md)** - Windows 平台编译详细指南
- **[交叉编译总结](WINDOWS_CROSS_COMPILE_SUMMARY.md)** - 技术实现细节

## 🔍 功能模块

### 1. 认证管理 (`/api/auth`)
- 用户登录
- 用户登出
- Token 验证
- 修改密码

### 2. 用户管理 (`/api/users`)
- 获取用户列表
- 创建新用户
- 修改用户信息
- 禁用/启用用户
- 重置密码

### 3. 学生管理 (`/api/students`)
- 获取学生列表
- 添加/修改/删除学生
- 按班级查询
- 学生信息导出

### 4. 课程管理 (`/api/courses`)
- 课程列表
- 添加/修改/删除课程
- 课程关联学生
- 课程信息导出

### 5. 成绩管理 (`/api/grades`)
- 成绩录入
- 成绩查询
- 成绩修改
- 成绩导出
- 批量录入

### 6. 统计分析 (`/api/statistics`)
- 总体概览
- 班级统计
- 课程统计
- 成绩分布
- 排名分析

### 7. 报表生成 (`/api/reports`)
- 成绩单生成
- 统计报表
- 打印准备
- 批量打印

### 8. 系统管理 (`/api/system`)
- 数据备份
- 数据恢复
- 操作日志
- 系统日志
- 系统设置

## 🛠️ 开发指南

### 项目架构

```
main.cpp
├── 初始化组件
│   ├── DataManager (数据管理)
│   ├── AuthManager (认证管理)
│   └── LogMiddleware (日志中间件)
│
├── 服务层
│   ├── UserService (用户服务)
│   ├── StudentService (学生服务)
│   ├── CourseService (课程服务)
│   ├── GradeService (成绩服务)
│   ├── StatisticsService (统计服务)
│   ├── ReportService (报表服务)
│   └── SystemService (系统服务)
│
└── API 路由
    ├── 认证路由
    ├── 业务路由
    └── 管理路由
```

### 数据模型

- **User**: 用户信息
- **Student**: 学生信息
- **Course**: 课程信息
- **Grade**: 成绩信息
- **OperationLog**: 操作日志
- **SystemLog**: 系统日志
- **JWTToken**: 认证 Token

### 依赖库

- **Crow**: C++ Web 框架
- **nlohmann/json**: JSON 库
- **OpenSSL**: 加密库 (macOS) / Windows Crypto API (Windows)

## 🔧 编译选项

### macOS
```bash
clang++ -std=c++17 \
    -I/opt/homebrew/include \
    -I/opt/homebrew/Cellar/crow/1.3.0/include \
    main.cpp \
    -o main \
    -L/opt/homebrew/lib \
    -lssl \
    -lcrypto
```

### Windows (交叉编译)
```bash
x86_64-w64-mingw32-g++ -std=c++17 \
    -I/opt/homebrew/include \
    -I/opt/homebrew/Cellar/crow/1.3.0/include \
    -D_WIN32 \
    main.cpp \
    -o main.exe \
    -lws2_32 \
    -ladvapi32 \
    -lmswsock \
    -static-libgcc \
    -static-libstdc++ \
    -static
```

## 📊 性能特点

- **启动时间**: < 1 秒
- **内存占用**: ~50MB
- **并发处理**: 基于 Crow 的异步处理
- **响应时间**: < 10ms (平均)
- **文件大小**: ~8MB (macOS) / ~24MB (Windows)

## 🔒 安全特性

- **密码加密**: SHA256 哈希
- **Token 认证**: JWT Token，24小时有效期
- **权限控制**: 基于角色的访问控制
- **操作审计**: 完整的操作日志记录
- **数据备份**: 支持数据备份和恢复

## 🐛 常见问题

### Q: 编译时找不到 Crow 库？
A: 确保已安装 `brew install crow`

### Q: Windows 版本无法运行？
A: 检查防火墙设置，确保端口 21180 未被占用

### Q: 数据文件丢失？
A: 确保 `data/` 目录存在且包含所有 JSON 文件

### Q: Token 过期？
A: 重新登录获取新的 Token

### Q: 权限不足？
A: 使用管理员账号登录或联系管理员

## 🤝 贡献指南

1. Fork 项目
2. 创建特性分支
3. 提交更改
4. 推送到分支
5. 创建 Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情

## 📞 支持

如有问题或建议，请通过以下方式联系：
- 提交 Issue
- 发送邮件
- 创建 Pull Request

## 🙏 致谢

- [Crow](https://github.com/CrowCpp/Crow) - C++ Web 框架
- [nlohmann/json](https://github.com/nlohmann/json) - JSON 库
- [OpenSSL](https://www.openssl.org/) - 加密库

---

**注意**: 本项目为后端服务，需要配合前端使用。详细的 API 接口说明请参考 [API文档.md](API文档.md)。