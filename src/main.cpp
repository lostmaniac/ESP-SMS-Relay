/**
 * @file main.cpp
 * @brief ESP32 SMS 中继系统主程序 - 集成终端管理器CLI
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该文件是系统的入口点，负责:
 * 1. 硬件初始化
 * 2. 启动终端管理器CLI
 * 3. 主循环管理和CLI交互
 */

#include <Arduino.h>
#include <sys/time.h>
#include "terminal_manager.h"
#include "database_manager.h"
#include "log_manager.h"
#include "filesystem_manager.h"
#include "gsm_service.h"
#include "carrier_config.h"
#include "phone_caller.h"
#include "uart_monitor.h"
#include "push_manager.h"
#include "task_scheduler.h"
#include "config.h"

// 全局管理器实例引用
TerminalManager& terminalManager = TerminalManager::getInstance();
DatabaseManager& databaseManager = DatabaseManager::getInstance();
LogManager& logManager = LogManager::getInstance();
FilesystemManager& filesystemManager = FilesystemManager::getInstance();
PushManager& pushManager = PushManager::getInstance();

// 定义硬件串口
HardwareSerial simSerial(SIM_SERIAL_NUM); // 使用配置的串口号

/**
 * @brief 系统初始化函数
 * 
 * 负责系统组件初始化和CLI启动
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool initializeSystem() {
    Serial.println("\n=== ESP-SMS-Relay System Starting ===");
    
    // 初始化日志管理器
    if (!logManager.initialize()) {
        Serial.println("Failed to initialize Log Manager");
        return false;
    }
    Serial.println("✓ Log Manager initialized");
    
    // 初始化文件系统管理器
    if (!filesystemManager.initialize()) {
        Serial.println("Failed to initialize Filesystem Manager: " + filesystemManager.getLastError());
        return false;
    }
    Serial.println("✓ Filesystem Manager initialized");
    
    // 初始化数据库管理器
    databaseManager.setDebugMode(true);  // 启用调试模式以获取详细错误信息
    if (!databaseManager.initialize()) {
        Serial.println("Failed to initialize Database Manager: " + databaseManager.getLastError());
        return false;
    }
    Serial.println("✓ Database Manager initialized");
    
    // 初始化终端管理器
    if (!terminalManager.initialize()) {
        Serial.println("Failed to initialize Terminal Manager: " + terminalManager.getLastError());
        return false;
    }
    Serial.println("✓ Terminal Manager initialized");
    
    // 初始化推送管理器
    if (!pushManager.initialize()) {
        Serial.println("Failed to initialize Push Manager: " + pushManager.getLastError());
        return false;
    }
    Serial.println("✓ Push Manager initialized");
    
    // 加载转发规则到缓存
    if (!pushManager.loadRulesToCache()) {
        Serial.println("⚠️  Failed to load rules to cache: " + pushManager.getLastError());
    } else {
        Serial.println("✓ Forward rules loaded to cache");
    }
    
    // 注意：UART监控任务将在GSM初始化完成后启动
    Serial.println("✓ UART Monitor Task will be started after GSM initialization");
    
    Serial.println("=== System Initialization Complete ===");
    return true;
}

/**
 * @brief 执行开机自动拨号功能
 * 
 * 检测运营商类型，如果是移动则自动拨打1008611并等待7秒后挂断
 */
