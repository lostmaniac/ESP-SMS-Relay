# HTTP客户端模块

## 概述

HTTP客户端模块是ESP-SMS-Relay项目的核心组件之一，提供了基于AT命令的HTTP/HTTPS请求功能。该模块支持GET和POST方法，可以配置自定义请求头，并提供完整的错误处理和响应解析功能。

## 特性

- ✅ 支持HTTP和HTTPS协议
- ✅ 支持GET和POST请求方法
- ✅ 支持自定义请求头配置
- ✅ 自动网络状态检查和PDP上下文管理
- ✅ 完整的错误处理机制
- ✅ 响应内容解析和状态码处理
- ✅ 调试模式支持
- ✅ 内存安全管理

## 文件结构

```
lib/http_client/
├── http_client.h              # HTTP客户端类定义
├── http_client.cpp            # HTTP客户端实现
├── http_client_example.h      # 使用示例类定义
├── http_client_example.cpp    # 使用示例实现
└── README.md                  # 本文档
```

## 依赖模块

- `AtCommandHandler`: AT命令处理器
- `GsmService`: GSM服务模块
- `ModuleManager`: 模块管理器

## 快速开始

### 1. 模块初始化

```cpp
#include "module_manager.h"
#include "http_client.h"

void setup() {
    Serial.begin(115200);
    
    // 初始化所有模块
    ModuleManager& moduleManager = ModuleManager::getInstance();
    if (!moduleManager.initializeAllModules()) {
        Serial.println("模块初始化失败: " + moduleManager.getLastError());
        return;
    }
    
    // 获取HTTP客户端实例
    HttpClient* httpClient = getHttpClient();
    if (!httpClient) {
        Serial.println("HTTP客户端未初始化");
        return;
    }
}
```

### 2. 简单GET请求

```cpp
void simpleGetExample() {
    HttpClient* httpClient = getHttpClient();
    
    HttpRequest request;
    request.method = HTTP_GET;
    request.url = "http://httpbin.org/get";
    
    HttpResponse response = httpClient->sendRequest(request);
    
    if (response.error == HTTP_SUCCESS) {
        Serial.printf("请求成功! 状态码: %d\n", response.statusCode);
        Serial.println("响应内容: " + response.body);
    } else {
        Serial.println("请求失败: " + httpClient->getErrorString(response.error));
    }
}
```

### 3. 带请求头的GET请求

```cpp
void getWithHeadersExample() {
    HttpClient* httpClient = getHttpClient();
    
    HttpRequest request;
    request.method = HTTP_GET;
    request.url = "http://httpbin.org/headers";
    request.headers.push_back({"User-Agent", "ESP32-Device/1.0"});
    request.headers.push_back({"Accept", "application/json"});
    
    HttpResponse response = httpClient->sendRequest(request);
    
    if (response.error == HTTP_SUCCESS) {
        Serial.println("GET请求成功!");
        Serial.println(response.body);
    }
}
```

### 4. POST请求

```cpp
void postExample() {
    HttpClient* httpClient = getHttpClient();
    
    HttpRequest request;
    request.method = HTTP_POST;
    request.url = "http://httpbin.org/post";
    request.headers.push_back({"Content-Type", "application/json"});
    request.body = "{\"message\":\"Hello from ESP32\"}";
    
    HttpResponse response = httpClient->sendRequest(request);
    
    if (response.error == HTTP_SUCCESS) {
        Serial.println("POST请求成功!");
        Serial.println(response.body);
    }
}
```

### 5. HTTPS请求

```cpp
void httpsExample() {
    HttpClient* httpClient = getHttpClient();
    
    HttpRequest request;
    request.method = HTTP_GET;
    request.url = "https://httpbin.org/get";  // 注意使用https://
    
    HttpResponse response = httpClient->sendRequest(request);
    
    if (response.error == HTTP_SUCCESS) {
        Serial.println("HTTPS请求成功!");
        Serial.println(response.body);
    }
}
```

## API参考

### HttpClient类

#### 构造函数
```cpp
HttpClient(AtCommandHandler& atHandler, GsmService& gsmService)
```

