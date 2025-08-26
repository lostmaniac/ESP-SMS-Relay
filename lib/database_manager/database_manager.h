/**
 * @file database_manager.h
 * @brief 数据库管理器头文件
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. SQLite数据库的初始化和管理
 * 2. 数据库表结构的创建和维护
 * 3. 数据库文件的管理
 * 4. 对外提供统一的数据库操作接口
 */

#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <Arduino.h>
#include <sqlite3.h>
#include <vector>
#include <map>
#include <mutex>

/**
 * @enum DatabaseStatus
 * @brief 数据库状态枚举
 */
enum DatabaseStatus {
    DB_NOT_INITIALIZED,     ///< 数据库未初始化
    DB_INITIALIZING,        ///< 数据库初始化中
    DB_READY,              ///< 数据库就绪
    DB_ERROR               ///< 数据库错误
};

/**
 * @struct APConfig
 * @brief AP配置结构体
 */
struct APConfig {
    String ssid;           ///< WiFi热点名称
    String password;       ///< WiFi热点密码
    bool enabled;          ///< 是否启用AP模式
    int channel;           ///< WiFi信道
    int maxConnections;    ///< 最大连接数
    String createdAt;      ///< 创建时间
    String updatedAt;      ///< 修改时间
};



/**
 * @struct ForwardRule
 * @brief 短信转发规则结构体
 */
struct ForwardRule {
    int id;                ///< 规则ID
    String ruleName;       ///< 规则名称
    String sourceNumber;   ///< 发送号码，可以为空，支持通配符
    String keywords;       ///< 关键词过滤
    String pushType;       ///< 推送类型：例如企业微信群机器人，钉钉群机器人，webhook
    String pushConfig;     ///< 推送配置为json格式，支持配置模板
    bool enabled;          ///< 是否使用
    bool isDefaultForward; ///< 是否默认转发（忽略关键词匹配）
    String createdAt;      ///< 创建时间
    String updatedAt;      ///< 修改时间
};

/**
 * @struct SMSRecord
 * @brief 短信记录结构体
 */
struct SMSRecord {
    int id;                ///< 记录ID
    String fromNumber;     ///< 发送方号码，支持索引
    String toNumber;       ///< 接收方号码
    String content;        ///< 短信内容，支持索引
    int ruleId;            ///< 匹配的转发规则ID
    bool forwarded;        ///< 是否已转发
    String status;         ///< 状态：received, forwarded, failed等
    String forwardedAt;    ///< 转发时间
    time_t receivedAt;     ///< 接收时间，time保存，支持索引，方便以后按照时间过滤短信
};

/**
 * @struct DatabaseInfo
 * @brief 数据库信息结构体
 */
struct DatabaseInfo {
    String dbPath;         ///< 数据库文件路径
    size_t dbSize;         ///< 数据库文件大小
    int tableCount;        ///< 表数量
    int recordCount;       ///< 总记录数
    String version;        ///< 数据库版本
    bool isOpen;           ///< 数据库是否打开
    String lastModified;   ///< 最后修改时间
};

/**
 * @class DatabaseManager
 * @brief 数据库管理器类
 * 
 * 负责SQLite数据库的统一管理和操作
 */
class DatabaseManager {
public:
    /**
     * @brief 获取单例实例
     * @return DatabaseManager& 单例引用
     */
    static DatabaseManager& getInstance();

    /**
     * @brief 初始化数据库
     * @param dbPath 数据库文件路径
     * @param createIfNotExists 如果数据库不存在是否创建
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize(const String& dbPath = "sms_relay.db", bool createIfNotExists = true);

    /**
     * @brief 关闭数据库连接
     * @return true 关闭成功
     * @return false 关闭失败
     */
    bool close();

    /**
     * @brief 检查数据库是否就绪
     * @return true 数据库就绪
     * @return false 数据库未就绪
     */
    bool isReady() const;

    /**
     * @brief 获取数据库状态
     * @return DatabaseStatus 数据库状态
     */
    DatabaseStatus getStatus() const;

    /**
     * @brief 获取数据库信息
     * @return DatabaseInfo 数据库信息
     */
    DatabaseInfo getDatabaseInfo();

    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError() const;

    // AP配置管理
    /**
     * @brief 获取AP配置
     * @return APConfig AP配置
     */
    APConfig getAPConfig();

    /**
     * @brief 更新AP配置
     * @param config AP配置
     * @return true 更新成功
     * @return false 更新失败
     */
    bool updateAPConfig(const APConfig& config);

    // STA配置管理
    

    // 转发规则管理
    /**
     * @brief 添加转发规则
     * @param rule 转发规则
     * @return int 规则ID，-1表示失败
     */
    int addForwardRule(const ForwardRule& rule);