void performStartupCall() {
    Serial.println("\n=== 开始执行开机自动拨号检测 ===");
    
    // 获取GSM服务实例
    GsmService& gsmService = GsmService::getInstance();
    
    // 初始化GSM服务
    if (!gsmService.initialize()) {
        Serial.println("⚠️  GSM服务初始化失败，跳过开机拨号: " + gsmService.getLastError());
        return;
    }
    
    // 检查GSM模块是否在线
    if (!gsmService.isModuleOnline()) {
        Serial.println("⚠️  GSM模块未在线，跳过开机拨号");
        return;
    }
    
    // 等待网络注册
    Serial.println("📡 等待网络注册...");
    if (!gsmService.waitForNetworkRegistration(15000)) {
        Serial.println("⚠️  网络注册超时，跳过开机拨号");
        return;
    }
    
    // GSM服务初始化成功后，尝试同步网络时间
    Serial.println("🕐 开始同步网络时间...");
    time_t networkTime = gsmService.getUnixTimestamp();
    if (networkTime > 0) {
        struct timeval tv;
        tv.tv_sec = networkTime;
        tv.tv_usec = 0;
        
        if (settimeofday(&tv, NULL) == 0) {
            Serial.println("✅ 网络时间同步成功");
            
            // 显示当前系统时间
            struct tm timeinfo;
            localtime_r(&networkTime, &timeinfo);
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
            Serial.println("📅 当前系统时间: " + String(timeStr));
        } else {
            Serial.println("❌ 设置系统时间失败");
        }
    } else {
        Serial.println("⚠️  获取网络时间失败: " + gsmService.getLastError());
    }
    
    // 获取IMSI号码
    String imsi = gsmService.getImsi();
    if (imsi.length() == 0) {
        Serial.println("⚠️  无法获取IMSI号码，跳过开机拨号");
        return;
    }
    
    Serial.println("📱 获取到IMSI: " + imsi);
    
    // 获取运营商配置实例并识别运营商
    CarrierConfig& carrierConfig = CarrierConfig::getInstance();
    CarrierType carrierType = carrierConfig.identifyCarrier(imsi);
    
    // 检查是否为中国移动
    if (carrierType == CARRIER_CHINA_MOBILE) {
        Serial.println("📞 检测到中国移动网络，开始自动拨号1008611...");
        
        // 获取电话拨打器实例
        PhoneCaller phoneCaller;
        
        // 拨打1008611并等待7秒后挂断
        PhoneCallResult result = phoneCaller.makeCallAndWait("1008611", 7);
        
        // 处理拨号结果
        switch (result) {
            case CALL_SUCCESS:
                Serial.println("✅ 开机自动拨号成功完成");
                break;
            case CALL_ERROR_NETWORK_NOT_READY:
                Serial.println("❌ 开机自动拨号失败: 网络未就绪");
                break;
            case CALL_ERROR_INVALID_NUMBER:
                Serial.println("❌ 开机自动拨号失败: 号码格式无效");
                break;
            case CALL_ERROR_AT_COMMAND_FAILED:
                Serial.println("❌ 开机自动拨号失败: AT命令执行失败");
                break;
            case CALL_ERROR_CALL_TIMEOUT:
                Serial.println("❌ 开机自动拨号失败: 拨打超时");
                break;
            case CALL_ERROR_HANGUP_FAILED:
                Serial.println("❌ 开机自动拨号失败: 挂断失败");
                break;
            default:
                Serial.println("❌ 开机自动拨号失败: 未知错误");
                break;
        }
        
        if (result != CALL_SUCCESS) {
            Serial.println("🔍 拨号错误详情: " + phoneCaller.getLastError());
        }
    } else {
        String carrierName = carrierConfig.getCarrierName(carrierType);
        Serial.println("📋 检测到运营商: " + carrierName + "，非移动网络，跳过开机拨号");
    }
    
    Serial.println("=== 开机自动拨号检测完成 ===");
    
    // GSM初始化完成后，启动UART监控任务
    Serial.println("\n=== 启动UART监控任务 ===");
    xTaskCreate(
        uart_monitor_task,   // Task function
        "UartMonitorTask",   // Task name
        10000,               // Stack size (bytes)
        NULL,                // Parameter
        1,                   // Priority
        NULL                 // Task handle
    );
    Serial.println("✓ UART Monitor Task started");
}

/**
 * @brief 创建示例转发规则
 */
