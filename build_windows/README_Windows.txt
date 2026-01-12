Windows 版本运行说明
=====================

1. 运行环境要求：
   - Windows 10 或更高版本（64位）
   - 无需额外安装运行时库（已静态链接）

2. 运行方法：
   - 双击 main.exe 运行
   - 或在命令行中运行: main.exe

3. 访问 API：
   - 默认监听地址: http://localhost:21180
   - API 文档请参考项目根目录的 API文档.md

4. 数据目录：
   - 程序会在当前目录下寻找 data 文件夹
   - 请确保 data 目录及其中的 JSON 文件存在
   - data 目录结构：
     data/
     ├── users.json
     ├── students.json
     ├── courses.json
     ├── grades.json
     ├── operation_logs.json
     ├── system_logs.json
     ├── backups.json
     ├── settings.json
     └── tokens.json

5. 注意事项：
   - 防火墙可能会阻止程序运行，请允许程序通过防火墙
   - 如果需要修改端口，请修改源代码中的端口设置
   - 确保 data 目录有读写权限
   - 程序会在首次运行时自动创建必要的数据文件

6. 常见问题：
   - 如果无法访问 API，检查防火墙设置
   - 如果提示端口被占用，修改端口或关闭占用程序
   - 如果数据读取错误，检查 data 目录权限和文件格式

7. 技术说明：
   - 使用 Windows Crypto API 进行 SHA256 哈希
   - 静态链接所有依赖，便于部署
   - 支持跨平台数据文件格式