#### 主要方法

##### initialize()
```cpp
bool initialize()
```
初始化HTTP客户端。

**返回值:**
- `true`: 初始化成功
- `false`: 初始化失败

##### sendRequest()
```cpp
HttpResponse sendRequest(const HttpRequest& request)
```
发送HTTP请求。

**参数:**
- `request`: HTTP请求对象

**返回值:**
- `HttpResponse`: 包含响应数据和错误信息的响应对象

##### get()
```cpp
HttpResponse get(const String& url, const std::vector<HttpHeader>& headers = {})
```
发送GET请求的便捷方法。

**参数:**
- `url`: 请求URL
- `headers`: 可选的请求头列表

**返回值:**
- `HttpResponse`: 响应对象

##### post()
```cpp
HttpResponse post(const String& url, const String& body, const std::vector<HttpHeader>& headers = {})
```
发送POST请求的便捷方法。

**参数:**
- `url`: 请求URL
- `body`: 请求体内容
- `headers`: 可选的请求头列表

**返回值:**
- `HttpResponse`: 响应对象

##### setDebugMode()
```cpp
void setDebugMode(bool enabled)
```
设置调试模式。

**参数:**
- `enabled`: 是否启用调试模式

##### getLastError()
```cpp
String getLastError() const
```
获取最后的错误信息。

**返回值:**
- `String`: 错误信息字符串

##### getErrorString()
```cpp
String getErrorString(HttpClientError error) const
```
将错误代码转换为可读的错误信息。

**参数:**
- `error`: 错误代码

**返回值:**
- `String`: 错误描述字符串

### 数据结构

#### HttpMethod枚举
```cpp
enum HttpMethod {
    HTTP_GET = 0,   ///< GET方法
    HTTP_POST = 1   ///< POST方法
};
```

#### HttpProtocol枚举
```cpp
enum HttpProtocol {
    PROTOCOL_HTTP = 0,   ///< HTTP协议
    PROTOCOL_HTTPS = 1   ///< HTTPS协议
};
```

#### HttpClientError枚举
```cpp
enum HttpClientError {
    HTTP_SUCCESS = 0,              ///< 成功
    HTTP_ERROR_NETWORK = 1,        ///< 网络错误
    HTTP_ERROR_PDP_CONTEXT = 2,    ///< PDP上下文错误
    HTTP_ERROR_INIT = 3,           ///< 初始化错误
    HTTP_ERROR_URL = 4,            ///< URL错误
    HTTP_ERROR_SEND_DATA = 5,      ///< 发送数据错误
    HTTP_ERROR_ACTION = 6,         ///< HTTP动作错误
    HTTP_ERROR_READ = 7,           ///< 读取响应错误
    HTTP_ERROR_TIMEOUT = 8,        ///< 超时错误
    HTTP_ERROR_UNKNOWN = 9         ///< 未知错误
};
```

#### HttpHeader结构体
```cpp
struct HttpHeader {
    String name;     ///< 请求头名称
    String value;    ///< 请求头值
};
```

#### HttpRequest结构体
```cpp
struct HttpRequest {
    HttpMethod method;                      ///< 请求方法
    String url;                            ///< 请求URL
    String body;                           ///< 请求体（POST请求使用）
    std::vector<HttpHeader> headers;       ///< 请求头列表
};
```

#### HttpResponse结构体
```cpp
struct HttpResponse {
    HttpClientError error;    ///< 错误代码
    int statusCode;          ///< HTTP状态码
    String body;             ///< 响应体内容
    size_t contentLength;    ///< 内容长度
};
```

## 错误处理

### 错误类型