void createExampleRules() {
    Serial.println("\n=== Creating Example Forward Rules ===");
    
    // 示例规则1：银行短信转发到企业微信
    ForwardRule bankRule;
    bankRule.ruleName = "Bank Notifications";  // 使用 ruleName 而不是 name
    bankRule.sourceNumber = "95588";  // 使用 sourceNumber 而不是 senderPattern
    bankRule.keywords = "*余额*";  // 使用 keywords 而不是 contentPattern
    bankRule.pushType = "wechat";
    bankRule.pushConfig = "{\"webhook\":\"https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=xxx\"}";
    bankRule.enabled = true;
    bankRule.isDefaultForward = false;
    
    int bankRuleId = terminalManager.addForwardRule(bankRule);
    if (bankRuleId > 0) {
        Serial.println("✓ Created bank rule with ID: " + String(bankRuleId));
    } else {
        Serial.println("✗ Failed to create bank rule: " + terminalManager.getLastError());
    }
    
    // 示例规则2：验证码转发到钉钉
    ForwardRule codeRule;
    codeRule.ruleName = "Verification Codes";  // 使用 ruleName 而不是 name
    codeRule.sourceNumber = "*";  // 使用 sourceNumber 而不是 senderPattern
    codeRule.keywords = "*验证码*";  // 使用 keywords 而不是 contentPattern
    codeRule.pushType = "dingtalk";
    codeRule.pushConfig = "{\"webhook\":\"https://oapi.dingtalk.com/robot/send?access_token=xxx\"}";
    codeRule.enabled = true;
    codeRule.isDefaultForward = false;
    
    int codeRuleId = terminalManager.addForwardRule(codeRule);
    if (codeRuleId > 0) {
        Serial.println("✓ Created verification code rule with ID: " + String(codeRuleId));
    } else {
        Serial.println("✗ Failed to create verification code rule: " + terminalManager.getLastError());
    }
    
    Serial.println("=== Example Rules Creation Complete ===");
}

/**
 * @brief 系统设置函数
 * 
 * 负责基础硬件初始化和系统组件启动
 */
void setup() {
    // 初始化串口
    Serial.begin(115200);
    simSerial.begin(SIM_BAUD_RATE, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
    
    // 等待串口稳定
    delay(1000);
    
    Serial.println("\n" + String('=', 50));
    Serial.println("    ESP32 SMS Relay System with CLI");
    Serial.println("    Version: 1.0.0");
    Serial.println("    Build: " + String(__DATE__) + " " + String(__TIME__));
    Serial.println(String('=', 50));
    
    // 初始化系统
    if (!initializeSystem()) {
        Serial.println("\n❌ System initialization failed! Halting.");
        while (true) {
            delay(1000);
        }
    }
    
    // 不再自动创建示例规则
    // 用户可以通过CLI手动添加规则
    
    // 显示当前状态
    Serial.println("\n=== Current System Status ===");
    Serial.println("Total rules: " + String(terminalManager.getRuleCount()));
    Serial.println("Enabled rules: " + String(terminalManager.getEnabledRuleCount()));
    Serial.println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
    
    // 执行开机自动拨号功能
    performStartupCall();
    
    // 启动CLI
    terminalManager.startCLI();
    
    Serial.println("\n🚀 System Ready! Type 'help' for available commands.");
    Serial.println("📝 CLI is now active and waiting for input...");
}

/**
 * @brief 主循环函数
 * 
 * 系统运行后的主循环，负责CLI交互和系统维护
 */
void loop() {
    // 处理CLI输入
    if (terminalManager.isCLIRunning()) {
        terminalManager.handleSerialInput();
    }
    
    // 处理定时任务调度
    TaskScheduler& taskScheduler = TaskScheduler::getInstance();
    taskScheduler.handleTasks();
    
    // 这里可以添加其他系统任务
    // 例如：处理SMS、网络通信、状态监控等
    
    // 系统心跳日志（每30秒）
    static unsigned long lastHeartbeat = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastHeartbeat > 30000) {
        logManager.logInfo(LOG_MODULE_SYSTEM, "System heartbeat - Rules: " + String(terminalManager.getRuleCount()) + 
                      ", Enabled: " + String(terminalManager.getEnabledRuleCount()) +
                      ", Free heap: " + String(ESP.getFreeHeap()) + " bytes");
        lastHeartbeat = currentTime;
    }
    
    // 内存监控和警告
    static unsigned long lastMemoryCheck = 0;
    if (currentTime - lastMemoryCheck > 60000) { // 每分钟检查一次
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 10000) { // 小于10KB时发出警告
            Serial.println("⚠️  Low memory warning: " + String(freeHeap) + " bytes free");
            logManager.logWarn(LOG_MODULE_SYSTEM, "WARNING: Low memory - " + String(freeHeap) + " bytes free");
        }
        lastMemoryCheck = currentTime;
    }
    
    // 短暂延迟，避免过度占用CPU
    delay(10);
}