/**
 * @file constants.h
 * @brief 系统常量定义文件
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 定义系统中使用的所有常量，避免硬编码值
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

// ==================== 系统配置常量 ====================

/// 系统版本信息
#define SYSTEM_VERSION "1.1.0"
#define SYSTEM_BUILD_YEAR 2024

/// 默认配置值
#define DEFAULT_WIFI_TIMEOUT_MS 30000
#define DEFAULT_HTTP_TIMEOUT_MS 30000
#define DEFAULT_AT_COMMAND_TIMEOUT_MS 5000
#define DEFAULT_GSM_INIT_TIMEOUT_MS 30000
#define DEFAULT_SMS_SEND_TIMEOUT_MS 30000
#define DEFAULT_PHONE_CALL_TIMEOUT_MS 10000
#define DEFAULT_UART_BAUD_RATE 115200
#define DEFAULT_WATCHDOG_TIMEOUT_MS 60000

/// AP 配置默认值
#define DEFAULT_AP_SSID "ESP-SMS-Relay"
#define DEFAULT_AP_PASSWORD "12345678"
#define DEFAULT_AP_CHANNEL 1
#define DEFAULT_AP_MAX_CONNECTIONS 4

/// STA 配置默认值 (用于首次启动时数据库为空的情况)
#define DEFAULT_STA_SSID ""
#define DEFAULT_STA_PASSWORD ""
#define DEFAULT_STA_ENABLED 0

/// 终端管理器配置
#define MAX_FORWARD_RULES 50
#define RULE_CACHE_SIZE 20

// ==================== 缓冲区大小常量 ====================

/// 字符串缓冲区大小
#define SMALL_BUFFER_SIZE 64
#define MEDIUM_BUFFER_SIZE 256
#define LARGE_BUFFER_SIZE 512
#define EXTRA_LARGE_BUFFER_SIZE 1024

/// 特定用途缓冲区
#define LOG_BUFFER_SIZE 1024
#define SMS_CONTENT_MAX_SIZE 1000
#define PHONE_NUMBER_MAX_SIZE 20
#define SSID_MAX_SIZE 32
#define PASSWORD_MAX_SIZE 64
#define URL_MAX_SIZE 512
#define JSON_BUFFER_SIZE 2048

// ==================== 网络配置常量 ====================

/// HTTP状态码
#define HTTP_STATUS_OK 200
#define HTTP_STATUS_CREATED 201
#define HTTP_STATUS_BAD_REQUEST 400
#define HTTP_STATUS_UNAUTHORIZED 401
#define HTTP_STATUS_FORBIDDEN 403
#define HTTP_STATUS_NOT_FOUND 404
#define HTTP_STATUS_INTERNAL_ERROR 500
#define HTTP_STATUS_SERVICE_UNAVAILABLE 503

/// 默认端口
#define DEFAULT_WEB_SERVER_PORT 80
#define DEFAULT_HTTPS_PORT 443

/// 网络重试配置
#define MAX_WIFI_RETRY_COUNT 3
#define MAX_HTTP_RETRY_COUNT 3
#define RETRY_DELAY_MS 1000

// ==================== 数据库配置常量 ====================

/// 数据库文件配置
#define DEFAULT_DB_PATH "/littlefs/sms_relay.db"
#define DB_BACKUP_PATH "/littlefs/sms_relay_backup.db"

/// 数据清理配置
#define DEFAULT_SMS_RETENTION_DAYS 30
#define MAX_SMS_RECORDS_COUNT 10000
#define SMS_CLEANUP_KEEP_COUNT 8000
#define SMS_BATCH_SIZE 100

/// 查询限制
#define DEFAULT_QUERY_LIMIT 100
#define MAX_QUERY_LIMIT 1000

// ==================== GSM/SMS配置常量 ====================

/// AT命令配置
#define AT_COMMAND_MAX_LENGTH 128
#define AT_RESPONSE_MAX_LENGTH 512
#define AT_COMMAND_RETRY_COUNT 3

/// SMS配置
#define SMS_PDU_MAX_LENGTH 320
#define SMS_TEXT_MAX_LENGTH 160
#define SMS_UNICODE_MAX_LENGTH 70

/// 信号强度阈值
#define SIGNAL_STRENGTH_EXCELLENT 20
#define SIGNAL_STRENGTH_GOOD 15
#define SIGNAL_STRENGTH_FAIR 10
#define SIGNAL_STRENGTH_POOR 5

// ==================== 推送服务配置常量 ====================

/// 推送类型
#define PUSH_TYPE_WECHAT "wechat"
#define PUSH_TYPE_DINGTALK "dingtalk"
#define PUSH_TYPE_FEISHU "feishu"
#define PUSH_TYPE_WEBHOOK "webhook"

/// 推送重试配置
#define MAX_PUSH_RETRY_COUNT 2
#define PUSH_RETRY_DELAY_MS 500

/// 推送消息长度限制
#define PUSH_MESSAGE_MAX_LENGTH 4096
#define PUSH_TITLE_MAX_LENGTH 100

// ==================== 文件系统配置常量 ====================

/// 文件路径
#define CONFIG_FILE_PATH "/config.json"
#define LOG_FILE_PATH "/logs/system.log"
#define BACKUP_DIR_PATH "/backup"

/// 文件大小限制
#define MAX_CONFIG_FILE_SIZE 8192
#define MAX_LOG_FILE_SIZE 1048576  // 1MB
#define MAX_BACKUP_FILES 5

// ==================== 任务调度配置常量 ====================

/// 任务优先级
#define TASK_PRIORITY_HIGH 3
#define TASK_PRIORITY_NORMAL 2
#define TASK_PRIORITY_LOW 1

/// 任务栈大小
#define TASK_STACK_SIZE_SMALL 2048
#define TASK_STACK_SIZE_MEDIUM 4096
#define TASK_STACK_SIZE_LARGE 8192

/// 任务执行间隔
#define TASK_INTERVAL_FAST_MS 1000
#define TASK_INTERVAL_NORMAL_MS 5000
#define TASK_INTERVAL_SLOW_MS 30000

// ==================== 模块管理配置常量 ====================

/// 模块数量
#define MAX_MODULE_COUNT 20

/// 模块初始化超时
#define MODULE_INIT_TIMEOUT_MS 10000
#define MODULE_START_TIMEOUT_MS 5000

// ==================== 调试配置常量 ====================

/// 调试输出配置
#define DEBUG_SERIAL_BAUD_RATE 115200
#define DEBUG_BUFFER_SIZE 256

/// 性能监控
#define PERFORMANCE_MONITOR_INTERVAL_MS 60000
#define MEMORY_CHECK_INTERVAL_MS 30000

// ==================== 安全配置常量 ====================

/// 密码强度要求
#define MIN_PASSWORD_LENGTH 8
#define MAX_PASSWORD_LENGTH 64

/// API访问限制
#define MAX_API_REQUESTS_PER_MINUTE 60
#define API_TOKEN_LENGTH 32

// ==================== 硬件配置常量 ====================

/// GPIO引脚（根据实际硬件配置）
#define SIM_POWER_PIN 4
#define SIM_RESET_PIN 5
#define LED_STATUS_PIN 2

/// 电源管理
#define LOW_BATTERY_THRESHOLD 3200  // mV
#define POWER_SAVE_TIMEOUT_MS 300000  // 5分钟

// ==================== 错误代码常量 ====================

/// 系统错误代码
#define ERROR_CODE_SUCCESS 0
#define ERROR_CODE_INIT_FAILED 1001
#define ERROR_CODE_CONFIG_INVALID 1002
#define ERROR_CODE_NETWORK_FAILED 1003
#define ERROR_CODE_DATABASE_ERROR 1004
#define ERROR_CODE_SMS_FAILED 1005
#define ERROR_CODE_PUSH_FAILED 1006
#define ERROR_CODE_MEMORY_INSUFFICIENT 1007
#define ERROR_CODE_TIMEOUT 1008
#define ERROR_CODE_INVALID_PARAMETER 1009
#define ERROR_CODE_PERMISSION_DENIED 1010

// ==================== 字符串常量 ====================

/// 系统消息
#define MSG_SYSTEM_STARTING "系统启动中..."
#define MSG_SYSTEM_READY "系统就绪"
#define MSG_SYSTEM_ERROR "系统错误"
#define MSG_NETWORK_CONNECTED "网络已连接"
#define MSG_NETWORK_DISCONNECTED "网络已断开"
#define MSG_SMS_RECEIVED "收到短信"
#define MSG_SMS_FORWARDED "短信已转发"

/// 配置键名
#define CONFIG_KEY_WIFI_SSID "wifi_ssid"
#define CONFIG_KEY_WIFI_PASSWORD "wifi_password"
#define CONFIG_KEY_AP_SSID "ap_ssid"
#define CONFIG_KEY_AP_PASSWORD "ap_password"
#define CONFIG_KEY_LOG_LEVEL "log_level"
#define CONFIG_KEY_DEBUG_MODE "debug_mode"

#endif // CONSTANTS_H