# GEMINI.md - ESP-SMS-Relay 项目

## 项目概述

ESP-SMS-Relay 是一个基于 ESP32-S3 的智能短信中继系统。其核心功能是通过 4G 蜂窝模块接收短信，并将其转发到多种消息平台。该系统设计目标是高可用性和高弹性，具备数据库存储、自动重试和电源管理等功能，可利用太阳能和电池实现离网独立运行。

项目使用 PlatformIO 和 C++ 的 Arduino 框架开发，并遵循模块化架构，将配置管理、数据库、日志记录、推送通知等功能拆分为独立的组件。

### 关键技术

*   **硬件:** ESP32-S3 (专为微雪 `Waveshare ESP32-S3-A7670E-4G` 开发板优化)
*   **框架:** Arduino
*   **构建系统:** PlatformIO
*   **编程语言:** C++
*   **核心库:**
    *   `ArduinoJson`: 用于解析和生成 Webhook 及 API 的 JSON 数据。
    *   `Sqlite3Esp32`: 用于在本地 SQLite 数据库中存储短信和配置信息。
    *   `pdulib`: 用于解码 PDU 格式的短信。

### 系统架构

应用程序被组织为多个管理器组件：

*   **`GsmService`:** 与 A7670E 4G 模块交互，处理蜂窝网络连接和短信收发。
*   **`SmsHandler`:** 处理接收到的短信。
*   **`PushManager`:** 管理向不同渠道（钉钉、企业微信、Webhook 等）的消息转发。
*   **`DatabaseManager`:** 负责短信数据和转发规则的存储与检索。
*   **`TerminalManager`:** 提供一个命令行界面（CLI），用于通过串口进行交互式管理。
*   **`WebServer`:** (规划中) 提供一个 Web 界面用于配置和监控。
*   **`FileSystemManager`:** 管理 LittleFS 文件系统上的文件。
*   **`LogManager`:** 提供集中的日志记录功能。

## 构建与运行

项目配置为使用 PlatformIO 进行构建和管理。当您在 VSCode 中使用 PlatformIO 插件时，可以通过以下方式执行相关操作。

### 环境设置

1.  **安装 VSCode 和 PlatformIO 插件:** 在 VSCode 的扩展市场中搜索并安装 `PlatformIO IDE`。
2.  **克隆项目:** `git clone <repository-url>`
3.  **安装依赖:** 首次构建项目时，PlatformIO 会自动安装 `platformio.ini` 文件中定义的所有依赖库。

### 核心操作

您可以在 VSCode 中打开 PlatformIO 的终端 (`终端` -> `新建终端`，然后选择 `PlatformIO CLI`) 来执行以下命令，或者直接使用 VSCode 侧边栏的 PlatformIO 图标和底部的状态栏按钮来操作。

*   **构建项目:**
    *   **命令:** `pio run`
    *   **VSCode UI:** 点击 PlatformIO 状态栏上的 **对勾 ✔️** 图标 (Build)。

*   **上传固件:**
    *   **命令:** `pio run --target upload`
    *   **VSCode UI:** 点击 PlatformIO 状态栏上的 **右箭头 ➡️** 图标 (Upload)。

*   **上传文件系统:**
    *   **命令:** `pio run --target uploadfs`
    *   **VSCode UI:** 在 PlatformIO 的项目任务列表中，选择 `env:esp32-s3-devkitm-1` -> `Platform` -> `Upload Filesystem Image`。

*   **串口监视器:**
    *   **命令:** `pio device monitor`
    *   **VSCode UI:** 点击 PlatformIO 状态栏上的 **插头 🔌** 图标 (Serial Port Monitor)。

*   **运行测试:**
    *   **命令:** `pio test`
    *   **VSCode UI:** 在 PlatformIO 的项目任务列表中，选择 `env:esp32-s3-devkitm-1` -> `Test`。

## 开发规范

*   **代码风格:** 代码遵循统一的风格和清晰的命名约定 (例如，类名使用 `PascalCase`，函数和变量名使用 `camelCase`)。
*   **模块化:** 项目高度模块化，每个主要功能都封装在自己的类（管理器）中，使代码更易于理解、维护和扩展。
*   **错误处理:** 代码包含错误处理和日志记录，以帮助诊断问题。
*   **配置管理:** 项目配置主要在 `platformio.ini` 和 `include/config.h` 文件中管理。
*   **代码测试:** 项目包含一个 `test` 目录，其中包含针对各个组件的单元测试。新功能应附带相应的测试。
*   **项目文档:** `docs` 目录包含了关于项目各项功能的详细文档。
*   **持续集成/持续部署 (CI/CD):** 项目使用 GitHub Actions 进行持续集成，并自动构建和发布新版本的固件。