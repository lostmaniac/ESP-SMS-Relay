/**
 * @file http_client.h
 * @brief HTTP客户端工具类 - 基于AT命令实现HTTP/HTTPS请求
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 支持HTTP和HTTPS协议
 * 2. 支持GET和POST请求方法
 * 3. 支持自定义请求头配置
 * 4. 提供完整的响应处理
 * 5. 网络状态检查和错误处理
 */

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <Arduino.h>
#include <map>
#include "at_command_handler.h"
#include "gsm_service.h"
#include "../../include/constants.h"

/**
 * @enum HttpClientMethod
 * @brief HTTP请求方法枚举
 */
enum HttpClientMethod {
    HTTP_CLIENT_GET = 0,       ///< GET请求
    HTTP_CLIENT_POST = 1,      ///< POST请求
    HTTP_CLIENT_PUT = 2,       ///< PUT请求
    HTTP_CLIENT_DELETE = 3     ///< DELETE请求
};

/**
 * @enum HttpProtocol
 * @brief HTTP协议类型枚举
 */
enum HttpProtocol {
    HTTP_PROTOCOL,      ///< HTTP协议
    HTTPS_PROTOCOL      ///< HTTPS协议
};

/**
 * @enum HttpClientError
 * @brief HTTP客户端错误枚举
 */
enum HttpClientError {
    HTTP_SUCCESS = 0,           ///< 请求成功
    HTTP_ERROR_NETWORK = 1,     ///< 网络错误
    HTTP_ERROR_TIMEOUT = 2,     ///< 请求超时
    HTTP_ERROR_INIT = 3,        ///< 初始化失败
    HTTP_ERROR_INVALID_URL = 4, ///< 无效URL
    HTTP_ERROR_SERVER = 5,      ///< 服务器错误
    HTTP_ERROR_AT_COMMAND = 6   ///< AT命令错误
};

/**
 * @struct HttpRequest
 * @brief HTTP请求结构体
 */
struct HttpRequest {
    String url;                         ///< 请求URL
    HttpClientMethod method;            ///< 请求方法
    HttpProtocol protocol;              ///< 协议类型
    std::map<String, String> headers;  ///< 请求头
    String body;                        ///< 请求体（POST/PUT使用）
    unsigned long timeout;              ///< 超时时间(ms)
    
    /**
     * @brief 构造函数
     */
    HttpRequest() : 
        method(HTTP_CLIENT_GET), 
        protocol(HTTP_PROTOCOL), 
        timeout(DEFAULT_HTTP_TIMEOUT_MS) {}
};

/**
 * @struct HttpResponse
 * @brief HTTP响应结构体
 */
struct HttpResponse {
    HttpClientError error;              ///< 错误代码
    int statusCode;                     ///< HTTP状态码
    String body;                        ///< 响应体
    std::map<String, String> headers;  ///< 响应头
    unsigned long duration;             ///< 请求耗时(ms)
    size_t contentLength;               ///< 内容长度
    
    /**
     * @brief 构造函数
     */
    HttpResponse() : 
        error(HTTP_SUCCESS), 
        statusCode(0), 
        duration(0), 
        contentLength(0) {}
};

/**
 * @class HttpClient
 * @brief HTTP客户端类
 * 
 * 基于AT命令实现的HTTP/HTTPS客户端
 */
class HttpClient {
public:
    /**
     * @brief 构造函数
     * @param atHandler AT命令处理器引用
     * @param gsmService GSM服务引用
     */
    HttpClient(AtCommandHandler& atHandler, GsmService& gsmService);
    
    /**
     * @brief 析构函数
     */
    ~HttpClient();
    
    /**
     * @brief 初始化HTTP客户端
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 执行HTTP请求
     * @param request 请求参数
     * @return HttpResponse 响应结果
     */
    HttpResponse request(const HttpRequest& request);
    
    /**
     * @brief 执行GET请求
     * @param url 请求URL
     * @param headers 请求头（可选）
     * @param timeout 超时时间（可选）
     * @return HttpResponse 响应结果
     */
    HttpResponse get(const String& url, 
                    const std::map<String, String>& headers = {},
                    unsigned long timeout = DEFAULT_HTTP_TIMEOUT_MS);
    
    /**
     * @brief 执行POST请求
     * @param url 请求URL
     * @param body 请求体
     * @param headers 请求头（可选）
     * @param timeout 超时时间（可选）
     * @return HttpResponse 响应结果
     */
    HttpResponse post(const String& url, 
                     const String& body,
                     const std::map<String, String>& headers = {},
                     unsigned long timeout = DEFAULT_HTTP_TIMEOUT_MS);
    
    /**
     * @brief 检查网络连接状态
     * @return true 网络已连接
     * @return false 网络未连接
     */
    bool isNetworkConnected();
    
    /**
     * @brief 检查PDP上下文状态
     * @return true PDP上下文已激活
     * @return false PDP上下文未激活
     */
    bool isPdpContextActive();
    
