/**
 * @file database_manager.cpp
 * @brief 数据库管理器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "database_manager.h"
#include "../filesystem_manager/filesystem_manager.h"
#include <Arduino.h>
#include <time.h>

// 静态回调函数用于查询结果处理
static std::vector<std::map<String, String>> queryResults;
static bool querySuccess = false;

/**
 * @brief SQLite查询回调函数
 * @param data 用户数据
 * @param argc 列数
 * @param argv 列值数组
 * @param azColName 列名数组
 * @return int 0表示继续，非0表示停止
 */
static int queryCallback(void *data, int argc, char **argv, char **azColName) {
    std::map<String, String> row;
    for (int i = 0; i < argc; i++) {
        String colName = String(azColName[i]);
        String colValue = argv[i] ? String(argv[i]) : "";
        row[colName] = colValue;
    }
    queryResults.push_back(row);
    querySuccess = true;
    return 0;
}

/**
 * @brief 获取单例实例
 * @return DatabaseManager& 单例引用
 */
DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager instance;
    return instance;
}

/**
 * @brief 私有构造函数
 */
DatabaseManager::DatabaseManager() 
    : db(nullptr), status(DB_NOT_INITIALIZED), debugMode(false) {
    dbInfo.isOpen = false;
    dbInfo.version = "1.0";
    dbInfo.lastModified = "";
}

/**
 * @brief 析构函数
 */
DatabaseManager::~DatabaseManager() {
    close();
}

/**
 * @brief 初始化数据库
 * @param dbPath 数据库文件路径
 * @param createIfNotExists 如果数据库不存在是否创建
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool DatabaseManager::initialize(const String& dbPath, bool createIfNotExists) {
    // 自动处理数据库路径，确保使用正确的LittleFS路径
    String fullDbPath;
    if (dbPath.startsWith("/littlefs/")) {
        // 如果已经包含完整路径，直接使用
        fullDbPath = dbPath;
    } else if (dbPath.startsWith("/")) {
        // 如果是绝对路径但不包含littlefs前缀，添加前缀
        fullDbPath = "/littlefs" + dbPath;
    } else {
        // 如果是相对路径，添加完整前缀
        fullDbPath = "/littlefs/" + dbPath;
    }
    
    debugPrint("开始初始化数据库: " + fullDbPath);
    debugPrint("[DEBUG] 路径处理 - 输入路径: " + dbPath + ", 完整路径: " + fullDbPath);
    status = DB_INITIALIZING;
    this->dbPath = fullDbPath;
    
    // 检查LittleFS是否已挂载
    // 注意：LittleFS应该由FilesystemManager统一管理
    // 这里只检查文件系统是否可用，不进行初始化
    FilesystemManager& fsManager = FilesystemManager::getInstance();
    if (!fsManager.isReady()) {
        setError("LittleFS文件系统未挂载或不可用，请先初始化FilesystemManager");
        status = DB_ERROR;
        return false;
    }
    
    debugPrint("文件系统检查通过，LittleFS已就绪");
    
    // 检查数据库文件是否存在
    // 从完整路径中提取LittleFS相对路径
    String checkPath = fullDbPath.substring(9); // 去掉 "/littlefs/" 前缀
    debugPrint("[DEBUG] 检查文件存在性 - 完整路径: " + fullDbPath + ", LittleFS路径: " + checkPath);
    bool dbExists = LittleFS.exists(checkPath);
    debugPrint("数据库文件存在: " + String(dbExists ? "是" : "否"));
    
    if (!dbExists && !createIfNotExists) {
        setError("数据库文件不存在且未启用创建选项");
        status = DB_ERROR;
        return false;
    }
    
    // 初始化SQLite
    int rc = sqlite3_initialize();
    if (rc != SQLITE_OK) {
        setError("SQLite初始化失败: " + String(rc));
        status = DB_ERROR;
        return false;
    }
    
    // 打开数据库
    rc = sqlite3_open(fullDbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        setError("无法打开数据库: " + String(sqlite3_errmsg(db)));
        sqlite3_close(db);
        db = nullptr;
        status = DB_ERROR;
        return false;
    }
    
    debugPrint("数据库连接成功");
    dbInfo.isOpen = true;
    dbInfo.dbPath = fullDbPath;
    
    // 只有在数据库文件不存在时才创建表结构和初始化默认数据
    if (!dbExists) {
        debugPrint("数据库文件不存在，开始创建表结构");
        if (!createTables()) {
            setError("创建数据库表失败");
            close();
            status = DB_ERROR;
            return false;
        }
        
        // 初始化默认数据
        if (!initializeDefaultData()) {
            setError("初始化默认数据失败");
            close();
            status = DB_ERROR;
            return false;
        }
        debugPrint("新数据库初始化完成");
    } else {
        debugPrint("数据库文件已存在，跳过表结构创建");
    }
    
    status = DB_READY;
    debugPrint("数据库初始化完成");
    return true;
}

/**
 * @brief 关闭数据库连接
 * @return true 关闭成功
 * @return false 关闭失败
 */