1. **网络错误** (`HTTP_ERROR_NETWORK`): 网络连接问题
2. **PDP上下文错误** (`HTTP_ERROR_PDP_CONTEXT`): 数据连接问题
3. **初始化错误** (`HTTP_ERROR_INIT`): HTTP服务初始化失败
4. **URL错误** (`HTTP_ERROR_URL`): URL格式错误或设置失败
5. **发送数据错误** (`HTTP_ERROR_SEND_DATA`): POST数据发送失败
6. **HTTP动作错误** (`HTTP_ERROR_ACTION`): HTTP请求执行失败
7. **读取响应错误** (`HTTP_ERROR_READ`): 响应读取失败
8. **超时错误** (`HTTP_ERROR_TIMEOUT`): 请求超时
9. **未知错误** (`HTTP_ERROR_UNKNOWN`): 其他未知错误

### 错误处理示例

```cpp
void handleHttpError(const HttpResponse& response, HttpClient* httpClient) {
    if (response.error != HTTP_SUCCESS) {
        Serial.println("HTTP请求失败:");
        Serial.println("错误代码: " + String(response.error));
        Serial.println("错误描述: " + httpClient->getErrorString(response.error));
        Serial.println("详细信息: " + httpClient->getLastError());
        
        // 根据错误类型采取不同的处理策略
        switch (response.error) {
            case HTTP_ERROR_NETWORK:
                Serial.println("建议: 检查网络连接和信号强度");
                break;
            case HTTP_ERROR_PDP_CONTEXT:
                Serial.println("建议: 重新激活PDP上下文");
                break;
            case HTTP_ERROR_URL:
                Serial.println("建议: 检查URL格式是否正确");
                break;
            default:
                Serial.println("建议: 稍后重试");
                break;
        }
    }
}
```

## 最佳实践

### 1. 网络状态检查

在发送HTTP请求之前，建议先检查网络状态：

```cpp
void checkNetworkBeforeRequest() {
    HttpClient* httpClient = getHttpClient();
    
    // 检查网络状态
    if (!httpClient->checkNetworkStatus()) {
        Serial.println("网络未就绪，请稍后重试");
        return;
    }
    
    // 发送请求
    // ...
}
```

### 2. 请求头设置

为了更好的兼容性，建议设置适当的请求头：

```cpp
void setRecommendedHeaders(HttpRequest& request) {
    request.headers.push_back({"User-Agent", "ESP32-SMS-Relay/1.0"});
    request.headers.push_back({"Accept", "*/*"});
    request.headers.push_back({"Connection", "close"});
    
    // 对于POST请求，设置Content-Type
    if (request.method == HTTP_POST && !request.body.isEmpty()) {
        request.headers.push_back({"Content-Type", "application/json"});
    }
}
```

### 3. 内存管理

对于大型响应，注意内存使用：

```cpp
void handleLargeResponse(const HttpResponse& response) {
    if (response.contentLength > 10000) { // 10KB
        Serial.println("警告: 响应内容较大，注意内存使用");
        
        // 可以考虑分块处理或只处理部分内容
        String partialContent = response.body.substring(0, 1000);
        Serial.println("部分内容: " + partialContent);
    }
}
```

### 4. 重试机制

实现简单的重试机制：

```cpp
HttpResponse sendRequestWithRetry(HttpClient* httpClient, const HttpRequest& request, int maxRetries = 3) {
    HttpResponse response;
    
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        Serial.printf("尝试发送请求 (第%d次)...\n", attempt);
        
        response = httpClient->sendRequest(request);
        
        if (response.error == HTTP_SUCCESS) {
            Serial.println("请求成功!");
            break;
        }
        
        Serial.printf("请求失败: %s\n", httpClient->getErrorString(response.error).c_str());
        
        if (attempt < maxRetries) {
            Serial.printf("等待%d秒后重试...\n", attempt * 2);
            delay(attempt * 2000); // 递增延迟
        }
    }
    
    return response;
}
```

## 调试

### 启用调试模式

```cpp
void enableDebugMode() {
    HttpClient* httpClient = getHttpClient();
    httpClient->setDebugMode(true);
    
    // 现在所有HTTP操作都会输出详细的调试信息
}
```

### 调试输出示例

启用调试模式后，您将看到类似以下的输出：

