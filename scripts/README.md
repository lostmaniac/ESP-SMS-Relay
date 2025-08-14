# 构建和发布脚本

本目录包含用于 ESP-SMS-Relay 项目构建、发布和维护的实用脚本。

## 📁 脚本列表

### 🚀 发布脚本

#### `release.bat` (Windows)
自动化版本发布脚本，用于创建新版本标签并触发 GitHub Actions 自动构建和发布。

**功能特性**:
- 检查 Git 仓库状态和工作目录清洁度
- 显示当前分支和最新标签信息
- 展示自上次发布以来的提交记录
- 版本号格式验证
- 自动创建和推送标签
- 触发 GitHub Actions 自动发布流程

**使用方法**:
```bash
# 在项目根目录运行
scripts\release.bat
```

**使用流程**:
1. 确保当前在 `main` 分支（推荐）
2. 确保工作目录干净（无未提交更改）
3. 运行脚本并按提示输入新版本号
4. 确认发布后自动创建标签并推送
5. GitHub Actions 自动开始构建和发布

#### `release.sh` (Linux/macOS)
Linux 和 macOS 版本的发布脚本，功能与 Windows 版本相同。

**使用方法**:
```bash
# 添加执行权限
chmod +x scripts/release.sh

# 运行脚本
./scripts/release.sh
```

### 🔍 构建检查脚本

#### `check-build.bat`
全面的构建环境和项目状态检查脚本。

**检查项目**:
- **开发环境**: PlatformIO 安装状态和版本
- **项目配置**: `platformio.ini` 和 GitHub Actions 工作流
- **项目依赖**: 库依赖安装状态
- **编译环境**: 语法检查和内存使用情况
- **Git 状态**: 分支、标签、提交状态
- **远程仓库**: GitHub 仓库信息

**使用方法**:
```bash
# 在项目根目录运行
scripts\check-build.bat
```

**输出信息**:
- 环境检查结果
- 编译状态和内存使用
- Git 仓库状态
- 构建建议和常用命令

## 📋 版本发布流程

### 标准发布流程

1. **开发完成**
   ```bash
   # 在 develop 分支完成功能开发
   git checkout develop
   git add .
   git commit -m "[feature] 新功能开发完成"
   git push origin develop
   ```

2. **合并到主分支**
   ```bash
   # 切换到 main 分支并合并
   git checkout main
   git merge develop
   git push origin main
   ```

3. **创建发布版本**
   ```bash
   # 使用发布脚本
   scripts\release.bat
   
   # 或手动创建标签
   git tag v1.2.0
   git push origin v1.2.0
   ```

4. **监控发布状态**
   - 访问 GitHub Actions 页面查看构建状态
   - 构建完成后检查 Releases 页面
   - 下载并测试发布的固件

### 版本号规范

遵循 [语义化版本](https://semver.org/lang/zh-CN/) 规范：

- **主版本号** (`v2.0.0`): 不兼容的 API 修改
- **次版本号** (`v1.1.0`): 向下兼容的功能性新增
- **修订号** (`v1.0.1`): 向下兼容的问题修正
- **预发布版本** (`v1.0.0-beta.1`): 测试版本
- **候选版本** (`v1.0.0-rc.1`): 发布候选版本

### 发布类型说明

| 版本类型 | 格式 | 说明 | 示例 |
|---------|------|------|------|
| 正式版本 | `v{major}.{minor}.{patch}` | 稳定的生产版本 | `v1.2.0` |
| 修复版本 | `v{major}.{minor}.{patch}` | 问题修复版本 | `v1.2.1` |
| 测试版本 | `v{version}-beta.{number}` | 功能测试版本 | `v1.3.0-beta.1` |
| 候选版本 | `v{version}-rc.{number}` | 发布候选版本 | `v1.3.0-rc.1` |

## 🛠️ 本地构建命令

### 基本构建命令

```bash
# 安装依赖
pio pkg install

# 清理构建
pio run --target clean

# 编译项目
pio run

# 详细编译信息
pio run --verbose

# 检查程序大小
pio run --target checkprogsize

# 编译并上传
pio run --target upload

# 上传文件系统
pio run --target uploadfs

# 串口监控
pio device monitor
```

### 高级构建选项

```bash
# 指定环境编译
pio run --environment esp32-s3-devkitm-1

# 指定端口上传
pio run --target upload --upload-port COM3

# 生成编译数据库
pio run --target compiledb

# 运行测试
pio test

# 静态代码分析
pio check
```

## 🔧 故障排除

### 常见问题

1. **PlatformIO 未找到**
   ```bash
   # 安装 PlatformIO
   pip install platformio
   
   # 或使用 PlatformIO IDE
   ```

2. **Git 命令失败**
   ```bash
   # 检查 Git 配置
   git config --list
   
   # 设置用户信息
   git config --global user.name "Your Name"
   git config --global user.email "your.email@example.com"
   ```

3. **编译失败**
   ```bash
   # 清理并重新编译
   pio run --target clean
   pio pkg install
   pio run --verbose
   ```

4. **依赖问题**
   ```bash
   # 更新依赖
   pio pkg update
   
   # 重新安装依赖
   pio pkg uninstall --all
   pio pkg install
   ```

### 脚本执行问题

1. **权限问题** (Linux/macOS)
   ```bash
   chmod +x scripts/*.sh
   ```

2. **路径问题**
   - 确保在项目根目录执行脚本
   - 检查脚本路径是否正确

3. **编码问题**
   - Windows: 确保使用 UTF-8 编码
   - 检查终端编码设置

## 📚 相关文档

- [CI/CD 指南](../docs/CI_CD_GUIDE.md)
- [项目 README](../README.md)
- [PlatformIO 文档](https://docs.platformio.org/)
- [GitHub Actions 文档](https://docs.github.com/en/actions)

## 🤝 贡献

如果您有改进脚本的建议或发现问题，请：

1. 创建 Issue 描述问题或建议
2. 提交 Pull Request 包含改进
3. 更新相关文档

## 📄 许可证

这些脚本遵循 ESP-SMS-Relay 项目的 MIT 许可证。