bool DatabaseManager::close() {
    if (db) {
        int rc = sqlite3_close(db);
        if (rc == SQLITE_OK) {
            db = nullptr;
            dbInfo.isOpen = false;
            status = DB_NOT_INITIALIZED;
            debugPrint("数据库连接已关闭");
            return true;
        } else {
            setError("关闭数据库失败: " + String(sqlite3_errmsg(db)));
            return false;
        }
    }
    return true;
}

/**
 * @brief 检查数据库是否就绪
 * @return true 数据库就绪
 * @return false 数据库未就绪
 */
bool DatabaseManager::isReady() const {
    return (status == DB_READY && db != nullptr && dbInfo.isOpen);
}

/**
 * @brief 获取数据库状态
 * @return DatabaseStatus 数据库状态
 */
DatabaseStatus DatabaseManager::getStatus() const {
    return status;
}

/**
 * @brief 获取数据库信息
 * @return DatabaseInfo 数据库信息
 */
DatabaseInfo DatabaseManager::getDatabaseInfo() {
    if (isReady()) {
        // 更新数据库文件大小
        // 从完整路径中提取LittleFS相对路径
        String littleFSPath = dbPath.substring(9); // 去掉 "/littlefs/" 前缀
        File dbFile = LittleFS.open(littleFSPath, "r");
        if (dbFile) {
            dbInfo.dbSize = dbFile.size();
            // 获取文件最后修改时间
            time_t lastWrite = dbFile.getLastWrite();
            if (lastWrite > 0) {
                struct tm* timeinfo = localtime(&lastWrite);
                char timeStr[64];
                strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
                dbInfo.lastModified = String(timeStr);
            } else {
                dbInfo.lastModified = "未知";
            }
            dbFile.close();
        }
        
        // 更新表数量和记录数
        queryResults.clear();
        if (executeQuery("SELECT COUNT(*) as count FROM sqlite_master WHERE type='table'", queryCallback, nullptr)) {
            if (!queryResults.empty()) {
                dbInfo.tableCount = queryResults[0]["count"].toInt();
            }
        }
        
        queryResults.clear();
        if (executeQuery("SELECT (SELECT COUNT(*) FROM forward_rules) + (SELECT COUNT(*) FROM sms_records) + (SELECT COUNT(*) FROM ap_config) as total", queryCallback, nullptr)) {
            if (!queryResults.empty()) {
                dbInfo.recordCount = queryResults[0]["total"].toInt();
            }
        }
    }
    return dbInfo;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String DatabaseManager::getLastError() const {
    return lastError;
}

/**
 * @brief 获取AP配置
 * @return APConfig AP配置
 */
APConfig DatabaseManager::getAPConfig() {
    APConfig config;
    config.ssid = "ESP-SMS-Relay";
    config.password = "12345678";
    config.enabled = true;
    config.channel = 1;
    config.maxConnections = 4;
    
    if (!isReady()) {
        return config;
    }
    
    queryResults.clear();
    if (executeQuery("SELECT * FROM ap_config WHERE id = 1", queryCallback, nullptr)) {
        if (!queryResults.empty()) {
            auto& row = queryResults[0];
            config.ssid = row["ssid"];
            config.password = row["password"];
            config.enabled = row["enabled"].toInt() == 1;
            config.channel = row["channel"].toInt();
            config.maxConnections = row["max_connections"].toInt();
        }
    }
    
    return config;
}

/**
 * @brief 更新AP配置
 * @param config AP配置
 * @return true 更新成功
 * @return false 更新失败
 */
bool DatabaseManager::updateAPConfig(const APConfig& config) {
    if (!isReady()) {
        setError("数据库未就绪");
        return false;
    }
    
    String sql = "UPDATE ap_config SET ssid='" + config.ssid + "', password='" + config.password + 
                 "', enabled=" + String(config.enabled ? 1 : 0) + 
                 ", channel=" + String(config.channel) + 
                 ", max_connections=" + String(config.maxConnections) + 
                 ", updated_at='" + getCurrentTimestamp() + "' WHERE id=1";
    
    return executeSQL(sql);
}

/**
 * @brief 添加转发规则
 * @param rule 转发规则
 * @return int 规则ID，-1表示失败
 */
int DatabaseManager::addForwardRule(const ForwardRule& rule) {
    if (!isReady()) {
        setError("数据库未就绪");
        return -1;
    }
    
    String timestamp = getCurrentTimestamp();
    String sql = "INSERT INTO forward_rules (rule_name, source_number, keywords, push_type, push_config, enabled, is_default_forward, created_at, updated_at) VALUES ('" +
                  rule.ruleName + "', '" + rule.sourceNumber + "', '" + rule.keywords + "', '" + 
                  rule.pushType + "', '" + rule.pushConfig + "', " + String(rule.enabled ? 1 : 0) + ", " + String(rule.isDefaultForward ? 1 : 0) + ", '" + timestamp + "', '" + timestamp + "')";
    
    if (executeSQL(sql)) {
        return sqlite3_last_insert_rowid(db);
    }
    return -1;
}

/**
 * @brief 更新转发规则
 * @param rule 转发规则
 * @return true 更新成功
 * @return false 更新失败
 */
bool DatabaseManager::updateForwardRule(const ForwardRule& rule) {
    if (!isReady()) {
        setError("数据库未就绪");
        return false;
    }
    
    String sql = "UPDATE forward_rules SET rule_name='" + rule.ruleName + "', source_number='" + rule.sourceNumber + 
                  "', keywords='" + rule.keywords + "', push_type='" + rule.pushType + 
                  "', push_config='" + rule.pushConfig + "', enabled=" + String(rule.enabled ? 1 : 0) + 
                  ", is_default_forward=" + String(rule.isDefaultForward ? 1 : 0) + 
                  ", updated_at='" + getCurrentTimestamp() + "' WHERE id=" + String(rule.id);
    
    return executeSQL(sql);
}

/**
 * @brief 删除转发规则
 * @param ruleId 规则ID
 * @return true 删除成功
 * @return false 删除失败
 */
bool DatabaseManager::deleteForwardRule(int ruleId) {
    if (!isReady()) {
        setError("数据库未就绪");
        return false;
    }
    
    String sql = "DELETE FROM forward_rules WHERE id=" + String(ruleId);
    return executeSQL(sql);
}

/**
 * @brief 获取所有转发规则
 * @return std::vector<ForwardRule> 转发规则列表
 */
std::vector<ForwardRule> DatabaseManager::getAllForwardRules() {
    std::vector<ForwardRule> rules;
    
    if (!isReady()) {
        return rules;
    }
    
    queryResults.clear();
    if (executeQuery("SELECT id, rule_name, source_number, keywords, push_type, push_config, enabled, is_default_forward, created_at, updated_at FROM forward_rules ORDER BY id", queryCallback, nullptr)) {
        for (const auto& row : queryResults) {
            ForwardRule rule;
            rule.id = row.at("id").toInt();
            rule.ruleName = row.at("rule_name");
            rule.sourceNumber = row.at("source_number");
            rule.keywords = row.at("keywords");
            rule.pushType = row.at("push_type");
            rule.pushConfig = row.at("push_config");
            rule.enabled = row.at("enabled").toInt() == 1;
            rule.isDefaultForward = row.at("is_default_forward").toInt() == 1;
            rule.createdAt = row.at("created_at");
            rule.updatedAt = row.at("updated_at");
            rules.push_back(rule);
        }
    }
    
    return rules;
}

/**
 * @brief 根据ID获取转发规则
 * @param ruleId 规则ID
 * @return ForwardRule 转发规则
 */
ForwardRule DatabaseManager::getForwardRuleById(int ruleId) {
    ForwardRule rule;
    rule.id = -1; // 表示未找到
    
    if (!isReady()) {
        return rule;
    }
    
    queryResults.clear();
    String sql = "SELECT id, rule_name, source_number, keywords, push_type, push_config, enabled, is_default_forward, created_at, updated_at FROM forward_rules WHERE id=" + String(ruleId);
    if (executeQuery(sql, queryCallback, nullptr)) {
        if (!queryResults.empty()) {
            auto& row = queryResults[0];
            rule.id = row["id"].toInt();
            rule.ruleName = row["rule_name"];
            rule.sourceNumber = row["source_number"];
            rule.keywords = row["keywords"];
            rule.pushType = row["push_type"];
            rule.pushConfig = row["push_config"];
            rule.enabled = row["enabled"].toInt() == 1;
            rule.isDefaultForward = row["is_default_forward"].toInt() == 1;
            rule.createdAt = row["created_at"];
            rule.updatedAt = row["updated_at"];
        }
    }
    
    return rule;
}

/**
 * @brief 添加短信记录
 * @param record 短信记录
 * @return int 记录ID，-1表示失败
 */
int DatabaseManager::addSMSRecord(const SMSRecord& record) {
    if (!isReady()) {
        setError("数据库未就绪");
        return -1;
    }
    
    time_t receivedTime = record.receivedAt != 0 ? record.receivedAt : time(nullptr);
    
    String sql = "INSERT INTO sms_records (from_number, to_number, content, rule_id, forwarded, status, forwarded_at, received_at) VALUES ('" +
                 record.fromNumber + "', '" + record.toNumber + "', '" + record.content + "', " + 
                 String(record.ruleId) + ", " + String(record.forwarded ? 1 : 0) + ", '" + record.status + "', '" + 
                 record.forwardedAt + "', " + String(receivedTime) + ")";
    
    if (executeSQL(sql)) {
        return sqlite3_last_insert_rowid(db);
    }
    return -1;
}

/**
 * @brief 更新短信记录
 * @param record 短信记录
 * @return true 更新成功
 * @return false 更新失败
 */
bool DatabaseManager::updateSMSRecord(const SMSRecord& record) {
    if (!isReady()) {
        setError("数据库未就绪");
        return false;
    }
    
    String sql = "UPDATE sms_records SET from_number='" + record.fromNumber + "', to_number='" + record.toNumber + 
                 "', content='" + record.content + "', rule_id=" + String(record.ruleId) + 
                 ", forwarded=" + String(record.forwarded ? 1 : 0) + ", status='" + record.status + 
                 "', forwarded_at='" + record.forwardedAt + "', received_at=" + String(record.receivedAt) + " WHERE id=" + String(record.id);
    
    return executeSQL(sql);
}

/**
 * @brief 获取短信记录
 * @param limit 限制数量
 * @param offset 偏移量
 * @return std::vector<SMSRecord> 短信记录列表
 */
std::vector<SMSRecord> DatabaseManager::getSMSRecords(int limit, int offset) {
    std::vector<SMSRecord> records;
    
    if (!isReady()) {
        return records;
    }
    
    queryResults.clear();
    String sql = "SELECT * FROM sms_records ORDER BY received_at DESC LIMIT " + String(limit) + " OFFSET " + String(offset);
    if (executeQuery(sql, queryCallback, nullptr)) {
        for (const auto& row : queryResults) {
            SMSRecord record;
            record.id = row.at("id").toInt();
            record.fromNumber = row.at("from_number");
            record.toNumber = row.at("to_number");
            record.content = row.at("content");
            record.ruleId = row.at("rule_id").toInt();
            record.forwarded = row.at("forwarded").toInt() == 1;
            record.status = row.at("status");
            record.forwardedAt = row.at("forwarded_at");
            record.receivedAt = row.at("received_at").toInt();
            records.push_back(record);
        }
    }
    
    return records;
}

/**
 * @brief 根据ID获取短信记录
 * @param recordId 记录ID
 * @return SMSRecord 短信记录
 */
SMSRecord DatabaseManager::getSMSRecordById(int recordId) {
    SMSRecord record;
    record.id = -1; // 表示未找到
    
    if (!isReady()) {
        return record;
    }
    
    queryResults.clear();
    String sql = "SELECT * FROM sms_records WHERE id=" + String(recordId);
    if (executeQuery(sql, queryCallback, nullptr)) {
        if (!queryResults.empty()) {
            auto& row = queryResults[0];
            record.id = row["id"].toInt();
            record.fromNumber = row["from_number"];
            record.toNumber = row["to_number"];
            record.content = row["content"];
            record.ruleId = row["rule_id"].toInt();
            record.forwarded = row["forwarded"].toInt() == 1;
            record.status = row["status"];
            record.forwardedAt = row["forwarded_at"];
            record.receivedAt = row["received_at"].toInt();
        }
    }
    
    return record;
}

/**
 * @brief 删除过期的短信记录
 * @param daysOld 保留天数
 * @return int 删除的记录数
 */
int DatabaseManager::deleteOldSMSRecords(int daysOld) {
    if (!isReady()) {
        setError("数据库未就绪");
        return 0;
    }
    
    time_t cutoffTime = time(nullptr) - (daysOld * 24 * 60 * 60); // 计算截止时间戳
    String sql = "DELETE FROM sms_records WHERE received_at < " + String(cutoffTime);
    if (executeSQL(sql)) {
        return sqlite3_changes(db);
    }
    return 0;
}

/**
 * @brief 启用调试模式
 * @param enable 是否启用
 */
void DatabaseManager::setDebugMode(bool enable) {
    debugMode = enable;
    debugPrint("调试模式: " + String(enable ? "启用" : "禁用"));
}

/**
 * @brief 创建数据库表
 * @return true 创建成功
 * @return false 创建失败
 */
bool DatabaseManager::createTables() {
    debugPrint("开始创建数据库表");
    
    // 创建AP配置表
    String createAPConfigTable = 
        "CREATE TABLE IF NOT EXISTS ap_config ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "ssid TEXT NOT NULL,"
        "password TEXT NOT NULL,"
        "enabled INTEGER DEFAULT 1,"
        "channel INTEGER DEFAULT 1,"
        "max_connections INTEGER DEFAULT 4,"
        "created_at TEXT NOT NULL,"
        "updated_at TEXT NOT NULL"
        ")";
    
    if (!executeSQL(createAPConfigTable)) {
        setError("创建AP配置表失败");
        return false;
    }
    
    // 创建转发规则表
    String createForwardRulesTable = 
        "CREATE TABLE IF NOT EXISTS forward_rules ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "rule_name TEXT NOT NULL,"
        "source_number TEXT DEFAULT '*',"
        "keywords TEXT DEFAULT '',"
        "push_type TEXT NOT NULL DEFAULT 'webhook',"
        "push_config TEXT NOT NULL DEFAULT '{}',"
        "enabled INTEGER DEFAULT 1,"
        "is_default_forward INTEGER DEFAULT 0,"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT DEFAULT CURRENT_TIMESTAMP"
        ")";
    
    if (!executeSQL(createForwardRulesTable)) {
        setError("创建转发规则表失败");
        return false;
    }
    
    // 创建短信记录表
    String createSMSRecordsTable = 
        "CREATE TABLE IF NOT EXISTS sms_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "from_number TEXT NOT NULL,"
        "to_number TEXT DEFAULT '',"
        "content TEXT NOT NULL,"
        "rule_id INTEGER DEFAULT 0,"
        "forwarded INTEGER DEFAULT 0,"
        "status TEXT DEFAULT 'received',"
        "forwarded_at TEXT DEFAULT '',"
        "received_at INTEGER NOT NULL"
        ")";
    
    if (!executeSQL(createSMSRecordsTable)) {
        setError("创建短信记录表失败");
        return false;
    }
    
    // 创建索引
    executeSQL("CREATE INDEX IF NOT EXISTS idx_forward_rules_enabled ON forward_rules(enabled)");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_sms_records_from_number ON sms_records(from_number)");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_sms_records_content ON sms_records(content)");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_sms_records_received_at ON sms_records(received_at)");
    
    debugPrint("数据库表创建完成");
    return true;
}

