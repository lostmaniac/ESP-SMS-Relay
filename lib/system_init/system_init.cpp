/**
 * @file system_init.cpp
 * @brief 系统初始化模块实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "system_init.h"
#include "module_manager.h"
#include "../uart_monitor/uart_monitor.h"
#include "config_manager.h"
#include "log_manager.h"
#include "../network_config/network_config.h"
#include "../filesystem_manager/filesystem_manager.h"
#include "../database_manager/database_manager.h"
#include "../wifi_manager/wifi_manager.h"
#include "../web_server/web_server.h"
#include "../gsm_service/gsm_service.h"
#include "../carrier_config/carrier_config.h"
#include "../phone_caller/phone_caller.h"
#include <Arduino.h>

/**
 * @brief 构造函数
 */
SystemInit::SystemInit() : 
    systemStatus(SYSTEM_NOT_INITIALIZED),
    lastError(""),
    initialized(false) {
}

/**
 * @brief 析构函数
 */
SystemInit::~SystemInit() {
    // 清理资源
}

/**
 * @brief 获取单例实例
 * @return SystemInit& 单例引用
 */
SystemInit& SystemInit::getInstance() {
    static SystemInit instance;
    return instance;
}

/**
 * @brief 初始化系统
 * @param runTests 是否运行测试
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool SystemInit::initialize(bool runTests) {
    if (initialized) {
        return true;
    }
    
    // 首先初始化配置管理器
    ConfigManager& configManager = ConfigManager::getInstance();
    if (!configManager.initialize()) {
        return false;
    }
    
    // 然后初始化日志管理器
    LogManager& logManager = LogManager::getInstance();
    if (!logManager.initialize()) {
        return false;
    }
    
    // 打印系统启动信息
    logManager.printStartupInfo();
    
    setSystemStatus(SYSTEM_INITIALIZING);
    LOG_INFO(LOG_MODULE_SYSTEM, "开始系统初始化");
    
    // 初始化文件系统
    LOG_INFO(LOG_MODULE_SYSTEM, "正在初始化文件系统...");
    FilesystemManager& filesystemManager = FilesystemManager::getInstance();
    filesystemManager.setDebugMode(true); // 启用调试模式
    
    if (!filesystemManager.initialize(true)) {
        setError("文件系统初始化失败: " + filesystemManager.getLastError());
        setSystemStatus(SYSTEM_ERROR);
        return false;
    }
    
    // 打印文件系统信息
    FilesystemInfo fsInfo = filesystemManager.getFilesystemInfo();
    LOG_INFO(LOG_MODULE_SYSTEM, "文件系统初始化成功");
    LOG_INFO(LOG_MODULE_SYSTEM, "文件系统总空间: " + String(fsInfo.totalBytes) + " 字节");
    LOG_INFO(LOG_MODULE_SYSTEM, "文件系统已使用: " + String(fsInfo.usedBytes) + " 字节 (" + String(fsInfo.usagePercent, 1) + "%)");
    LOG_INFO(LOG_MODULE_SYSTEM, "文件系统可用空间: " + String(fsInfo.freeBytes) + " 字节");
    
    // 打印/littlefs目录下的所有文件
    LOG_INFO(LOG_MODULE_SYSTEM, "正在扫描/littlefs目录下的文件...");
    fs::FS& fs = filesystemManager.getFS();
    File root = fs.open("/");
    if (!root) {
        LOG_INFO(LOG_MODULE_SYSTEM, "无法打开/littlefs根目录");
    } else if (!root.isDirectory()) {
        LOG_INFO(LOG_MODULE_SYSTEM, "/littlefs根路径不是目录");
        root.close();
    } else {
        int fileCount = 0;
        File file = root.openNextFile();
        while (file) {
            if (file.isDirectory()) {
                LOG_INFO(LOG_MODULE_SYSTEM, "  [目录] " + String(file.name()));
            } else {
                LOG_INFO(LOG_MODULE_SYSTEM, "  [文件] " + String(file.name()) + " (" + String(file.size()) + " 字节)");
            }
            fileCount++;
            file.close();
            file = root.openNextFile();
        }
        root.close();
        if (fileCount == 0) {
            LOG_INFO(LOG_MODULE_SYSTEM, "/littlefs目录为空");
        } else {
            LOG_INFO(LOG_MODULE_SYSTEM, "/littlefs目录下共有 " + String(fileCount) + " 个项目");
        }
    }
    
    // 测试data目录文件访问
    LOG_INFO(LOG_MODULE_SYSTEM, "正在测试文件系统访问...");
    if (filesystemManager.fileExists("/test.txt")) {
        LOG_INFO(LOG_MODULE_SYSTEM, "test.txt文件存在于根目录");
    } else {
        LOG_INFO(LOG_MODULE_SYSTEM, "test.txt文件不存在于根目录");
    }
    
    // 初始化数据库
    LOG_INFO(LOG_MODULE_SYSTEM, "正在初始化数据库...");
    DatabaseManager& databaseManager = DatabaseManager::getInstance();
    databaseManager.setDebugMode(true); // 启用调试模式
    
    if (!databaseManager.initialize("sms_relay.db")) {
        setError("数据库初始化失败: " + databaseManager.getLastError());
        setSystemStatus(SYSTEM_ERROR);
        return false;
    }
    
    // 打印数据库信息
    DatabaseInfo dbInfo = databaseManager.getDatabaseInfo();
    LOG_INFO(LOG_MODULE_SYSTEM, "数据库初始化成功");
    LOG_INFO(LOG_MODULE_SYSTEM, "数据库路径: " + dbInfo.dbPath);
    LOG_INFO(LOG_MODULE_SYSTEM, "数据库大小: " + String(dbInfo.dbSize) + " 字节");
    LOG_INFO(LOG_MODULE_SYSTEM, "数据库表数量: " + String(dbInfo.tableCount));
    LOG_INFO(LOG_MODULE_SYSTEM, "数据库记录总数: " + String(dbInfo.recordCount));
    
    // 获取并显示AP配置信息
    APConfig apConfig = databaseManager.getAPConfig();
    LOG_INFO(LOG_MODULE_SYSTEM, "AP配置 - SSID: " + apConfig.ssid + ", 密码: " + apConfig.password + ", 启用: " + String(apConfig.enabled ? "是" : "否"));
    LOG_INFO(LOG_MODULE_SYSTEM, "AP配置 - 信道: " + String(apConfig.channel) + ", 最大连接数: " + String(apConfig.maxConnections));
    
    // 初始化并启动WiFi管理器
    LOG_INFO(LOG_MODULE_SYSTEM, "正在初始化WiFi管理器...");
    WiFiManager& wifiManager = WiFiManager::getInstance();
    wifiManager.setDebugMode(true); // 启用调试模式
    
    if (!wifiManager.initialize()) {
        setError("WiFi管理器初始化失败: " + wifiManager.getLastError());
        setSystemStatus(SYSTEM_ERROR);
        return false;
    }
    
    // 如果AP配置启用，则启动WiFi热点
    if (apConfig.enabled) {
        LOG_INFO(LOG_MODULE_SYSTEM, "正在启动WiFi热点...");
        if (!wifiManager.startAP()) {
            LOG_WARN(LOG_MODULE_SYSTEM, "WiFi热点启动失败: " + wifiManager.getLastError());
        } else {
            WiFiConnectionInfo connInfo = wifiManager.getConnectionInfo();
            LOG_INFO(LOG_MODULE_SYSTEM, "WiFi热点启动成功 - IP: " + connInfo.apIP + ", MAC: " + connInfo.apMAC);
        }
    } else {
        LOG_INFO(LOG_MODULE_SYSTEM, "AP配置未启用，跳过WiFi热点启动");
    }
    
    // 初始化并启动Web服务器
    LOG_INFO(LOG_MODULE_SYSTEM, "正在初始化Web服务器...");
    WebServerManager& webServer = WebServerManager::getInstance();
    webServer.setDebugMode(true); // 启用调试模式
    
    if (!webServer.initialize()) {
        LOG_WARN(LOG_MODULE_SYSTEM, "Web服务器初始化失败: " + webServer.getLastError());
    } else {
        // 如果WiFi热点已激活，则启动Web服务器
        if (wifiManager.isAPActive()) {
            LOG_INFO(LOG_MODULE_SYSTEM, "正在启动Web服务器...");
            if (!webServer.start()) {
                LOG_WARN(LOG_MODULE_SYSTEM, "Web服务器启动失败: " + webServer.getLastError());
            } else {
                LOG_INFO(LOG_MODULE_SYSTEM, "Web服务器启动成功 - URL: " + webServer.getServerURL());
            }
        } else {
            LOG_INFO(LOG_MODULE_SYSTEM, "WiFi热点未激活，Web服务器将在热点启动后自动启动");
        }
    }
    
    // 获取模块管理器实例
    ModuleManager& moduleManager = ModuleManager::getInstance();
    
    // 初始化所有模块
    LOG_INFO(LOG_MODULE_SYSTEM, "正在初始化系统模块...");
    if (!moduleManager.initializeAllModules()) {
        setError("模块初始化失败: " + moduleManager.getLastError());
        setSystemStatus(SYSTEM_ERROR);
        return false;
    }
    
    // 检查所有模块是否就绪
    if (!moduleManager.areAllModulesReady()) {
        setError("部分模块未就绪");
        setSystemStatus(SYSTEM_ERROR);
        return false;
    }
    
    LOG_INFO(LOG_MODULE_SYSTEM, "所有模块初始化完成");
    
    // 配置网络连接
    LOG_INFO(LOG_MODULE_SYSTEM, "正在配置网络连接...");
    NetworkConfig& networkConfig = NetworkConfig::getInstance();
    
    // 初始化网络配置模块
    if (!networkConfig.initialize()) {
        setError("网络配置模块初始化失败: " + networkConfig.getLastError());
        setSystemStatus(SYSTEM_ERROR);
        return false;
    }
    
    // 自动配置网络（识别运营商并配置APN）
    NetworkConfigResult configResult = networkConfig.autoConfigureNetwork();
    if (configResult.status != NETWORK_CONFIG_SUCCESS) {
        LOG_WARN(LOG_MODULE_SYSTEM, "网络自动配置失败: " + configResult.errorMessage);
        LOG_INFO(LOG_MODULE_SYSTEM, "尝试使用默认配置...");
        
        // 尝试使用默认配置
        NetworkConfigResult defaultResult = networkConfig.configureNetwork(CARRIER_UNKNOWN);
        if (defaultResult.status != NETWORK_CONFIG_SUCCESS) {
            setError("网络配置失败，无法建立网络连接: " + defaultResult.errorMessage);
            setSystemStatus(SYSTEM_ERROR);
            return false;
        }
        configResult = defaultResult; // 使用默认配置结果
    }
    
    LOG_INFO(LOG_MODULE_SYSTEM, "网络配置完成 - 运营商: " + configResult.carrierName + ", APN: " + configResult.apnConfig.apn);
    
    // 统一检查网络连接状态（避免重复检查）
    if (networkConfig.isNetworkReady()) {
        LOG_INFO(LOG_MODULE_SYSTEM, "网络连接已建立，系统就绪");
    } else {
        LOG_WARN(LOG_MODULE_SYSTEM, "网络连接未完全建立，但系统将继续运行");
    }
    
    // 运行测试（可选）
    if (runTests) {
        LOG_INFO(LOG_MODULE_SYSTEM, "正在运行系统测试...");
        if (!moduleManager.runModuleTests()) {
            LOG_WARN(LOG_MODULE_SYSTEM, "部分测试未通过，但系统将继续运行");
        } else {
            LOG_INFO(LOG_MODULE_SYSTEM, "所有测试通过");
        }
    }
    
    setSystemStatus(SYSTEM_READY);
    initialized = true;
    
    logManager.printSeparator("系统初始化完成");
    return true;
}

/**
 * @brief 启动系统
 * @return true 启动成功
 * @return false 启动失败
 */
