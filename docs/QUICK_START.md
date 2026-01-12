# 🚀 快速入门指南

## 一分钟启动

### macOS 用户

```bash
# 1. 安装依赖
brew install crow nlohmann-json openssl

# 2. 编译
./build.sh

# 3. 运行
./main
```

### Windows 用户

```bash
# 1. 编译 Windows 版本 (在 macOS 上)
./build_windows.sh

# 2. 复制 build_windows/ 到 Windows
# 3. 在 Windows 上双击 main.exe 运行
```

## 📱 立即测试

### 1. 登录获取 Token
```bash
curl -X POST http://localhost:21180/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123","role":"admin"}'
```

### 2. 查看学生列表
```bash
# 替换 YOUR_TOKEN 为上一步获取的 token
curl -X GET http://localhost:21180/api/students \
  -H "Authorization: Bearer YOUR_TOKEN"
```

## 🎯 默认账号

| 角色 | 用户名 | 密码 |
|------|--------|------|
| 管理员 | admin | admin123 |
| 教师 | teacher | teacher123 |
| 学生 | student | student123 |

## 📋 必需文件

确保 `data/` 目录包含以下文件：
- users.json
- students.json
- courses.json
- grades.json
- operation_logs.json
- system_logs.json
- backups.json
- settings.json
- tokens.json

## 🔍 验证运行状态

访问以下地址检查服务是否正常：
- API 根地址: http://localhost:21180/api
- API 文档: 查看 API文档.md

## ⚠️ 常见问题

**无法访问 API？**
- 检查防火墙设置
- 确认端口 21180 未被占用
- 确保程序正在运行

**编译失败？**
- macOS: 确认已安装 Xcode Command Line Tools
- Windows: 确认已安装 mingw-w64

**数据读取错误？**
- 检查 data/ 目录是否存在
- 确认 JSON 文件格式正确

## 📚 更多帮助

- 完整文档: [README.md](README.md)
- API 详情: [API文档.md](API文档.md)
- Windows 指南: [WINDOWS_BUILD.md](WINDOWS_BUILD.md)