    /**
     * @brief 更新转发规则
     * @param rule 转发规则
     * @return true 更新成功
     * @return false 更新失败
     */
    bool updateForwardRule(const ForwardRule& rule);

    /**
     * @brief 删除转发规则
     * @param ruleId 规则ID
     * @return true 删除成功
     * @return false 删除失败
     */
    bool deleteForwardRule(int ruleId);

    /**
     * @brief 获取所有转发规则
     * @return std::vector<ForwardRule> 转发规则列表
     */
    std::vector<ForwardRule> getAllForwardRules();

    /**
     * @brief 根据ID获取转发规则
     * @param ruleId 规则ID
     * @return ForwardRule 转发规则
     */
    ForwardRule getForwardRuleById(int ruleId);

    /**
     * @brief 获取转发规则总数
     * @return int 规则总数
     */
    int getForwardRuleCount();

    /**
     * @brief 获取启用的转发规则数量
     * @return int 启用的规则数量
     */
    int getEnabledForwardRuleCount();

    // 短信记录管理
    /**
     * @brief 添加短信记录
     * @param record 短信记录
     * @return int 记录ID，-1表示失败
     */
    int addSMSRecord(const SMSRecord& record);

    /**
     * @brief 更新短信记录
     * @param record 短信记录
     * @return true 更新成功
     * @return false 更新失败
     */
    bool updateSMSRecord(const SMSRecord& record);

    /**
     * @brief 获取短信记录
     * @param limit 限制数量
     * @param offset 偏移量
     * @return std::vector<SMSRecord> 短信记录列表
     */
    std::vector<SMSRecord> getSMSRecords(int limit = 100, int offset = 0);

    /**
     * @brief 根据ID获取短信记录
     * @param recordId 记录ID
     * @return SMSRecord 短信记录
     */
    SMSRecord getSMSRecordById(int recordId);

    /**
     * @brief 获取短信记录总数
     * @return int 短信记录总数
     */
    int getSMSRecordCount();

    /**
     * @brief 删除过期的短信记录
     * @param daysOld 保留天数
     * @return int 删除的记录数
     */
    int deleteOldSMSRecords(int daysOld = 30);

    /**
     * @brief 按数量清理短信记录（保留最新的指定数量）
     * @param keepCount 保留的记录数量
     * @return int 删除的记录数
     */
    int cleanupSMSRecordsByCount(int keepCount = 10000);

    /**
     * @brief 检查并执行短信记录清理（如果超过指定数量）
     * @param maxCount 最大允许的记录数量
     * @param keepCount 清理后保留的记录数量
     * @return int 删除的记录数，0表示无需清理
     */
    int checkAndCleanupSMSRecords(int maxCount = 10000, int keepCount = 8000);

    /**
     * @brief 启用调试模式
     * @param enable 是否启用
     */
    void setDebugMode(bool enable);

    /**
     * @brief 执行WAL检查点，将WAL文件中的更改写入主数据库文件
     */
    void checkpoint();

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    DatabaseManager();

    /**
     * @brief 析构函数
     */
    ~DatabaseManager();

    /**
     * @brief 禁用拷贝构造函数
     */
    DatabaseManager(const DatabaseManager&) = delete;

    /**
     * @brief 禁用赋值操作符
     */
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /**
     * @brief 创建数据库表
     * @return true 创建成功
     * @return false 创建失败
     */
    bool createTables();

    /**
     * @brief 初始化默认数据
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initializeDefaultData();

    /**
     * @brief 执行SQL语句
     * @param sql SQL语句
     * @return true 执行成功
     * @return false 执行失败
     */
    bool executeSQL(const String& sql);

    /**
     * @brief 执行查询SQL语句
     * @param sql SQL语句
     * @param callback 回调函数
     * @param data 回调数据
     * @return true 执行成功
     * @return false 执行失败
     */
    bool executeQuery(const String& sql, int (*callback)(void*, int, char**, char**), void* data);

    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);

    /**
     * @brief 调试输出
     * @param message 调试信息
     */
    void debugPrint(const String& message);

    /**
     * @brief 获取当前时间戳
     * @return String 时间戳字符串
     */
    String getCurrentTimestamp();



private:
    sqlite3* db;                    ///< SQLite数据库连接
    DatabaseStatus status;          ///< 数据库状态
    String dbPath;                  ///< 数据库文件路径
    String lastError;               ///< 最后的错误信息
    bool debugMode;                 ///< 调试模式
    DatabaseInfo dbInfo;            ///< 数据库信息
    mutable std::mutex dbMutex;     ///< 数据库操作互斥锁
};

#endif // DATABASE_MANAGER_H