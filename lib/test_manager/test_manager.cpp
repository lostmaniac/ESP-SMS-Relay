/**
 * @file test_manager.cpp
 * @brief 测试管理器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "test_manager.h"
#include "gsm_service.h"
#include "phone_caller.h"
#include "module_manager.h"
#include <Arduino.h>

/**
 * @brief 构造函数
 */
TestManager::TestManager() : initialized(false), completedTests(0) {
    // 初始化默认测试配置
    config.testPhoneNumber = "1008611";
    config.testSmsNumber = "+8610086";
    config.testSmsContent = "TEST";
    config.callDuration = 5;  // 修改为5秒
    config.enableDetailedLog = true;
    config.testTimeout = 30000;
    
    // 清空测试报告
    clearTestReports();
    lastError = "";
}

/**
 * @brief 析构函数
 */
TestManager::~TestManager() {
    // 清理资源
}

/**
 * @brief 获取单例实例
 * @return TestManager& 单例引用
 */
TestManager& TestManager::getInstance() {
    static TestManager instance;
    return instance;
}

/**
 * @brief 初始化测试管理器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool TestManager::initialize() {
    if (initialized) {
        return true;
    }
    
    Serial.println("正在初始化测试管理器...");
    
    // 清空测试报告
    clearTestReports();
    
    initialized = true;
    Serial.println("测试管理器初始化完成。");
    return true;
}

/**
 * @brief 设置测试配置
 * @param config 测试配置
 */
void TestManager::setTestConfig(const TestConfig& config) {
    this->config = config;
}

/**
 * @brief 获取测试配置
 * @return TestConfig 测试配置
 */
TestConfig TestManager::getTestConfig() {
    return config;
}

/**
 * @brief 运行指定类型的测试
 * @param testType 测试类型
 * @return TestResult 测试结果
 */
TestResult TestManager::runTest(TestType testType) {
    if (!initialized) {
        setError("测试管理器未初始化");
        return TEST_ERROR;
    }
    
    if (testType == TEST_ALL) {
        return runAllTests() ? TEST_SUCCESS : TEST_FAILED;
    }
    
    Serial.printf("\n=== 开始 %s ===\n", getTestTypeName(testType).c_str());
    
    unsigned long startTime = millis();
    TestResult result = TEST_ERROR;
    String description = "";
    String errorMessage = "";
    
    switch (testType) {
        case TEST_GSM_BASIC:
            result = testGsmBasic();
            description = "GSM基础功能测试";
            break;
        case TEST_SMS_SEND:
            result = testSmsSend();
            description = "短信发送测试";
            break;
        case TEST_SMS_RECEIVE:
            result = testSmsReceive();
            description = "短信接收测试";
            break;
        case TEST_PHONE_CALL:
            result = testPhoneCall();
            description = "电话拨打测试";
            break;
        case TEST_NETWORK_STATUS:
            result = testNetworkStatus();
            description = "网络状态测试";
            break;
        case TEST_SIGNAL_STRENGTH:
            result = testSignalStrength();
            description = "信号强度测试";
            break;
        case TEST_SIM_CARD:
            result = testSimCard();
            description = "SIM卡测试";
            break;
        default:
            result = TEST_NOT_IMPLEMENTED;
            description = "未实现的测试";
            errorMessage = "测试类型未实现";
            break;
    }
    
    unsigned long duration = millis() - startTime;
    
    if (result != TEST_SUCCESS && errorMessage.length() == 0) {
        errorMessage = lastError;
    }
    
    createTestReport(testType, result, description, errorMessage, duration);
    
    Serial.printf("=== %s 完成: %s ===\n\n", 
                  getTestTypeName(testType).c_str(), 
                  getTestResultName(result).c_str());
    
    return result;
}

/**
 * @brief 运行所有测试
 * @return bool 所有测试是否通过
 */
