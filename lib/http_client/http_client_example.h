/**
 * @file http_client_example.h
 * @brief HTTP客户端使用示例
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该文件提供HTTP客户端的使用示例和最佳实践
 */

#ifndef HTTP_CLIENT_EXAMPLE_H
#define HTTP_CLIENT_EXAMPLE_H

#include "http_client.h"
#include <Arduino.h>

/**
 * @class HttpClientExample
 * @brief HTTP客户端使用示例类
 * 
 * 提供各种HTTP请求的示例代码
 */
class HttpClientExample {
public:
    /**
     * @brief 构造函数
     */
    HttpClientExample();
    
    /**
     * @brief 析构函数
     */
    ~HttpClientExample();
    
    /**
     * @brief 运行所有示例
     * @return true 所有示例执行成功
     * @return false 有示例执行失败
     */
    bool runAllExamples();
    
    /**
     * @brief 简单GET请求示例
     * @return true 请求成功
     * @return false 请求失败
     */
    bool simpleGetExample();
    
    /**
     * @brief 带请求头的GET请求示例
     * @return true 请求成功
     * @return false 请求失败
     */
    bool getWithHeadersExample();
    
    /**
     * @brief 简单POST请求示例
     * @return true 请求成功
     * @return false 请求失败
     */
    bool simplePostExample();
    
    /**
     * @brief JSON POST请求示例
     * @return true 请求成功
     * @return false 请求失败
     */
    bool jsonPostExample();
    
    /**
     * @brief HTTPS GET请求示例
     * @return true 请求成功
     * @return false 请求失败
     */
    bool httpsGetExample();
    
    /**
     * @brief HTTPS POST请求示例
     * @return true 请求成功
     * @return false 请求失败
     */
    bool httpsPostExample();
    
    /**
     * @brief 错误处理示例
     * @return true 示例执行完成
     * @return false 示例执行失败
     */
    bool errorHandlingExample();
    
    /**
     * @brief 自定义请求示例
     * @return true 请求成功
     * @return false 请求失败
     */
    bool customRequestExample();
    
    /**
     * @brief 网络状态检查示例
     * @return true 检查完成
     * @return false 检查失败
     */
    bool networkStatusExample();
    
    /**
     * @brief 打印HTTP响应信息
     * @param response HTTP响应
     * @param requestName 请求名称
     */
    void printResponse(const HttpResponse& response, const String& requestName);
    
    /**
     * @brief 获取错误描述
     * @param error 错误代码
     * @return String 错误描述
     */
    String getErrorDescription(HttpClientError error);

private:
    HttpClient* httpClient;     ///< HTTP客户端实例
    bool debugMode;            ///< 调试模式
    
    /**
     * @brief 打印示例标题
     * @param title 标题
     */
    void printExampleTitle(const String& title);
    
    /**
     * @brief 打印分隔线
     */
    void printSeparator();
};

// 全局函数声明

/**
 * @brief 运行HTTP客户端示例
 * @return true 示例运行成功
 * @return false 示例运行失败
 */
bool runHttpClientExamples();

/**
 * @brief 测试HTTP GET请求
 * @param url 测试URL
 * @return true 测试成功
 * @return false 测试失败
 */
bool testHttpGet(const String& url);

/**
 * @brief 测试HTTP POST请求
 * @param url 测试URL
 * @param data 发送数据
 * @return true 测试成功
 * @return false 测试失败
 */
bool testHttpPost(const String& url, const String& data);

/**
 * @brief 测试HTTPS请求
 * @param url 测试URL
 * @return true 测试成功
 * @return false 测试失败
 */
bool testHttpsRequest(const String& url);

#endif // HTTP_CLIENT_EXAMPLE_H