    /**
     * @brief 配置APN
     * @param apn APN名称
     * @param username 用户名（可选）
     * @param password 密码（可选）
     * @return true 配置成功
     * @return false 配置失败
     */
    bool configureApn(const String& apn, const String& username = "", const String& password = "");
    
    /**
     * @brief 激活PDP上下文
     * @return true 激活成功
     * @return false 激活失败
     */
    bool activatePdpContext();
    
    /**
     * @brief 配置APN并激活PDP上下文
     * @param apn APN名称
     * @param username 用户名（可选）
     * @param password 密码（可选）
     * @return true 配置和激活成功
     * @return false 配置或激活失败
     */
    bool configureAndActivateApn(const String& apn, const String& username = "", const String& password = "");
    
    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError();
    
    /**
     * @brief 设置调试模式
     * @param enabled 是否启用调试模式
     */
    void setDebugMode(bool enabled);
    
    /**
     * @brief 获取详细的调试日志
     * @return String 调试日志内容
     */
    String getDebugLog();
    
    /**
     * @brief 清空调试日志
     */
    void clearDebugLog();
    
    /**
     * @brief 记录请求详细信息
     * @param request HTTP请求对象
     */
    void logRequestDetails(const HttpRequest& request);
    
    /**
     * @brief 记录响应详细信息
     * @param response HTTP响应对象
     */
    void logResponseDetails(const HttpResponse& response);
    
    /**
     * @brief 记录网络状态信息
     */
    void logNetworkStatus();
    
    /**
     * @brief 记录AT命令执行详情
     * @param command AT命令
     * @param response AT响应
     * @param duration 执行时长
     */
    void logAtCommandDetails(const String& command, const String& response, unsigned long duration);
    
    /**
     * @brief 设置默认超时时间
     * @param timeout 超时时间(ms)
     */
    void setDefaultTimeout(unsigned long timeout);
    
    /**
     * @brief 获取单例实例
     * @return HttpClient& 单例引用
     */
    static HttpClient& getInstance();

private:
    AtCommandHandler& atCommandHandler; ///< AT命令处理器引用
    GsmService& gsmService;            ///< GSM服务引用
    String lastError;                  ///< 最后的错误信息
    bool debugMode;                   ///< 调试模式
    bool initialized;                 ///< 是否已初始化
    bool httpServiceActive;             ///< HTTP服务是否激活
    unsigned long defaultTimeout;       ///< 默认超时时间
    
    // 调试日志相关成员
    String debugLog;                   ///< 调试日志缓冲区
    unsigned long maxLogSize;          ///< 最大日志大小
    unsigned long requestCount;       ///< 请求计数
    unsigned long lastLogTime;        ///< 最后日志时间
    
    /**
     * @brief 初始化HTTP服务
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initHttpService();
    
    /**
     * @brief 终止HTTP服务
     * @return true 终止成功
     * @return false 终止失败
     */
    bool terminateHttpService();
    
    /**
     * @brief 设置HTTP参数
     * @param parameter 参数名
     * @param value 参数值
     * @return true 设置成功
     * @return false 设置失败
     */
    bool setHttpParameter(const String& parameter, const String& value);
    
    /**
     * @brief 执行HTTP动作
     * @param method HTTP方法
     * @param timeout 超时时间
     * @return HttpResponse 响应结果
     */
    HttpResponse executeHttpAction(HttpClientMethod method, unsigned long timeout);
    
    /**
     * @brief 发送HTTP数据（用于POST请求）
     * @param data 要发送的数据
     * @param timeout 超时时间
     * @return true 发送成功
     * @return false 发送失败
     */
    bool sendHttpData(const String& data, unsigned long timeout);
    
    /**
     * @brief 读取HTTP响应
     * @param startPos 开始位置
     * @param length 读取长度
     * @return String 响应内容
     */
    String readHttpResponse(int startPos = 0, int length = 1000);
    
    /**
     * @brief 解析HTTP动作响应
     * @param response AT+HTTPACTION的响应
     * @param httpResponse HTTP响应结构体
     * @return true 解析成功
     * @return false 解析失败
     */
    bool parseHttpActionResponse(const String& response, HttpResponse& httpResponse);
    
    /**
     * @brief 检测URL协议类型
     * @param url URL字符串
     * @return HttpProtocol 协议类型
     */
    HttpProtocol detectProtocol(const String& url);
    
    /**
     * @brief 验证URL格式
     * @param url URL字符串
     * @return true URL有效
     * @return false URL无效
     */
    bool validateUrl(const String& url);
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);
    
    /**
     * @brief 打印调试信息
     * @param message 调试信息
     */
    void debugPrint(const String& message);
    
    /**
     * @brief 获取HTTP方法字符串
     * @param method HTTP方法
     * @return String 方法字符串
     */
    String getMethodString(HttpClientMethod method);
    
    /**
     * @brief 向调试日志添加内容
     * @param message 要添加的消息
     */
    void appendToDebugLog(const String& message);
};

#endif // HTTP_CLIENT_H