```
[HTTP] 检查网络状态...
[HTTP] 网络已注册，信号强度: 15
[HTTP] 检查PDP上下文状态...
[HTTP] PDP上下文已激活
[HTTP] 初始化HTTP服务...
[HTTP] 发送命令: AT+HTTPINIT
[HTTP] 响应: OK
[HTTP] 设置URL: http://httpbin.org/get
[HTTP] 发送命令: AT+HTTPPARA="URL","http://httpbin.org/get"
[HTTP] 响应: OK
[HTTP] 执行HTTP GET请求...
[HTTP] 发送命令: AT+HTTPACTION=0
[HTTP] 响应: OK
[HTTP] 等待HTTP动作完成...
[HTTP] 收到: +HTTPACTION: 0,200,312
[HTTP] HTTP请求完成，状态码: 200，数据长度: 312
[HTTP] 读取响应内容...
[HTTP] 响应读取完成
[HTTP] 终止HTTP服务...
```

## 集成状态

✅ **已完成集成到模块管理器**
- HTTP客户端已集成到 `ModuleManager` 中
- 支持通过 `MODULE_HTTP_CLIENT` 类型进行管理
- 提供 `getHttpClient()` 全局访问函数
- AT命令处理器已集成并支持串口参数
- GSM服务依赖已正确配置

## 编译状态

✅ **编译成功**
- 所有依赖关系已正确配置
- AT命令处理器集成完成
- GSM服务集成完成
- 模块管理器集成完成
- 内存使用: RAM 6.0%, Flash 10.5%

## 测试文件

📁 **可用的测试和示例文件**
- `examples/http_client_usage.cpp` - 基础使用示例
- `examples/test_http_integration.cpp` - 集成测试示例
- `test/test_http_client.cpp` - 单元测试
- `lib/http_client/http_client_example.cpp` - 详细示例

## 示例代码

完整的使用示例请参考：
- `examples/http_client_usage.cpp` - 基本使用示例
- `lib/http_client/http_client_example.cpp` - 详细功能示例

## 注意事项

1. **网络依赖**: HTTP客户端依赖稳定的网络连接，请确保GSM模块已正确连接到网络
2. **内存限制**: ESP32的内存有限，避免处理过大的HTTP响应
3. **超时设置**: 网络请求可能需要较长时间，请设置合适的超时值
4. **并发限制**: 当前实现不支持并发请求，请确保前一个请求完成后再发送下一个
5. **SSL证书**: HTTPS请求依赖GSM模块的SSL支持，某些自签名证书可能无法验证

## 故障排除

### 常见问题

1. **初始化失败**
   - 检查GSM模块是否正常工作
   - 确认SIM卡已插入且有效
   - 检查网络信号强度

2. **网络错误**
   - 使用`AT+CSQ`检查信号强度
   - 使用`AT+CREG?`检查网络注册状态
   - 使用`AT+CGREG?`检查GPRS注册状态

3. **PDP上下文错误**
   - 检查APN设置是否正确
   - 确认运营商支持数据服务
   - 尝试手动激活PDP上下文

4. **HTTP请求失败**
   - 检查URL格式是否正确
   - 确认目标服务器可访问
   - 检查请求头设置

5. **内存不足**
   - 减少响应内容大小
   - 优化代码中的字符串使用
   - 考虑分块处理大型响应

### 诊断命令

```cpp
void diagnosticCommands() {
    GsmService& gsm = GsmService::getInstance();
    
    // 检查信号强度
    Serial.println("信号强度: " + String(gsm.getSignalStrength()));
    
    // 检查网络注册状态
    GsmNetworkStatus status = gsm.getNetworkRegistrationStatus();
    Serial.println("网络状态: " + String(status));
    
    // 检查模块状态
    GsmModuleStatus moduleStatus = gsm.getModuleStatus();
    Serial.println("模块状态: " + String(moduleStatus));
}
```

## 版本历史

- **v1.0.0** (2024): 初始版本
  - 基本HTTP/HTTPS GET/POST支持
  - 自定义请求头支持
  - 错误处理和调试功能
  - 网络状态检查
  - 模块化设计

## 许可证

本模块是ESP-SMS-Relay项目的一部分，遵循项目的开源许可证。