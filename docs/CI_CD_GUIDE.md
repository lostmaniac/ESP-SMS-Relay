# ESP-SMS-Relay CI/CD 使用指南

本文档介绍如何使用 GitHub Actions 进行 ESP-SMS-Relay 项目的自动构建和发布。

## 📋 概述

项目配置了两个主要的 GitHub Actions 工作流：

1. **持续集成 (CI)** - `ci.yml`：代码推送时进行快速验证
2. **构建和发布** - `build-and-release.yml`：完整构建固件并自动发布

## 🔄 工作流详情

### 1. 持续集成工作流 (ci.yml)

**触发条件：**
- 推送到 `main`、`develop`、`feature/*` 分支
- 向 `main`、`develop` 分支提交 Pull Request

**执行任务：**
- 代码风格检查
- 项目文件结构验证
- 依赖关系检查
- 编译语法验证

**特点：**
- 快速执行（约 2-3 分钟）
- 并发控制，避免重复构建
- 仅验证代码正确性，不生成固件

### 2. 构建和发布工作流 (build-and-release.yml)

**触发条件：**
- 推送到 `main`、`develop` 分支
- 推送标签（`v*` 格式）
- 向 `main` 分支提交 Pull Request

**执行任务：**

#### 构建阶段 (build)
1. 设置 Python 和 PlatformIO 环境
2. 缓存依赖以加速构建
3. 编译 ESP32-S3 固件
4. 生成版本信息和构建报告
5. 重命名固件文件（包含版本号）
6. 上传构建产物到 Artifacts

#### 发布阶段 (release) - 仅标签触发
1. 下载构建产物
2. 生成详细的发布说明
3. 创建 GitHub Release
4. 上传固件文件到 Release

#### 通知阶段 (notify)
1. 发送构建状态通知
2. 提供相关链接和信息

## 🏷️ 版本发布流程

### 自动发布（推荐）

1. **准备发布**
   ```bash
   # 确保代码已提交并推送
   git add .
   git commit -m "feat: 准备发布 v1.3.0"
   git push origin main
   ```

2. **创建标签**
   ```bash
   # 创建版本标签
   git tag v1.3.0
   git push origin v1.3.0
   ```

3. **自动处理**
   - GitHub Actions 自动触发构建
   - 编译固件文件
   - 创建 Release 页面
   - 上传固件到 Release

### 手动发布

1. **在 GitHub 网页创建标签**
   - 访问项目的 Releases 页面
   - 点击 "Create a new release"
   - 输入标签名（如 `v1.3.0`）
   - 填写发布说明
   - 发布后自动触发构建

## 📦 构建产物

### Artifacts（所有构建）
每次构建都会生成以下文件并上传到 GitHub Artifacts：

- `esp-sms-relay-{version}.bin` - 主固件文件
- `esp-sms-relay-{version}.elf` - 调试符号文件
- `build-info.txt` - 构建信息文档

**下载方式：**
1. 进入 Actions 页面
2. 选择对应的构建任务
3. 在页面底部下载 Artifacts

### Release（标签构建）
标签构建会额外创建 GitHub Release，包含：

- 固件文件（`.bin` 和 `.elf`）
- 详细的发布说明
- 安装和使用指南
- 硬件支持信息

## 🔧 配置说明

### 环境要求

- **Python**: 3.11
- **PlatformIO**: 最新版本
- **目标平台**: ESP32-S3
- **构建环境**: Ubuntu Latest

### 缓存策略

为了加速构建，工作流使用了以下缓存：

- PlatformIO 核心文件
- 已下载的库和工具链
- 编译缓存

缓存键基于 `platformio.ini` 文件的哈希值，确保配置变更时重新构建。

### 版本命名规则

- **标签构建**: 使用标签名作为版本号（如 `v1.3.0`）
- **开发构建**: 使用格式 `dev-YYYYMMDD-{commit-hash}`

## 🚀 使用建议

### 开发流程

1. **功能开发**
   ```bash
   git checkout -b feature/new-feature
   # 开发代码...
   git push origin feature/new-feature
   ```
   - 推送后自动触发 CI 检查

2. **合并到开发分支**
   ```bash
   git checkout develop
   git merge feature/new-feature
   git push origin develop
   ```
   - 触发完整构建，生成开发版固件

3. **发布准备**
   ```bash
   git checkout main
   git merge develop
   git push origin main
   ```
   - 生成候选发布版本

4. **正式发布**
   ```bash
   git tag v1.x.x
   git push origin v1.x.x
   ```
   - 自动创建 Release

### 标签命名规范

- **正式版本**: `v1.0.0`、`v1.1.0`、`v2.0.0`
- **预发布版本**: `v1.0.0-beta.1`、`v1.0.0-rc.1`
- **修复版本**: `v1.0.1`、`v1.0.2`

### 发布说明编写

自动生成的发布说明包含：

- 版本信息和构建时间
- 下载文件说明
- 安装指南
- 硬件支持信息
- 主要功能列表

如需自定义发布说明，可以在创建标签时手动编辑。

## 🔍 故障排除

### 常见问题

1. **构建失败**
   - 检查 `platformio.ini` 配置
   - 确认所有依赖库可用
   - 查看构建日志中的错误信息

2. **Release 创建失败**
   - 确认标签格式正确（以 `v` 开头）
   - 检查 GitHub Token 权限
   - 确认仓库设置允许创建 Release

3. **缓存问题**
   - 可以在 Actions 页面手动清除缓存
   - 修改 `platformio.ini` 会自动更新缓存

### 调试方法

1. **查看构建日志**
   - 进入 Actions 页面
   - 点击失败的构建任务
   - 展开相应的步骤查看详细日志

2. **本地测试**
   ```bash
   # 本地验证构建
   pio run --environment esp32-s3-devkitm-1
   ```

3. **检查文件**
   ```bash
   # 检查生成的固件文件
   ls -la .pio/build/esp32-s3-devkitm-1/
   ```

## 📊 监控和统计

### 构建状态徽章

可以在 README 中添加构建状态徽章：

```markdown
![Build Status](https://github.com/your-username/ESP-SMS-Relay/workflows/Build%20and%20Release%20ESP32%20Firmware/badge.svg)
![CI Status](https://github.com/your-username/ESP-SMS-Relay/workflows/Continuous%20Integration/badge.svg)
```

### 构建历史

- 在 Actions 页面可以查看所有构建历史
- 每个构建都有详细的日志和时间记录
- 可以重新运行失败的构建

## 🔒 安全考虑

- 使用 GitHub 提供的 `GITHUB_TOKEN`，无需额外配置
- 构建环境是隔离的，每次都是全新环境
- 敏感信息不会暴露在日志中
- 所有操作都有完整的审计日志

## 📚 相关资源

- [GitHub Actions 官方文档](https://docs.github.com/en/actions)
- [PlatformIO CI/CD 指南](https://docs.platformio.org/en/latest/integration/ci/index.html)
- [ESP32-S3 开发指南](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)

---

**注意**: 首次使用时，请确保仓库设置中启用了 Actions 功能，并且有足够的 Actions 使用配额。