# ESP-SMS-Relay 项目

[![Build and Release](https://github.com/your-username/ESP-SMS-Relay/workflows/Build%20and%20Release%20ESP32%20Firmware/badge.svg)](https://github.com/your-username/ESP-SMS-Relay/actions/workflows/build-and-release.yml)
[![CI Status](https://github.com/your-username/ESP-SMS-Relay/workflows/Continuous%20Integration/badge.svg)](https://github.com/your-username/ESP-SMS-Relay/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/your-username/ESP-SMS-Relay)](https://github.com/your-username/ESP-SMS-Relay/releases)
[![License](https://img.shields.io/github/license/your-username/ESP-SMS-Relay)](LICENSE)

> **🚀 自动构建**: 代码推送后自动编译固件，打tag自动发布到Release  
> **📦 即用固件**: 在 [Releases](https://github.com/your-username/ESP-SMS-Relay/releases) 页面下载最新编译好的固件

## 项目概述

ESP-SMS-Relay 是一个基于 ESP32-S3 的智能短信中继系统，具备短信接收、转发、数据库管理、多平台推送等功能。系统采用模块化架构设计，支持企业微信、钉钉、自定义Webhook等多种推送方式，并提供完整的Web管理界面和CLI终端管理功能。

**🌟 核心亮点**：基于微雪ESP32-S3-A7670E-4G开发板，支持太阳能板+锂电池供电，真正实现离网独立运行，只要有阳光就能持续工作，无需依赖任何外部电源和网络基础设施。

## 应用场景

### 🏢 企业级短信管理平台
- **多平台机器人集成**：支持钉钉群机器人、企业微信群机器人、飞书群机器人
- **权限隔离管理**：不同部门、项目组可独立配置推送权限和接收范围
- **24小时在线转发**：无人值守自动转发，确保重要信息及时送达
- **企业账号统一管理**：支持注册和管理各种第三方平台账号

### 🔐 企业安全管理
- **防离职风险**：避免员工离职后手机号交接不及时导致的信息泄露
- **授权随时收回**：管理员可随时撤销或调整手机号的使用权限
- **一号多用**：一个手机号码可同时为多个平台、多个项目组服务
- **访问控制**：基于角色的权限管理，确保信息安全

### 📱 多场景应用支持
- **验证码转发**：各类平台注册、登录验证码统一管理
- **告警信息分发**：系统监控、设备故障等告警信息及时推送
- **业务通知转发**：订单状态、支付通知等业务消息自动分发
- **应急通信保障**：关键业务的备用通信渠道

## 核心功能

### 🌞 绿色能源特性
- **太阳能供电**：支持5-6V太阳能板直接供电，绿色环保
- **锂电池备电**：18650锂电池槽，夜间或阴天持续工作
- **智能充电管理**：自动充放电控制，延长电池寿命
- **离网独立运行**：无需外部电源，真正的自给自足系统
- **低功耗优化**：多级功耗管理，最大化续航时间

### 📱 短信管理
- 自动接收和解析短信
- 智能转发规则匹配
- 短信数据库存储和管理
- 自动数据库清理（保持1万条记录以内）

### 🔄 多平台推送
- **企业微信机器人**：支持富文本消息推送
- **钉钉机器人**：支持Markdown格式消息
- **自定义Webhook**：灵活的HTTP推送接口
- **模板化消息**：支持变量替换和自定义格式

### 🌐 网络管理
- 4G网络连接（A7670E模组）
- WiFi连接管理和AP模式
- Web服务器和RESTful API
- HTTP客户端和网络请求管理
- 网络状态监控和自动重连

### 💾 数据管理
- SQLite数据库存储
- LittleFS文件系统管理
- 配置文件管理
- 数据备份和恢复

### 🛠️ 系统管理
- 模块化架构设计
- 定时任务调度器
- 系统日志管理
- CLI终端管理界面
- 系统状态监控

## 硬件要求

### 推荐硬件平台
- **微雪 ESP32-S3-A7670E-4G 开发板**（推荐）
- **ESP32-S3R2** 主控芯片
- **外部 16MB Flash + 2MB PSRAM**
- **板载 A7670E 4G通讯模组**

### 核心特性
- ✅ **太阳能供电支持**：板载太阳能充电接口，支持5-6V太阳能板输入
- ✅ **锂电池供电**：内置18650电池槽，支持锂电池供电和充电管理
- ✅ **离网运行**：太阳能+锂电池组合，无需外部电源和网络即可独立运行
- ✅ **4G网络**：板载A7670E模组，支持2G/3G/4G网络，无需WiFi依赖
- ✅ **低功耗设计**：多种低功耗工作状态，适合户外长期部署
- ✅ **扩展接口**：OV摄像头接口、TF卡槽、RGB炫彩灯等丰富外设

### 替代方案
- **ESP32-S3-DevKitM-1** 开发板 + 外接GSM模块
- **PSRAM**: 8MB（必需）
- **Flash**: 16MB（推荐）

### GSM模块
- **SIMCom A7670E**（推荐，板载）
- **SIMCom A7670C** 或兼容模块
- 支持2G/3G/4G网络
- UART通信接口

### 连接配置

#### 微雪 ESP32-S3-A7670E-4G（推荐）
- **A7670E模组已板载集成**，无需额外连线
- **太阳能接口**：支持5-6V太阳能板直接连接
- **电池接口**：18650锂电池槽，支持充放电管理
- **USB接口**：TYPE-C接口，支持ESP32-S3和4G模组USB切换

#### 自制方案连接
```
ESP32-S3    <->    SIMCom A7670C/E
GPIO17      <->    TXD
GPIO18      <->    RXD
GPIO19      <->    RI (Ring Indicator)
GPIO20      <->    DTR (Data Terminal Ready)
3.3V        <->    VCC
GND         <->    GND
```

## 开发环境配置

### 1. 安装PlatformIO

#### 方法一：VSCode插件安装
1. 安装 [Visual Studio Code](https://code.visualstudio.com/)
2. 安装 PlatformIO IDE 插件
3. 重启 VSCode

#### 方法二：命令行安装
```bash
# 安装Python（如果未安装）
# 安装PlatformIO Core
pip install platformio
```

### 2. 克隆项目
```bash
git clone <项目地址>
cd ESP-SMS-Relay
```

### 3. 安装依赖
```bash
# PlatformIO会自动安装以下依赖库：
# - pdulib: PDU格式短信解析
# - ArduinoJson: JSON数据处理
# - Sqlite3Esp32: SQLite数据库支持
pio lib install
```

## 编译和烧录

### 1. 编译项目
```bash
# 清理构建文件
pio run --target clean

# 编译项目
pio run

# 编译并显示详细信息
pio run --verbose
```

### 2. 烧录固件
```bash
# 自动检测端口并烧录
pio run --target upload

# 指定端口烧录（Windows）
pio run --target upload --upload-port COM3

# 指定端口烧录（Linux/Mac）
pio run --target upload --upload-port /dev/ttyUSB0
```

### 3. 烧录文件系统
```bash
# 烧录LittleFS文件系统（如果有data目录文件）
pio run --target uploadfs
```

### 4. 一键编译烧录
```bash
# 编译并烧录固件和文件系统
pio run --target upload && pio run --target uploadfs
```

## 调试和监控

### 1. 串口监控
```bash
# 启动串口监控
pio device monitor

# 指定波特率
pio device monitor --baud 115200

# 指定端口和波特率
pio device monitor --port COM3 --baud 115200
```

### 2. Xshell调试配置

#### 连接设置
1. 打开Xshell，创建新会话
2. 协议：`Serial`
3. 端口：选择ESP32对应的COM端口（如COM3）
4. 波特率：`115200`
5. 数据位：`8`
6. 停止位：`1`
7. 奇偶校验：`None`
8. 流控制：`None`

#### 终端设置
1. 终端类型：`xterm`
2. 编码：`UTF-8`
3. 回车键：`CR+LF`
4. 退格键：`ASCII DEL`

#### 调试技巧
```bash
# 系统启动后，可以使用以下CLI命令：

# 查看系统状态
status

# 查看短信记录
sms list

# 查看转发规则
rule list

# 测试推送配置
test push wechat

# 查看帮助
help
```

### 3. 日志级别配置

在 `include/config.h` 中配置日志级别：
```cpp
// 日志级别定义
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

// 设置当前日志级别
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
```

## 项目架构

### 模块化设计

项目采用严格的模块化架构，每个模块负责特定功能：

```
ESP-SMS-Relay/
├── src/                    # 主程序入口
│   └── main.cpp           # 系统启动和主循环
├── lib/                    # 功能模块库
│   ├── system_init/       # 系统初始化
│   ├── module_manager/    # 模块生命周期管理
│   ├── config_manager/    # 配置管理
│   ├── database_manager/  # SQLite数据库管理
│   ├── filesystem_manager/# LittleFS文件系统
│   ├── wifi_manager/      # WiFi连接管理
│   ├── web_server/        # Web服务器
│   ├── http_client/       # HTTP客户端
│   ├── gsm_service/       # GSM模块通信
│   ├── sms_handler/       # 短信处理
│   ├── sms_sender/        # 短信发送
│   ├── push_manager/      # 推送管理
│   ├── phone_caller/      # 电话功能
│   ├── terminal_manager/  # CLI终端管理
│   ├── task_scheduler/    # 定时任务调度
│   ├── log_manager/       # 日志管理
│   ├── uart_monitor/      # 串口监控
│   ├── uart_dispatcher/   # 串口调度
│   ├── at_command_handler/# AT命令处理
│   ├── carrier_config/    # 运营商配置
│   └── network_config/    # 网络配置
├── include/               # 全局头文件
│   └── config.h          # 系统配置
├── docs/                  # 项目文档
├── test/                  # 测试代码
├── data/                  # 数据文件（可选）
├── partitions.csv         # 分区表
└── platformio.ini         # 项目配置
```

### 核心模块说明

#### 系统管理模块
- **system_init**: 系统启动和初始化流程控制
- **module_manager**: 模块注册、初始化和生命周期管理
- **config_manager**: 配置文件读写和管理
- **log_manager**: 系统日志记录和管理
- **task_scheduler**: 定时任务调度和管理

#### 通信模块
- **wifi_manager**: WiFi连接、AP模式、网络状态管理
- **web_server**: HTTP服务器、RESTful API、静态文件服务
- **http_client**: HTTP客户端、请求发送、响应处理
- **gsm_service**: GSM模块初始化、网络注册、状态监控
- **uart_monitor**: 串口数据监控和解析
- **uart_dispatcher**: 串口数据分发和路由
- **at_command_handler**: AT命令发送、接收和解析

#### 业务模块
- **sms_handler**: 短信接收、解析、转发逻辑
- **sms_sender**: 短信发送功能
- **push_manager**: 多平台推送管理（企业微信、钉钉、Webhook）
- **phone_caller**: 电话拨打、接听、通话管理
- **terminal_manager**: CLI终端界面、命令处理

#### 数据管理模块
- **database_manager**: SQLite数据库操作、事务管理、数据清理
- **filesystem_manager**: LittleFS文件系统管理、文件操作

#### 配置模块
- **carrier_config**: 运营商相关配置和管理
- **network_config**: 网络参数配置和管理

## 配置说明

### 1. 硬件配置 (include/config.h)

```cpp
// SIMCom模块UART配置
#define SIM_SERIAL_NUM 1          // 串口号
#define SIM_SERIAL_BAUD 115200    // 波特率
#define SIM_SERIAL_RX_PIN 17      // RX引脚
#define SIM_SERIAL_TX_PIN 18      // TX引脚
#define SIM_RI_PIN 19             // RI引脚
#define SIM_DTR_PIN 20            // DTR引脚
```

### 2. 系统配置

```cpp
// 数据库配置
#define SMS_CLEANUP_THRESHOLD 10000    // 触发清理的阈值
#define SMS_CLEANUP_KEEP_COUNT 10000   // 清理后保留的记录数
#define SMS_CLEANUP_INTERVAL (24*60*60*1000)  // 清理间隔（毫秒）

// WiFi配置
#define WIFI_CONNECT_TIMEOUT 30000     // WiFi连接超时（毫秒）
#define WIFI_RETRY_INTERVAL 5000       // 重连间隔（毫秒）

// Web服务器配置
#define WEB_SERVER_PORT 80             // Web服务器端口
#define API_TIMEOUT 30000              // API超时时间
```

### 3. 分区表配置 (partitions.csv)

```csv
# Name,   Type, SubType, Offset,  Size,     Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x320000,
littlefs, data, spiffs,  0x330000,0xCB000,
coredump, data, coredump,0x3FB000,0x5000,
```

## API接口文档

### 1. Web API接口

#### 系统状态
```http
GET /api/status
```

#### 短信管理
```http
# 获取短信列表
GET /api/sms?limit=50&offset=0

# 发送短信
POST /api/sms/send
Content-Type: application/json
{
  "phone": "+8613800138000",
  "content": "测试短信内容"
}
```

#### 转发规则管理
```http
# 获取转发规则
GET /api/rules

# 添加转发规则
POST /api/rules
Content-Type: application/json
{
  "name": "规则名称",
  "phone_filter": "+86138",
  "keyword_filter": "验证码,通知",
  "push_type": "wechat",
  "push_config": "{\"webhook_url\":\"https://...\",\"template\":\"...\"}",
  "enabled": true
}

# 更新转发规则
PUT /api/rules/{id}

# 删除转发规则
DELETE /api/rules/{id}
```

### 2. CLI命令接口

#### 系统命令
```bash
status              # 查看系统状态
restart             # 重启系统
factory_reset       # 恢复出厂设置
```

#### 短信命令
```bash
sms list [limit]    # 查看短信列表
sms send <phone> <content>  # 发送短信
sms clear           # 清空短信记录
```

#### 规则命令
```bash
rule list           # 查看转发规则
rule add <name> <phone_filter> <keyword_filter> <push_type> <push_config>
rule enable <id>    # 启用规则
rule disable <id>   # 禁用规则
rule delete <id>    # 删除规则
```

#### 测试命令
```bash
test push <type>    # 测试推送配置
test sms <phone>    # 测试短信发送
test network        # 测试网络连接
```

## 推送配置示例

### 1. 企业微信机器人

```json
{
  "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY",
  "template": "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}"
}
```

### 2. 钉钉机器人

```json
{
  "webhook_url": "https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN",
  "template": "## 📱 短信通知\n\n**发送方:** {sender}\n\n**时间:** {timestamp}\n\n**内容:** {content}"
}
```

### 3. 自定义Webhook

```json
{
  "webhook_url": "https://your-api.com/webhook",
  "method": "POST",
  "content_type": "application/json",
  "headers": "Authorization:Bearer TOKEN,X-Source:ESP-SMS-Relay",
  "body_template": "{\"event\":\"sms_received\",\"data\":{\"from\":\"{sender}\",\"content\":\"{content}\",\"timestamp\":\"{timestamp}\"}}"
}
```

## 开发规范

### 1. 代码规范

#### 命名规范
- **类名**: PascalCase (如 `DatabaseManager`)
- **函数名**: camelCase (如 `initialize()`)
- **变量名**: camelCase (如 `systemStatus`)
- **常量名**: UPPER_SNAKE_CASE (如 `SIM_SERIAL_NUM`)
- **文件名**: snake_case (如 `database_manager.h`)

#### 注释规范
```cpp
/**
 * @file filename.h
 * @brief 文件简要描述
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

/**
 * @class ClassName
 * @brief 类的简要描述
 */

/**
 * @brief 函数简要描述
 * @param param1 参数1描述
 * @return 返回值描述
 */
```

### 2. 模块开发规范

#### 模块接口设计
```cpp
class ModuleManager {
public:
    // 单例获取
    static ModuleManager& getInstance();
    
    // 初始化
    bool initialize();
    
    // 核心功能
    bool doSomething();
    
    // 状态查询
    bool isReady();
    
    // 错误处理
    String getLastError();
    
    // 清理资源
    void cleanup();
    
private:
    ModuleManager();
    ~ModuleManager();
    
    // 禁止拷贝
    ModuleManager(const ModuleManager&) = delete;
    ModuleManager& operator=(const ModuleManager&) = delete;
};
```

### 3. 错误处理规范

- 所有公共接口必须有错误处理
- 使用枚举定义错误类型
- 提供 `getLastError()` 方法
- 关键操作失败不应导致系统崩溃

### 4. 内存管理规范

- 及时释放动态分配的内存
- 使用RAII原则管理资源
- 避免内存泄漏
- 合理使用栈内存和堆内存

## 测试和验证

### 1. 单元测试

```bash
# 运行所有测试
pio test

# 运行特定测试
pio test --filter test_database_manager
```

### 2. 功能测试

#### 短信接收测试
1. 向SIM卡发送短信
2. 检查串口日志确认接收
3. 验证数据库存储
4. 确认转发规则匹配

#### 推送功能测试
1. 配置推送规则
2. 发送测试短信
3. 验证推送消息接收
4. 检查推送日志

#### Web界面测试
1. 连接WiFi网络
2. 访问Web管理界面
3. 测试API接口
4. 验证配置修改

### 3. 性能测试

#### 内存使用监控
```cpp
// 在代码中添加内存监控
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
```

#### 数据库性能测试
```bash
# 批量插入测试
test db insert 1000

# 查询性能测试
test db query 100

# 清理性能测试
test db cleanup
```

## 故障排除

### 1. 常见问题

#### 编译错误
```bash
# 清理构建缓存
pio run --target clean

# 重新安装依赖
pio lib uninstall --all
pio lib install

# 检查平台版本
pio platform update
```

#### 烧录失败
```bash
# 检查端口
pio device list

# 手动进入下载模式
# 按住BOOT键，按下RESET键，松开RESET键，松开BOOT键

# 擦除Flash
esptool.py --chip esp32s3 --port COM3 erase_flash
```

#### 运行时错误
```bash
# 检查串口输出
pio device monitor --baud 115200

# 检查分区表
esptool.py --chip esp32s3 --port COM3 read_flash 0x8000 0x1000 partition_table.bin

# 检查文件系统
# 在CLI中执行: fs status
```

### 2. 调试技巧

#### 启用调试模式
```cpp
// 在main.cpp中启用调试
#define DEBUG_MODE 1

// 启用模块调试
DatabaseManager::getInstance().setDebugMode(true);
PushManager::getInstance().setDebugMode(true);
```

#### 日志分析
```bash
# 过滤特定模块日志
pio device monitor | grep "[DATABASE]"

# 保存日志到文件
pio device monitor > debug.log
```

## 🔄 CI/CD 自动构建和发布

本项目配置了完整的 GitHub Actions 工作流，支持自动构建和发布功能。

### 🚀 自动构建

#### 触发条件
- **代码推送**: 推送到 `main`、`develop` 分支时自动构建
- **拉取请求**: 创建或更新 PR 时进行代码检查和构建验证
- **标签发布**: 创建标签时自动构建并发布到 Release

#### 构建产物
- **固件文件**: `esp-sms-relay-{version}.bin`
- **构建信息**: `build-info.txt` (包含版本、时间、提交信息)
- **分区表**: `partitions.csv`
- **引导程序**: `bootloader.bin`

### 📦 自动发布

#### 版本发布流程
1. **开发完成**: 在 `develop` 分支完成功能开发
2. **合并主分支**: 将 `develop` 合并到 `main` 分支
3. **创建标签**: 使用语义化版本号创建标签
   ```bash
   git tag v1.2.0
   git push origin v1.2.0
   ```
4. **自动发布**: GitHub Actions 自动创建 Release 并上传固件

#### 版本命名规范
- **正式版本**: `v1.0.0`、`v1.1.0`、`v2.0.0`
- **预发布版本**: `v1.0.0-beta.1`、`v1.0.0-rc.1`
- **修复版本**: `v1.0.1`、`v1.0.2`

### 🔧 工作流配置

#### 持续集成 (CI)
- **文件**: `.github/workflows/ci.yml`
- **功能**: 代码检查、语法验证、快速构建
- **触发**: 推送到 `main`、`develop`、`feature/*` 分支或创建 PR

#### 构建和发布
- **文件**: `.github/workflows/build-and-release.yml`
- **功能**: 完整构建、固件生成、自动发布
- **触发**: 推送到 `main`、`develop` 分支或创建标签

### 📥 下载和使用

#### 获取最新固件
1. 访问项目的 [Releases 页面](../../releases)
2. 下载最新版本的 `esp-sms-relay-vX.X.X.bin` 文件
3. 使用 PlatformIO 或 esptool 烧录固件

#### 快速烧录
```bash
# 使用 PlatformIO
pio run --target upload --upload-port COM3

# 使用 esptool
esptool.py --chip esp32s3 --port COM3 --baud 921600 write_flash 0x10000 esp-sms-relay-v1.2.0.bin
```

### 🔍 构建状态监控

项目首页显示的徽章可以实时查看构建状态：
- **Build and Release**: 显示最新构建状态
- **CI Status**: 显示持续集成状态
- **Release**: 显示最新发布版本

### 📚 详细文档

更多 CI/CD 配置和使用说明，请参考 [CI/CD 指南](docs/CI_CD_GUIDE.md)。

## 部署和维护

### 1. 生产环境部署

#### 固件发布
```bash
# 编译发布版本
pio run --environment release

# 生成固件文件
cp .pio/build/esp32-s3-devkitm-1/firmware.bin releases/esp-sms-relay-v1.0.0.bin
```

#### 批量烧录
```bash
# 创建烧录脚本
echo "esptool.py --chip esp32s3 --port %1 --baud 921600 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin" > flash.bat
```

### 2. 太阳能供电系统部署

#### 硬件安装
1. **太阳能板选择**
   - 推荐功率：10W-20W
   - 输出电压：5-6V（默认配置）
   - 更高电压需要修改板载电阻配置

2. **电池配置**
   - 使用18650锂电池（3.7V，2000mAh以上推荐）
   - 确保电池正负极正确安装
   - 建议使用带保护板的电池

3. **安装位置选择**
   - 太阳能板朝南倾斜安装，角度约等于当地纬度
   - 避免遮挡，确保全天候光照
   - 设备防水防尘，IP65级别以上
   - 考虑散热和通风

#### 功耗优化配置
```cpp
// 在config.h中配置低功耗模式
#define ENABLE_DEEP_SLEEP 1           // 启用深度睡眠
#define SLEEP_DURATION_MS 300000      // 睡眠时长（5分钟）
#define BATTERY_LOW_THRESHOLD 3.2     // 低电量阈值（V）
#define SOLAR_CHARGING_THRESHOLD 4.0  // 太阳能充电阈值（V）
```

#### 环境适应性配置
```cpp
// 温度补偿和环境适应
#define TEMP_COMPENSATION_ENABLED 1   // 启用温度补偿
#define WORKING_TEMP_MIN -20          // 最低工作温度（°C）
#define WORKING_TEMP_MAX 60           // 最高工作温度（°C）
#define HUMIDITY_MAX 95               // 最大湿度（%）
```

### 3. 系统维护

#### 定期维护任务
- **每周检查**：
  - 太阳能板清洁度（清除灰尘、积雪）
  - 电池电压和充电状态
  - 设备外壳密封性
  - 天线连接状态

- **每月检查**：
  - 数据库大小和清理状态
  - 系统内存使用情况
  - 推送配置和规则有效性
  - 备份重要配置数据

- **季度检查**：
  - 电池容量测试和更换
  - 太阳能板支架稳固性
  - 防水密封胶更新
  - 系统固件更新

#### 远程监控接口
```cpp
// 扩展的健康检查接口
GET /api/health
{
  "status": "ok",
  "uptime": 86400,
  "free_heap": 234567,
  "sms_count": 1234,
  "last_sms": "2024-01-01 12:00:00",
  "power_status": {
    "battery_voltage": 3.8,
    "solar_voltage": 5.2,
    "charging": true,
    "battery_percentage": 75
  },
  "environmental": {
    "temperature": 25.5,
    "humidity": 60,
    "signal_strength": -65
  }
}
```

#### 故障诊断和处理
1. **电源相关故障**
   - 电池电压过低：检查太阳能板和充电电路
   - 充电异常：检查太阳能板连接和遮挡情况
   - 功耗异常：检查软件配置和硬件故障

2. **通信故障**
   - 4G信号弱：调整天线位置或更换高增益天线
   - SIM卡问题：检查SIM卡安装和运营商服务
   - 短信发送失败：检查运营商短信中心配置

3. **环境适应性问题**
   - 高温保护：增加散热措施或遮阳设施
   - 低温启动：使用低温电池或加热措施
   - 防水问题：检查密封胶和防水等级

#### 维护工具和备件
- **必备工具**：万用表、螺丝刀套装、密封胶、清洁布
- **备件清单**：18650电池、SIM卡、天线、密封圈
- **测试设备**：信号强度测试仪、电池容量测试仪

## 贡献指南

### 1. 开发流程

1. Fork项目仓库
2. 创建功能分支 (`git checkout -b feature/new-feature`)
3. 提交更改 (`git commit -am 'Add new feature'`)
4. 推送分支 (`git push origin feature/new-feature`)
5. 创建Pull Request

### 2. 代码提交规范

```bash
# 提交信息格式
[模块名] 简要描述

详细描述更改内容和原因

# 示例
[database] 添加自动清理功能

实现了数据库自动清理机制，当短信记录超过1万条时自动清理旧记录，
保持数据库大小在合理范围内。
```

### 3. 测试要求

- 新功能必须包含单元测试
- 确保所有现有测试通过
- 提供功能测试用例
- 更新相关文档

## 许可证

本项目采用 MIT 许可证，详见 [LICENSE](LICENSE) 文件。

## 联系方式

- 项目主页: [GitHub Repository]
- 问题反馈: [GitHub Issues]
- 技术讨论: [GitHub Discussions]

## 技术优势


### 📡 通信优势
- **网络独立**：4G网络覆盖广，不依赖WiFi和有线网络
- **稳定可靠**：多网络制式支持，确保通信稳定
- **低延迟**：短信推送实时性高，关键信息及时送达
- **兼容性好**：支持多种推送平台，适应不同需求

### 🛠️ 技术优势
- **模块化设计**：易于扩展和定制，满足不同应用需求
- **低功耗优化**：多级功耗管理，延长设备续航时间
- **数据安全**：本地数据库存储，支持数据加密和备份
- **远程管理**：Web界面和CLI管理，支持远程配置和监控

## 致谢

感谢以下开源项目和硬件厂商的支持：
- [微雪电子](https://www.waveshare.net/) - ESP32-S3-A7670E-4G开发板
- [PlatformIO](https://platformio.org/) - 嵌入式开发平台
- [ArduinoJson](https://arduinojson.org/) - JSON处理库
- [SQLite](https://www.sqlite.org/) - 嵌入式数据库
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) - ESP32 Arduino框架
- [SIMCom](https://www.simcom.com/) - A7670E 4G通信模组

---

**注意**: 本项目仅供学习和研究使用，请遵守当地法律法规，合理使用短信转发功能。在户外部署时请注意设备防护和环境适应性。