/**
 * @brief 初始化默认数据
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool DatabaseManager::initializeDefaultData() {
    debugPrint("开始初始化默认数据");
    
    // 检查AP配置是否已存在
    queryResults.clear();
    if (executeQuery("SELECT COUNT(*) as count FROM ap_config", queryCallback, nullptr)) {
        if (!queryResults.empty() && queryResults[0]["count"].toInt() == 0) {
            // 插入默认AP配置
            String timestamp = getCurrentTimestamp();
            String insertAPConfig = "INSERT INTO ap_config (ssid, password, enabled, channel, max_connections, created_at, updated_at) VALUES ('ESP-SMS-Relay', '12345678', 1, 1, 4, '" + timestamp + "', '" + timestamp + "')";
            
            if (!executeSQL(insertAPConfig)) {
                setError("插入默认AP配置失败");
                return false;
            }
            debugPrint("默认AP配置已创建");
        }
    }
    
    debugPrint("默认数据初始化完成");
    return true;
}

/**
 * @brief 执行SQL语句
 * @param sql SQL语句
 * @return true 执行成功
 * @return false 执行失败
 */
bool DatabaseManager::executeSQL(const String& sql) {
    if (!db) {
        setError("数据库连接无效");
        return false;
    }
    
    debugPrint("执行SQL: " + sql);
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        String error = "SQL执行失败: " + String(errMsg ? errMsg : "未知错误");
        setError(error);
        if (errMsg) {
            sqlite3_free(errMsg);
        }
        return false;
    }
    
    return true;
}

