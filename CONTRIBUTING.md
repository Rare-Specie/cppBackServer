# 🤝 贡献指南

感谢您对本项目的关注！我们欢迎所有形式的贡献，包括 bug 修复、新功能、文档改进等。

## 📋 目录

- [如何贡献](#如何贡献)
- [开发环境设置](#开发环境设置)
- [代码规范](#代码规范)
- [提交规范](#提交规范)
- [Pull Request 流程](#pull-request-流程)
- [报告问题](#报告问题)
- [请求新功能](#请求新功能)

## 🤔 如何贡献

### 1. 报告 Bug
如果您发现了 bug，请：
- 检查是否已有相似的 issue
- 提供详细的复现步骤
- 说明您的操作系统和环境
- 附上错误信息或日志

### 2. 提交代码
```
Fork 项目 → 创建分支 → 提交代码 → 创建 PR
```

### 3. 改进文档
- 修正错别字
- 补充说明
- 添加示例
- 翻译文档

### 4. 分享建议
- 功能建议
- 性能优化
- 架构改进

## 🛠️ 开发环境设置

### 前置要求

#### macOS 开发环境
```bash
# 1. 安装 Xcode Command Line Tools
xcode-select --install

# 2. 安装 Homebrew (如果未安装)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 3. 安装依赖
brew install crow nlohmann-json openssl

# 4. 克隆项目
git clone https://github.com/your-username/cppBackServer.git
cd cppBackServer

# 5. 编译测试
./build.sh
./main
```

#### Windows 开发环境 (通过交叉编译)
```bash
# 1. 安装 Homebrew (macOS)
# 2. 安装 mingw-w64
brew install mingw-w64

# 3. 克隆项目
git clone https://github.com/your-username/cppBackServer.git
cd cppBackServer

# 4. 编译 Windows 版本
./build_windows.sh

# 5. 验证
./verify_windows_build.sh
```

### 项目结构说明

```
cppBackServer/
├── main.cpp              # 主程序 - 路由配置
├── include/              # 头文件目录
│   ├── models.h         # 数据模型
│   ├── data_manager.h   # 数据管理
│   ├── auth.h           # 认证逻辑
│   ├── middleware.h     # 中间件
│   └── *_service.h      # 业务服务
├── data/                # 数据文件
├── build.sh             # macOS 编译
├── build_windows.sh     # Windows 编译
└── API文档.md           # API 文档
```

## 📝 代码规范

### C++ 代码风格

#### 命名规范
```cpp
// 类名：大驼峰
class UserManager { };

// 函数名：小驼峰
std::string getUserRole();

// 变量名：小驼峰
std::string userName;

// 常量：全大写 + 下划线
const std::string JWT_SECRET = "secret";

// 结构体：大驼峰
struct OperationLog { };
```

#### 代码格式
```cpp
// 使用 4 空格缩进
void exampleFunction() {
    if (condition) {
        // 4 空格缩进
        doSomething();
    }
}

// 指针和引用
void func(const std::string& str, User* user);

// 模板和 lambda
auto lambda = [](const User& u) {
    return u.role == "admin";
};
```

#### 注释规范
```cpp
// 单行注释使用 //

/* 
 * 多行注释用于函数说明
 * 参数说明
 * 返回值说明
 */

// 类说明
class AuthManager {
private:
    // 私有成员说明
    DataManager* dataManager;
    
public:
    // 公共方法说明
    std::string sha256(const std::string& str);
};
```

### 头文件规范

```cpp
#ifndef HEADER_NAME_H
#define HEADER_NAME_H

// 标准库头文件
#include <string>
#include <vector>

// 第三方库头文件
#include <nlohmann/json.hpp>

// 项目头文件
#include "models.h"

// 使用声明
using json = nlohmann::json;

// 代码...

#endif // HEADER_NAME_H
```

### JSON 数据格式

```json
{
  "id": "unique_id",
  "name": "名称",
  "createdAt": "2026-01-12 14:00:00",
  "updatedAt": "2026-01-12 14:00:00"
}
```

## 📤 提交规范

### Commit Message 格式

```
类型(范围): 简短描述

详细描述（可选）

BREAKING CHANGE: 重大变更说明
```

### 类型定义

- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式调整
- `refactor`: 代码重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具相关

### 示例

```
feat(auth): 添加 OAuth2 支持

支持 Google 和 GitHub 登录
添加 OAuth 配置文件

BREAKING CHANGE: 登录 API 参数变更
```

```
fix(data_manager): 修复文件读写并发问题

在多线程环境下可能导致数据损坏
添加互斥锁保护
```

## 🔄 Pull Request 流程

### 1. Fork 项目
```bash
git clone https://github.com/your-username/cppBackServer.git
cd cppBackServer
```

### 2. 创建特性分支
```bash
git checkout -b feature/amazing-feature
```

### 3. 提交代码
```bash
# 添加修改
git add .

# 提交
git commit -m "feat: 添加新功能"

# 推送
git push origin feature/amazing-feature
```

### 4. 创建 PR
1. 访问 GitHub 仓库
2. 点击 "Compare & pull request"
3. 填写 PR 描述
4. 等待代码审查

### 5. PR 要求
- ✅ 代码符合规范
- ✅ 通过编译测试
- ✅ 更新相关文档
- ✅ 添加必要注释
- ✅ 说明变更内容

## 🐛 报告问题

### Bug 报告模板

```markdown
**描述 Bug**
清晰简洁地描述问题

**复现步骤**
1. 进入 '...'
2. 点击 '....'
3. 滚动到 '....'
4. 出现错误

**预期行为**
描述应该发生什么

**实际行为**
描述实际发生了什么

**环境信息**
- OS: [例如: macOS 14.0]
- 版本: [例如: v1.0.0]
- 编译器: [例如: clang++ 15.0]

**附加信息**
截图、日志等
```

## 💡 请求新功能

### 功能请求模板

```markdown
**功能描述**
清晰描述你想要的功能

**使用场景**
这个功能解决了什么问题？

**替代方案**
你考虑过其他解决方案吗？

**额外上下文**
添加任何相关的截图或说明
```

## 🎯 开发任务

### 新手友好任务
- [ ] 添加 API 错误码常量
- [ ] 补充函数文档注释
- [ ] 添加单元测试示例
- [ ] 优化 README 结构
- [ ] 添加代码覆盖率工具

### 进阶任务
- [ ] 实现 WebSocket 支持
- [ ] 添加数据库支持 (SQLite)
- [ ] 实现文件上传功能
- [ ] 添加 API 限流
- [ ] 实现缓存层

### 专家任务
- [ ] 重构为微服务架构
- [ ] 添加 gRPC 支持
- [ ] 实现插件系统
- [ ] 添加 GraphQL 接口

## 🧪 测试

### 运行测试
```bash
# macOS
./build.sh && ./main

# Windows
./build_windows.sh
# 复制到 Windows 运行
```

### 手动测试清单
- [ ] 用户登录/登出
- [ ] 权限验证
- [ ] 学生 CRUD
- [ ] 课程 CRUD
- [ ] 成绩 CRUD
- [ ] 统计分析
- [ ] 报表生成
- [ ] 数据备份/恢复

## 📚 文档贡献

### 文档类型
1. **API 文档** - 接口说明
2. **用户指南** - 使用教程
3. **开发者文档** - 架构说明
4. **部署文档** - 安装指南
5. **FAQ** - 常见问题

### 文档标准
- 清晰的结构
- 准确的示例
- 完整的说明
- 中英文对照（可选）

## 🎭 代码审查清单

### 审查者请检查
- [ ] 代码符合项目规范
- [ ] 功能完整且正确
- [ ] 没有引入新的依赖
- [ ] 文档已更新
- [ ] 测试覆盖充分
- [ ] 性能影响评估
- [ ] 安全性考虑

### 提交者请确认
- [ ] 本地编译通过
- [ ] 代码格式化
- [ ] 注释清晰
- [ ] 提交信息规范
- [ ] 更新了 CHANGELOG

## 📊 贡献统计

### 贡献者徽章
- 🥇 核心贡献者
- 🥈 功能贡献者
- 🥉 文档贡献者
- 🐛 Bug 猎人
- 📚 文档维护者

### 贡献价值
- 代码质量
- 功能完整性
- 文档完善度
- 社区活跃度

## 📄 许可证

本项目采用 MIT 许可证，贡献代码即表示您同意此许可证。

## 🙏 感谢

感谢所有贡献者的付出！🎉

---

**问题？** 请在 Issue 中提问  
**讨论？** 欢迎在 Discussion 中交流  
**帮助？** 查看 [README.md](README.md) 和 [API文档.md](API文档.md)