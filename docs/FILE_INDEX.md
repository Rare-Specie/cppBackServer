# 📁 项目文件索引

## 📚 文档文件

### 核心文档
| 文件 | 说明 | 适用对象 |
|------|------|----------|
| [README.md](README.md) | 项目主文档，包含概述、快速开始 | 所有用户 |
| [QUICK_START.md](QUICK_START.md) | 一分钟快速入门指南 | 新用户 |
| [API文档.md](API文档.md) | 完整的 API 接口说明 | 开发者 |
| [CHANGELOG.md](CHANGELOG.md) | 版本变更历史 | 所有用户 |

### 平台特定文档
| 文件 | 说明 | 适用对象 |
|------|------|----------|
| [WINDOWS_BUILD.md](WINDOWS_BUILD.md) | Windows 编译详细指南 | Windows 用户 |
| [WINDOWS_CROSS_COMPILE_SUMMARY.md](WINDOWS_CROSS_COMPILE_SUMMARY.md) | 交叉编译技术总结 | 开发者 |

### 开发文档
| 文件 | 说明 | 适用对象 |
|------|------|----------|
| [CONTRIBUTING.md](CONTRIBUTING.md) | 贡献指南 | 贡献者 |
| [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) | 项目架构概览 | 开发者 |
| [FILE_INDEX.md](FILE_INDEX.md) | 本文件 - 文件索引 | 所有用户 |

### 法律文件
| 文件 | 说明 |
|------|------|
| [LICENSE](LICENSE) | MIT 许可证 |

## 🔧 脚本文件

### 编译脚本
| 文件 | 平台 | 说明 |
|------|------|------|
| [build.sh](build.sh) | macOS | macOS 编译脚本 |
| [build_windows.sh](build_windows.sh) | macOS → Windows | Windows 交叉编译脚本 |
| [verify_windows_build.sh](verify_windows_build.sh) | macOS | Windows 版本验证脚本 |

### 使用方法

#### macOS 开发
```bash
# 编译 macOS 版本
./build.sh

# 运行
./main
```

#### Windows 部署
```bash
# 编译 Windows 版本
./build_windows.sh

# 验证
./verify_windows_build.sh

# 部署到 Windows
# 复制 build_windows/ 目录
```

## 📂 源代码文件

### 主程序
| 文件 | 说明 |
|------|------|
| main.cpp | 程序入口，路由配置 |

### 头文件 (include/)
| 文件 | 说明 |
|------|------|
| models.h | 数据模型定义 |
| data_manager.h | 数据持久化管理 |
| auth.h | 认证与权限管理 |
| middleware.h | HTTP 中间件 |
| user_service.h | 用户管理服务 |
| student_service.h | 学生管理服务 |
| course_service.h | 课程管理服务 |
| grade_service.h | 成绩管理服务 |
| statistics_service.h | 统计分析服务 |
| report_service.h | 报表生成服务 |
| system_service.h | 系统管理服务 |

### 数据文件 (data/)
| 文件 | 说明 |
|------|------|
| users.json | 用户账号数据 |
| students.json | 学生基本信息 |
| courses.json | 课程信息 |
| grades.json | 成绩记录 |
| operation_logs.json | 用户操作日志 |
| system_logs.json | 系统运行日志 |
| backups.json | 备份记录 |
| settings.json | 系统设置 |
| tokens.json | 认证 Token |

## 📊 文档分类指南

### 🎯 我是新手
1. **开始**: [QUICK_START.md](QUICK_START.md)
2. **深入了解**: [README.md](README.md)
3. **API 使用**: [API文档.md](API文档.md)

### 💻 我是开发者
1. **项目架构**: [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md)
2. **API 详情**: [API文档.md](API文档.md)
3. **贡献代码**: [CONTRIBUTING.md](CONTRIBUTING.md)

### 🪟 我使用 Windows
1. **编译指南**: [WINDOWS_BUILD.md](WINDOWS_BUILD.md)
2. **技术细节**: [WINDOWS_CROSS_COMPILE_SUMMARY.md](WINDOWS_CROSS_COMPILE_SUMMARY.md)
3. **快速开始**: [QUICK_START.md](QUICK_START.md)

### 📋 我想了解变更
1. **版本历史**: [CHANGELOG.md](CHANGELOG.md)
2. **升级指南**: [CHANGELOG.md](CHANGELOG.md) - 升级指南部分

## 🔍 按功能查找

### 编译相关
- `build.sh` - macOS 编译
- `build_windows.sh` - Windows 编译
- `verify_windows_build.sh` - 验证编译结果

### 文档相关
- `README.md` - 总览
- `QUICK_START.md` - 快速开始
- `API文档.md` - API 参考
- `CONTRIBUTING.md` - 贡献指南

### 平台相关
- `WINDOWS_BUILD.md` - Windows 指南
- `WINDOWS_CROSS_COMPILE_SUMMARY.md` - 技术总结

### 项目管理
- `CHANGELOG.md` - 版本历史
- `LICENSE` - 许可证
- `PROJECT_OVERVIEW.md` - 架构说明

## 📈 文档完整性

| 类别 | 文档数量 | 完整度 |
|------|----------|--------|
| 用户文档 | 4 | ⭐⭐⭐⭐⭐ |
| 开发文档 | 4 | ⭐⭐⭐⭐⭐ |
| 平台文档 | 2 | ⭐⭐⭐⭐⭐ |
| 法律文件 | 1 | ⭐⭐⭐⭐⭐ |
| **总计** | **11** | **⭐⭐⭐⭐⭐** |

## 🎯 推荐阅读顺序

### 完整学习路径
1. [QUICK_START.md](QUICK_START.md) - 5分钟体验
2. [README.md](README.md) - 全面了解
3. [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) - 架构理解
4. [API文档.md](API文档.md) - API 掌握
5. [CONTRIBUTING.md](CONTRIBUTING.md) - 参与贡献

### Windows 用户路径
1. [QUICK_START.md](QUICK_START.md) - 快速了解
2. [WINDOWS_BUILD.md](WINDOWS_BUILD.md) - 编译部署
3. [README.md](README.md) - 功能详情
4. [API文档.md](API文档.md) - API 使用

### 开发者路径
1. [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) - 架构概览
2. [API文档.md](API文档.md) - API 设计
3. [CONTRIBUTING.md](CONTRIBUTING.md) - 贡献规范
4. [CHANGELOG.md](CHANGELOG.md) - 版本规划

## 📞 获取帮助

### 文档内搜索
使用 `Ctrl+F` (Windows) 或 `Cmd+F` (macOS) 在文档中搜索关键词

### 问题解决
1. 查看 [QUICK_START.md](QUICK_START.md) 常见问题
2. 搜索 [CHANGELOG.md](CHANGELOG.md) 已知问题
3. 检查 [CONTRIBUTING.md](CONTRIBUTING.md) 问题报告模板

### 社区支持
- 提交 Issue
- 创建 Pull Request
- 参与讨论

---

**提示**: 所有文档都相互链接，方便跳转阅读。