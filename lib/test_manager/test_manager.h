/**
 * @file test_manager.h
 * @brief 测试管理器 - 统一管理所有功能模块的测试
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 统一管理所有功能模块的测试
 * 2. 提供测试结果报告
 * 3. 支持单独测试和批量测试
 * 4. 测试数据管理
 */

#ifndef TEST_MANAGER_H
#define TEST_MANAGER_H

#include <Arduino.h>

/**
 * @enum TestResult
 * @brief 测试结果枚举
 */
enum TestResult {
    TEST_SUCCESS,           ///< 测试成功
    TEST_FAILED,           ///< 测试失败
    TEST_SKIPPED,          ///< 测试跳过
    TEST_ERROR,            ///< 测试错误
    TEST_TIMEOUT,          ///< 测试超时
    TEST_NOT_IMPLEMENTED   ///< 测试未实现
};

/**
 * @enum TestType
 * @brief 测试类型枚举
 */
enum TestType {
    TEST_GSM_BASIC,        ///< GSM基础功能测试
    TEST_SMS_SEND,         ///< 短信发送测试
    TEST_SMS_RECEIVE,      ///< 短信接收测试
    TEST_PHONE_CALL,       ///< 电话拨打测试
    TEST_NETWORK_STATUS,   ///< 网络状态测试
    TEST_SIGNAL_STRENGTH,  ///< 信号强度测试
    TEST_SIM_CARD,         ///< SIM卡测试
    TEST_ALL,              ///< 全部测试
    TEST_TYPE_COUNT        ///< 测试类型总数
};

/**
 * @struct TestReport
 * @brief 测试报告结构体
 */
struct TestReport {
    TestType testType;          ///< 测试类型
    TestResult result;          ///< 测试结果
    String description;         ///< 测试描述
    String errorMessage;        ///< 错误信息
    unsigned long duration;     ///< 测试耗时（毫秒）
    unsigned long timestamp;    ///< 测试时间戳
};

/**
 * @struct TestConfig
 * @brief 测试配置结构体
 */
struct TestConfig {
    String testPhoneNumber;     ///< 测试电话号码
    String testSmsNumber;       ///< 测试短信号码
    String testSmsContent;      ///< 测试短信内容
    unsigned long callDuration; ///< 通话持续时间（秒）
    bool enableDetailedLog;     ///< 是否启用详细日志
    unsigned long testTimeout;  ///< 测试超时时间（毫秒）
};

/**
 * @class TestManager
 * @brief 测试管理器类
 * 
 * 负责管理所有功能模块的测试执行和结果报告
 */
class TestManager {
public:
    /**
     * @brief 构造函数
     */
    TestManager();
    
    /**
     * @brief 析构函数
     */
    ~TestManager();
    
    /**
     * @brief 初始化测试管理器
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 设置测试配置
     * @param config 测试配置
     */
    void setTestConfig(const TestConfig& config);
    
    /**
     * @brief 获取测试配置
     * @return TestConfig 测试配置
     */
    TestConfig getTestConfig();
    
    /**
     * @brief 运行指定类型的测试
     * @param testType 测试类型
     * @return TestResult 测试结果
     */
    TestResult runTest(TestType testType);
    
    /**
     * @brief 运行所有测试
     * @return bool 所有测试是否通过
     */
    bool runAllTests();
    
    /**
     * @brief 获取测试报告
     * @param testType 测试类型
     * @return TestReport 测试报告
     */
    TestReport getTestReport(TestType testType);
    
    /**
     * @brief 获取所有测试报告
     * @param reports 测试报告数组
     * @param maxReports 最大报告数量
     * @return int 实际报告数量
     */
    int getAllTestReports(TestReport* reports, int maxReports);
    
    /**
     * @brief 打印测试报告
     * @param testType 测试类型（TEST_ALL表示打印所有）
     */
    void printTestReport(TestType testType = TEST_ALL);
    
    /**
     * @brief 清除测试报告
     */
    void clearTestReports();
    
    /**
     * @brief 获取测试通过率
     * @return float 通过率（0.0-1.0）
     */
    float getTestPassRate();
    
    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError();
    
    /**
     * @brief 获取单例实例
     * @return TestManager& 单例引用
     */
    static TestManager& getInstance();

private:
    TestConfig config;                          ///< 测试配置
    TestReport reports[TEST_TYPE_COUNT];        ///< 测试报告数组
    String lastError;                          ///< 最后的错误信息
    bool initialized;                          ///< 是否已初始化
    int completedTests;                        ///< 已完成的测试数量
    
    /**
     * @brief GSM基础功能测试
     * @return TestResult 测试结果
     */
    TestResult testGsmBasic();
    
    /**
     * @brief 短信发送测试
     * @return TestResult 测试结果
     */
    TestResult testSmsSend();
    
    /**
     * @brief 短信接收测试
     * @return TestResult 测试结果
     */
    TestResult testSmsReceive();
    
    /**
     * @brief 电话拨打测试
     * @return TestResult 测试结果
     */
    TestResult testPhoneCall();
    
    /**
     * @brief 网络状态测试
     * @return TestResult 测试结果
     */
    TestResult testNetworkStatus();
    
    /**
     * @brief 信号强度测试
     * @return TestResult 测试结果
     */
    TestResult testSignalStrength();
    
    /**
     * @brief SIM卡测试
     * @return TestResult 测试结果
     */
    TestResult testSimCard();
    
    /**
     * @brief 创建测试报告
     * @param testType 测试类型
     * @param result 测试结果
     * @param description 测试描述
     * @param errorMessage 错误信息
     * @param duration 测试耗时
     */
    void createTestReport(TestType testType, TestResult result, 
                         const String& description, const String& errorMessage, 
                         unsigned long duration);
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);
    
    /**
     * @brief 获取测试类型名称
     * @param testType 测试类型
     * @return String 测试类型名称
     */
    String getTestTypeName(TestType testType);
    
    /**
     * @brief 获取测试结果名称
     * @param result 测试结果
     * @return String 测试结果名称
     */
    String getTestResultName(TestResult result);
};

#endif // TEST_MANAGER_H