bool SystemInit::start() {
    if (!initialized) {
        setError("系统未初始化");
        return false;
    }
    
    if (systemStatus != SYSTEM_READY) {
        setError("系统状态不正确，无法启动");
        return false;
    }
    
    LOG_INFO(LOG_MODULE_SYSTEM, "正在启动系统服务...");
    
    // 获取模块管理器实例
    ModuleManager& moduleManager = ModuleManager::getInstance();
    
    // 启动后台任务
    if (!moduleManager.startBackgroundTasks()) {
        setError("启动后台任务失败: " + moduleManager.getLastError());
        setSystemStatus(SYSTEM_ERROR);
        return false;
    }
    
    setSystemStatus(SYSTEM_RUNNING);
    LogManager::getInstance().printSeparator("系统启动完成，开始运行");
    
    // 执行开机自动拨号功能
    performStartupCall();
    
    return true;
}

/**
 * @brief 获取系统状态
 * @return SystemStatus 系统状态
 */
SystemStatus SystemInit::getSystemStatus() {
    return systemStatus;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String SystemInit::getLastError() {
    return lastError;
}

/**
 * @brief 重启系统
 * @return true 重启成功
 * @return false 重启失败
 */
bool SystemInit::restart() {
    // 重置状态
    initialized = false;
    setSystemStatus(SYSTEM_NOT_INITIALIZED);
    
    // 等待一段时间
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // 重新初始化
    if (initialize()) {
        return start();
    }
    
    return false;
}

/**
 * @brief 设置系统状态
 * @param status 系统状态
 */
void SystemInit::setSystemStatus(SystemStatus status) {
    systemStatus = status;
    
    // 打印状态变化
    String statusName;
    switch (status) {
        case SYSTEM_NOT_INITIALIZED: statusName = "未初始化"; break;
        case SYSTEM_INITIALIZING: statusName = "初始化中"; break;
        case SYSTEM_READY: statusName = "就绪"; break;
        case SYSTEM_ERROR: statusName = "错误"; break;
        case SYSTEM_RUNNING: statusName = "运行中"; break;
        default: statusName = "未知"; break;
    }
    
    // 系统状态检查
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void SystemInit::setError(const String& error) {
    lastError = error;
    LOG_ERROR(LOG_MODULE_SYSTEM, "系统错误: " + error);
}

/**
 * @brief 系统初始化任务函数
 * @param pvParameters 任务参数
 */
void system_init_task(void *pvParameters) {
    // 等待系统稳定
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    SystemInit& systemInit = SystemInit::getInstance();
    
    // 获取配置管理器以确定是否运行测试
    ConfigManager& configManager = ConfigManager::getInstance();
    SystemConfig sysConfig = configManager.getSystemConfig();
    
    // 初始化系统（根据配置决定是否运行测试）
    if (systemInit.initialize(sysConfig.runTestsOnStartup)) {
        // 启动系统
        if (systemInit.start()) {
            LOG_INFO(LOG_MODULE_SYSTEM, "系统启动成功，初始化任务完成");
        } else {
            LOG_ERROR(LOG_MODULE_SYSTEM, "系统启动失败: " + systemInit.getLastError());
        }
    } else {
        LOG_ERROR(LOG_MODULE_SYSTEM, "系统初始化失败: " + systemInit.getLastError());
    }
    
    // 删除此任务
    vTaskDelete(NULL);
}

/**
 * @brief 执行开机自动拨号功能
 * 
 * 检测运营商类型，如果是移动则自动拨打1008611并等待7秒后挂断
 */
void SystemInit::performStartupCall() {
    LOG_INFO(LOG_MODULE_SYSTEM, "开始执行开机自动拨号检测...");
    
    // 获取GSM服务实例
    GsmService& gsmService = GsmService::getInstance();
    
    // 检查GSM模块是否在线
    if (!gsmService.isModuleOnline()) {
        LOG_WARN(LOG_MODULE_SYSTEM, "GSM模块未在线，跳过开机拨号");
        return;
    }
    
    // 检查网络是否就绪
    if (gsmService.getNetworkStatus() != GSM_NETWORK_REGISTERED_HOME && 
        gsmService.getNetworkStatus() != GSM_NETWORK_REGISTERED_ROAMING) {
        LOG_WARN(LOG_MODULE_SYSTEM, "网络未注册，跳过开机拨号");
        return;
    }
    
    // 获取IMSI号码
    String imsi = gsmService.getImsi();
    if (imsi.length() == 0) {
        LOG_WARN(LOG_MODULE_SYSTEM, "无法获取IMSI号码，跳过开机拨号");
        return;
    }
    
    LOG_INFO(LOG_MODULE_SYSTEM, "获取到IMSI: " + imsi);
    
    // 获取运营商配置实例并识别运营商
    CarrierConfig& carrierConfig = CarrierConfig::getInstance();
    CarrierType carrierType = carrierConfig.identifyCarrier(imsi);
    
    // 检查是否为中国移动
    if (carrierType == CARRIER_CHINA_MOBILE) {
        LOG_INFO(LOG_MODULE_SYSTEM, "检测到中国移动网络，开始自动拨号1008611...");
        
        // 获取电话拨打器实例
        PhoneCaller phoneCaller;
        
        // 拨打1008611并等待7秒后挂断
        PhoneCallResult result = phoneCaller.makeCallAndWait("1008611", 7);
        
        // 处理拨号结果
        switch (result) {
            case CALL_SUCCESS:
                LOG_INFO(LOG_MODULE_SYSTEM, "开机自动拨号成功完成");
                break;
            case CALL_ERROR_NETWORK_NOT_READY:
                LOG_WARN(LOG_MODULE_SYSTEM, "开机自动拨号失败: 网络未就绪");
                break;
            case CALL_ERROR_INVALID_NUMBER:
                LOG_WARN(LOG_MODULE_SYSTEM, "开机自动拨号失败: 号码格式无效");
                break;
            case CALL_ERROR_AT_COMMAND_FAILED:
                LOG_WARN(LOG_MODULE_SYSTEM, "开机自动拨号失败: AT命令执行失败");
                break;
            case CALL_ERROR_CALL_TIMEOUT:
                LOG_WARN(LOG_MODULE_SYSTEM, "开机自动拨号失败: 拨打超时");
                break;
            case CALL_ERROR_HANGUP_FAILED:
                LOG_WARN(LOG_MODULE_SYSTEM, "开机自动拨号失败: 挂断失败");
                break;
            default:
                LOG_WARN(LOG_MODULE_SYSTEM, "开机自动拨号失败: 未知错误");
                break;
        }
        
        if (result != CALL_SUCCESS) {
            LOG_WARN(LOG_MODULE_SYSTEM, "拨号错误详情: " + phoneCaller.getLastError());
        }
    } else {
        String carrierName = carrierConfig.getCarrierName(carrierType);
        LOG_INFO(LOG_MODULE_SYSTEM, "检测到运营商: " + carrierName + "，非移动网络，跳过开机拨号");
    }
}