/**
 * @brief 执行查询SQL语句
 * @param sql SQL语句
 * @param callback 回调函数
 * @param data 回调数据
 * @return true 执行成功
 * @return false 执行失败
 */
bool DatabaseManager::executeQuery(const String& sql, int (*callback)(void*, int, char**, char**), void* data) {
    if (!db) {
        setError("数据库连接无效");
        return false;
    }
    
    debugPrint("执行查询: " + sql);
    
    char* errMsg = nullptr;
    querySuccess = false;
    int rc = sqlite3_exec(db, sql.c_str(), callback, data, &errMsg);
    
    if (rc != SQLITE_OK) {
        String error = "查询执行失败: " + String(errMsg ? errMsg : "未知错误");
        setError(error);
        if (errMsg) {
            sqlite3_free(errMsg);
        }
        return false;
    }
    
    return true;
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void DatabaseManager::setError(const String& error) {
    lastError = error;
    debugPrint("错误: " + error);
}

/**
 * @brief 调试输出
 * @param message 调试信息
 */
void DatabaseManager::debugPrint(const String& message) {
    if (debugMode) {
        Serial.println("[DatabaseManager] " + message);
    }
}

/**
 * @brief 获取当前时间戳
 * @return String 时间戳字符串
 */
String DatabaseManager::getCurrentTimestamp() {
    // 使用millis()生成简单的时间戳
    // 在实际应用中，可以使用RTC或NTP时间
    unsigned long currentTime = millis();
    return String(currentTime);
}