bool TestManager::runAllTests() {
    Serial.println("\n=== 开始运行所有测试 ===");
    
    bool allPassed = true;
    
    // 按顺序运行所有测试
    TestType tests[] = {
        TEST_GSM_BASIC,
        TEST_SIM_CARD,
        TEST_NETWORK_STATUS,
        TEST_SIGNAL_STRENGTH,
        TEST_SMS_SEND,
        TEST_PHONE_CALL
        // TEST_SMS_RECEIVE 需要外部触发，暂时跳过
    };
    
    int testCount = sizeof(tests) / sizeof(tests[0]);
    
    for (int i = 0; i < testCount; i++) {
        TestResult result = runTest(tests[i]);
        if (result != TEST_SUCCESS) {
            allPassed = false;
        }
        
        // 测试间隔
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    Serial.println("\n=== 所有测试完成 ===");
    printTestReport(TEST_ALL);
    
    return allPassed;
}

/**
 * @brief GSM基础功能测试
 * @return TestResult 测试结果
 */
TestResult TestManager::testGsmBasic() {
    GsmService& gsmService = GsmService::getInstance();
    
    if (!gsmService.isModuleOnline()) {
        setError("GSM模块离线");
        return TEST_FAILED;
    }
    
    if (config.enableDetailedLog) {
        Serial.println("GSM模块在线，基础功能正常。");
    }
    
    return TEST_SUCCESS;
}

/**
 * @brief 短信发送测试
 * @return TestResult 测试结果
 */
TestResult TestManager::testSmsSend() {
    // 这里需要获取全局的短信发送器实例
    // 由于模块管理器还未完全集成，暂时返回跳过
    if (config.enableDetailedLog) {
        Serial.println("短信发送测试暂时跳过，等待模块管理器集成。");
    }
    return TEST_SKIPPED;
}

/**
 * @brief 短信接收测试
 * @return TestResult 测试结果
 */
TestResult TestManager::testSmsReceive() {
    if (config.enableDetailedLog) {
        Serial.println("短信接收测试需要外部触发，暂时跳过。");
    }
    return TEST_SKIPPED;
}

/**
 * @brief 电话拨打测试
 * @return TestResult 测试结果
 */
TestResult TestManager::testPhoneCall() {
    if (config.enableDetailedLog) {
        Serial.println("开始电话拨打测试...");
    }
    
    // 获取电话拨打器实例
    PhoneCaller* phoneCaller = getPhoneCaller();
    if (!phoneCaller) {
        setError("电话拨打器未初始化");
        return TEST_FAILED;
    }
    
    // 检查网络状态
    if (!phoneCaller->isNetworkReady()) {
        setError("网络未就绪，无法拨打电话");
        return TEST_FAILED;
    }
    
    // 检查测试电话号码是否配置
    if (config.testPhoneNumber.length() == 0) {
        if (config.enableDetailedLog) {
            Serial.println("未配置测试电话号码，跳过电话拨打测试");
        }
        return TEST_SKIPPED;
    }
    
    if (config.enableDetailedLog) {
        Serial.printf("拨打测试电话: %s\n", config.testPhoneNumber.c_str());
    }
    
    // 执行电话拨打测试
    PhoneCallResult result = phoneCaller->makeCallAndWait(config.testPhoneNumber, config.callDuration);
    
    switch (result) {
        case CALL_SUCCESS:
            if (config.enableDetailedLog) {
                Serial.println("电话拨打测试成功");
            }
            return TEST_SUCCESS;
            
        case CALL_ERROR_NETWORK_NOT_READY:
            setError("网络未就绪");
            return TEST_FAILED;
            
        case CALL_ERROR_INVALID_NUMBER:
            setError("电话号码格式无效");
            return TEST_FAILED;
            
        case CALL_ERROR_AT_COMMAND_FAILED:
            setError("AT命令执行失败: " + phoneCaller->getLastError());
            return TEST_FAILED;
            
        case CALL_ERROR_CALL_TIMEOUT:
            setError("拨打超时");
            return TEST_FAILED;
            
        case CALL_ERROR_HANGUP_FAILED:
            setError("挂断失败");
            return TEST_FAILED;
            
        default:
            setError("未知错误");
            return TEST_FAILED;
    }
}

/**
 * @brief 网络状态测试
 * @return TestResult 测试结果
 */
TestResult TestManager::testNetworkStatus() {
    GsmService& gsmService = GsmService::getInstance();
    
    GsmNetworkStatus status = gsmService.getNetworkStatus();
    
    if (status == GSM_NETWORK_REGISTERED_HOME || status == GSM_NETWORK_REGISTERED_ROAMING) {
        if (config.enableDetailedLog) {
            Serial.printf("网络状态正常: %s\n", 
                         status == GSM_NETWORK_REGISTERED_HOME ? "本地网络" : "漫游网络");
        }
        return TEST_SUCCESS;
    } else {
        setError("网络未注册或状态异常");
        return TEST_FAILED;
    }
}

/**
 * @brief 信号强度测试
 * @return TestResult 测试结果
 */
TestResult TestManager::testSignalStrength() {
    GsmService& gsmService = GsmService::getInstance();
    
    int signalStrength = gsmService.getSignalStrength();
    
    if (signalStrength >= 0 && signalStrength <= 31) {
        if (config.enableDetailedLog) {
            Serial.printf("信号强度: %d/31\n", signalStrength);
        }
        
        if (signalStrength < 10) {
            if (config.enableDetailedLog) {
                Serial.println("警告: 信号强度较弱");
            }
        }
        
        return TEST_SUCCESS;
    } else {
        setError("无法获取信号强度");
        return TEST_FAILED;
    }
}

/**
 * @brief SIM卡测试
 * @return TestResult 测试结果
 */
TestResult TestManager::testSimCard() {
    GsmService& gsmService = GsmService::getInstance();
    
    if (gsmService.isSimCardReady()) {
        if (config.enableDetailedLog) {
            Serial.println("SIM卡状态正常。");
        }
        return TEST_SUCCESS;
    } else {
        setError("SIM卡未就绪");
        return TEST_FAILED;
    }
}

/**
 * @brief 获取测试报告
 * @param testType 测试类型
 * @return TestReport 测试报告
 */
TestReport TestManager::getTestReport(TestType testType) {
    if (testType < TEST_TYPE_COUNT) {
        return reports[testType];
    }
    
    TestReport emptyReport = {};
    return emptyReport;
}

/**
 * @brief 获取所有测试报告
 * @param reports 测试报告数组
 * @param maxReports 最大报告数量
 * @return int 实际报告数量
 */
int TestManager::getAllTestReports(TestReport* reports, int maxReports) {
    int count = 0;
    for (int i = 0; i < TEST_TYPE_COUNT && count < maxReports; i++) {
        if (this->reports[i].testType != TEST_ALL) { // 有效的测试报告
            reports[count] = this->reports[i];
            count++;
        }
    }
    return count;
}

/**
 * @brief 打印测试报告
 * @param testType 测试类型（TEST_ALL表示打印所有）
 */
void TestManager::printTestReport(TestType testType) {
    Serial.println("\n=== 测试报告 ===");
    
    if (testType == TEST_ALL) {
        int passedTests = 0;
        int totalTests = 0;
        
        for (int i = 0; i < TEST_TYPE_COUNT; i++) {
            if (reports[i].testType != TEST_ALL) {
                totalTests++;
                if (reports[i].result == TEST_SUCCESS) {
                    passedTests++;
                }
                
                Serial.printf("%s: %s", 
                             getTestTypeName(reports[i].testType).c_str(),
                             getTestResultName(reports[i].result).c_str());
                
                if (reports[i].duration > 0) {
                    Serial.printf(" (%lums)", reports[i].duration);
                }
                
                if (reports[i].errorMessage.length() > 0) {
                    Serial.printf(" - %s", reports[i].errorMessage.c_str());
                }
                
                Serial.println();
            }
        }
        
        Serial.printf("\n总计: %d/%d 通过 (%.1f%%)\n", 
                     passedTests, totalTests, 
                     totalTests > 0 ? (float)passedTests / totalTests * 100 : 0);
    } else {
        TestReport report = getTestReport(testType);
        if (report.testType != TEST_ALL) {
            Serial.printf("%s: %s", 
                         getTestTypeName(report.testType).c_str(),
                         getTestResultName(report.result).c_str());
            
            if (report.duration > 0) {
                Serial.printf(" (%lums)", report.duration);
            }
            
            if (report.errorMessage.length() > 0) {
                Serial.printf(" - %s", report.errorMessage.c_str());
            }
            
            Serial.println();
        }
    }
    
    Serial.println("=== 报告结束 ===");
}

/**
 * @brief 清除测试报告
 */
void TestManager::clearTestReports() {
    for (int i = 0; i < TEST_TYPE_COUNT; i++) {
        reports[i].testType = TEST_ALL; // 标记为无效
        reports[i].result = TEST_ERROR;
        reports[i].description = "";
        reports[i].errorMessage = "";
        reports[i].duration = 0;
        reports[i].timestamp = 0;
    }
    completedTests = 0;
}

/**
 * @brief 获取测试通过率
 * @return float 通过率（0.0-1.0）
 */
float TestManager::getTestPassRate() {
    int passedTests = 0;
    int totalTests = 0;
    
    for (int i = 0; i < TEST_TYPE_COUNT; i++) {
        if (reports[i].testType != TEST_ALL) {
            totalTests++;
            if (reports[i].result == TEST_SUCCESS) {
                passedTests++;
            }
        }
    }
    
    return totalTests > 0 ? (float)passedTests / totalTests : 0.0;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String TestManager::getLastError() {
    return lastError;
}

/**
 * @brief 创建测试报告
 * @param testType 测试类型
 * @param result 测试结果
 * @param description 测试描述
 * @param errorMessage 错误信息
 * @param duration 测试耗时
 */
void TestManager::createTestReport(TestType testType, TestResult result, 
                                  const String& description, const String& errorMessage, 
                                  unsigned long duration) {
    if (testType < TEST_TYPE_COUNT) {
        reports[testType].testType = testType;
        reports[testType].result = result;
        reports[testType].description = description;
        reports[testType].errorMessage = errorMessage;
        reports[testType].duration = duration;
        reports[testType].timestamp = millis();
        
        if (reports[testType].testType == testType) {
            // 如果是新的测试报告
            completedTests++;
        }
    }
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void TestManager::setError(const String& error) {
    lastError = error;
    if (config.enableDetailedLog) {
        Serial.printf("测试错误: %s\n", error.c_str());
    }
}

/**
 * @brief 获取测试类型名称
 * @param testType 测试类型
 * @return String 测试类型名称
 */
String TestManager::getTestTypeName(TestType testType) {
    switch (testType) {
        case TEST_GSM_BASIC: return "GSM基础测试";
        case TEST_SMS_SEND: return "短信发送测试";
        case TEST_SMS_RECEIVE: return "短信接收测试";
        case TEST_PHONE_CALL: return "电话拨打测试";
        case TEST_NETWORK_STATUS: return "网络状态测试";
        case TEST_SIGNAL_STRENGTH: return "信号强度测试";
        case TEST_SIM_CARD: return "SIM卡测试";
        case TEST_ALL: return "全部测试";
        default: return "未知测试";
    }
}

/**
 * @brief 获取测试结果名称
 * @param result 测试结果
 * @return String 测试结果名称
 */
String TestManager::getTestResultName(TestResult result) {
    switch (result) {
        case TEST_SUCCESS: return "成功";
        case TEST_FAILED: return "失败";
        case TEST_SKIPPED: return "跳过";
        case TEST_ERROR: return "错误";
        case TEST_TIMEOUT: return "超时";
        case TEST_NOT_IMPLEMENTED: return "未实现";
        default: return "未知";
    }
}