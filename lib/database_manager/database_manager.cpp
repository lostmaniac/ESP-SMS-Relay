/**
 * @file database_manager.cpp
 * @brief 数据库管理器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "database_manager.h"
#include "../filesystem_manager/filesystem_manager.h"
#include "../../include/constants.h"
#include <Arduino.h>
#include <time.h>
#include <mutex>

/**
 * @brief SQLite查询回调函数
 * @param data 用户数据
 * @param argc 列数
 * @param argv 列值数组
 * @param azColName 列名数组
 * @return int 0表示继续，非0表示停止
 */
static int queryCallback(void *data, int argc, char **argv, char **azColName) {
    if (!data) {
        return 1; // 错误：无效的数据指针
    }
    
    std::vector<std::map<String, String>>* results = static_cast<std::vector<std::map<String, String>>*>(data);
    std::map<String, String> row;
    
    // 添加边界检查
    if (argc < 0) {
        return 1; // 错误：无效的列数
    }
    
    for (int i = 0; i < argc; i++) {
        if (!azColName || !azColName[i]) {
            continue; // 跳过无效的列名
        }
        String colName = String(azColName[i]);
        String colValue = argv[i] ? String(argv[i]) : "";
        row[colName] = colValue;
    }
    
    results->push_back(row);
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
    bool dbExists = fsManager.fileExists(checkPath);
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
    
    debugPrint("数据库连接成功，开始配置SQLite参数");
    
    // 配置SQLite以优化ESP32-S3的内存使用
    // 设置页面大小为1KB（适合ESP32的内存特性）
    executeSQL("PRAGMA page_size = 1024");
    
    // 设置缓存大小（页面数量，1000页 = 1MB缓存）
    executeSQL("PRAGMA cache_size = 1000");
    
    // 设置临时存储为内存模式
    executeSQL("PRAGMA temp_store = MEMORY");
    
    // 设置日志模式为WAL（Write-Ahead Logging）以提高性能
    executeSQL("PRAGMA journal_mode = WAL");
    
    // 设置同步模式为NORMAL（平衡性能和安全性）
    executeSQL("PRAGMA synchronous = NORMAL");
    
    // 设置内存映射大小（限制为256KB以避免PSRAM问题）
    executeSQL("PRAGMA mmap_size = 262144");
    
    // 启用外键约束
    executeSQL("PRAGMA foreign_keys = ON");
    
    // 设置自动清理
    executeSQL("PRAGMA auto_vacuum = INCREMENTAL");
    
    debugPrint("SQLite配置完成");
    dbInfo.isOpen = true;
    dbInfo.dbPath = fullDbPath;
    
    // 无论数据库文件是否存在，都要确保表结构被正确创建
    debugPrint("开始创建/验证表结构");
    if (!createTables()) {
        setError("创建数据库表失败");
        close();
        status = DB_ERROR;
        return false;
    }
    
    // 只有在数据库文件不存在时才初始化默认数据
    if (!dbExists) {
        debugPrint("数据库文件不存在，初始化默认数据");
        // 初始化默认数据
        if (!initializeDefaultData()) {
            setError("初始化默认数据失败");
            close();
            status = DB_ERROR;
            return false;
        }
        debugPrint("新数据库初始化完成");
    } else {
        debugPrint("数据库文件已存在，跳过默认数据初始化");
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
        FilesystemManager& fsManager = FilesystemManager::getInstance();
        fs::FS& fs = fsManager.getFS();
        File dbFile = fs.open(littleFSPath, "r");
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
        std::vector<std::map<String, String>> tableResults;
        if (executeQuery("SELECT COUNT(*) as count FROM sqlite_master WHERE type='table'", queryCallback, &tableResults)) {
            if (!tableResults.empty()) {
                dbInfo.tableCount = tableResults[0]["count"].toInt();
            }
        }
        
        std::vector<std::map<String, String>> recordResults;
        if (executeQuery("SELECT (SELECT COUNT(*) FROM forward_rules) + (SELECT COUNT(*) FROM sms_records) + (SELECT COUNT(*) FROM ap_config) as total", queryCallback, &recordResults)) {
            if (!recordResults.empty()) {
                dbInfo.recordCount = recordResults[0]["total"].toInt();
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
    
    std::vector<std::map<String, String>> configResults;
    if (executeQuery("SELECT * FROM ap_config WHERE id = 1", queryCallback, &configResults)) {
        if (!configResults.empty()) {
            auto& row = configResults[0];
            config.ssid = row["ssid"];
            config.password = row["password"];
            config.enabled = row["enabled"].toInt() == 1;
            config.channel = row["channel"].toInt();
            config.maxConnections = row["max_connections"].toInt();
            config.createdAt = row["created_at"];
            config.updatedAt = row["updated_at"];
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
    
    const char* sql = "UPDATE ap_config SET ssid=?, password=?, enabled=?, channel=?, max_connections=?, updated_at=? WHERE id=1";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return false;
    }
    
    // 绑定参数 - 使用SQLITE_TRANSIENT确保字符串被复制，避免内存访问错误
    sqlite3_bind_text(stmt, 1, config.ssid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, config.password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, config.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 4, config.channel);
    sqlite3_bind_int(stmt, 5, config.maxConnections);
    String timestamp = getCurrentTimestamp();
    sqlite3_bind_text(stmt, 6, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        setError("执行SQL语句失败: " + String(sqlite3_errmsg(db)));
        return false;
    }
    
    return true;
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
    
    const char* sql = "INSERT INTO forward_rules (rule_name, source_number, keywords, push_type, push_config, enabled, is_default_forward, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return -1;
    }
    
    // 绑定参数 - 使用SQLITE_TRANSIENT确保字符串被复制，避免内存访问错误
    String timestamp = getCurrentTimestamp();
    sqlite3_bind_text(stmt, 1, rule.ruleName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, rule.sourceNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, rule.keywords.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, rule.pushType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, rule.pushConfig.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, rule.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 7, rule.isDefaultForward ? 1 : 0);
    sqlite3_bind_text(stmt, 8, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        setError("执行SQL语句失败: " + String(sqlite3_errmsg(db)));
        return -1;
    }
    
    return sqlite3_last_insert_rowid(db);
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
    
    const char* sql = "UPDATE forward_rules SET rule_name=?, source_number=?, keywords=?, push_type=?, push_config=?, enabled=?, is_default_forward=?, updated_at=? WHERE id=?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return false;
    }
    
    // 绑定参数 - 使用SQLITE_TRANSIENT确保字符串被复制，避免内存访问错误
    String timestamp = getCurrentTimestamp();
    sqlite3_bind_text(stmt, 1, rule.ruleName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, rule.sourceNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, rule.keywords.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, rule.pushType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, rule.pushConfig.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, rule.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 7, rule.isDefaultForward ? 1 : 0);
    sqlite3_bind_text(stmt, 8, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, rule.id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        setError("执行SQL语句失败: " + String(sqlite3_errmsg(db)));
        return false;
    }
    
    return true;
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
    
    const char* sql = "DELETE FROM forward_rules WHERE id=?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, ruleId);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        setError("执行SQL语句失败: " + String(sqlite3_errmsg(db)));
        return false;
    }
    
    return true;
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
    
    // 使用线程安全的方式，避免全局变量冲突
    std::vector<std::map<String, String>> localResults;
    if (executeQuery("SELECT id, rule_name, source_number, keywords, push_type, push_config, enabled, is_default_forward, created_at, updated_at FROM forward_rules ORDER BY id", queryCallback, &localResults)) {
        for (const auto& row : localResults) {
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
        setError("数据库未就绪");
        return rule;
    }
    
    const char* sql = "SELECT id, rule_name, source_number, keywords, push_type, push_config, enabled, is_default_forward, created_at, updated_at FROM forward_rules WHERE id=?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return rule;
    }
    
    sqlite3_bind_int(stmt, 1, ruleId);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        rule.id = sqlite3_column_int(stmt, 0);
        rule.ruleName = String((char*)sqlite3_column_text(stmt, 1));
        rule.sourceNumber = String((char*)sqlite3_column_text(stmt, 2));
        rule.keywords = String((char*)sqlite3_column_text(stmt, 3));
        rule.pushType = String((char*)sqlite3_column_text(stmt, 4));
        rule.pushConfig = String((char*)sqlite3_column_text(stmt, 5));
        rule.enabled = sqlite3_column_int(stmt, 6) == 1;
        rule.isDefaultForward = sqlite3_column_int(stmt, 7) == 1;
        rule.createdAt = String((char*)sqlite3_column_text(stmt, 8));
        rule.updatedAt = String((char*)sqlite3_column_text(stmt, 9));
    }
    
    sqlite3_finalize(stmt);
    return rule;
}

/**
 * @brief 获取转发规则总数
 * @return int 规则总数
 */
int DatabaseManager::getForwardRuleCount() {
    if (!isReady()) {
        setError("数据库未就绪");
        return 0;
    }
    
    const char* sql = "SELECT COUNT(*) FROM forward_rules";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return 0;
    }
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

/**
 * @brief 获取启用的转发规则数量
 * @return int 启用的规则数量
 */
int DatabaseManager::getEnabledForwardRuleCount() {
    if (!isReady()) {
        setError("数据库未就绪");
        return 0;
    }
    
    const char* sql = "SELECT COUNT(*) FROM forward_rules WHERE enabled = 1";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return 0;
    }
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
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
    
    const char* sql = "INSERT INTO sms_records (from_number, to_number, content, rule_id, forwarded, status, forwarded_at, received_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return -1;
    }
    
    // 绑定参数 - 使用SQLITE_TRANSIENT确保字符串被复制，避免内存访问错误
    sqlite3_bind_text(stmt, 1, record.fromNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, record.toNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, record.content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, record.ruleId);
    sqlite3_bind_int(stmt, 5, record.forwarded ? 1 : 0);
    sqlite3_bind_text(stmt, 6, record.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, record.forwardedAt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 8, receivedTime);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        setError("执行SQL语句失败: " + String(sqlite3_errmsg(db)));
        return -1;
    }
    
    return sqlite3_last_insert_rowid(db);
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
    
    const char* sql = "UPDATE sms_records SET from_number=?, to_number=?, content=?, rule_id=?, forwarded=?, status=?, forwarded_at=?, received_at=? WHERE id=?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return false;
    }
    
    // 绑定参数 - 使用SQLITE_TRANSIENT确保字符串被复制，避免内存访问错误
    sqlite3_bind_text(stmt, 1, record.fromNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, record.toNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, record.content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, record.ruleId);
    sqlite3_bind_int(stmt, 5, record.forwarded ? 1 : 0);
    sqlite3_bind_text(stmt, 6, record.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, record.forwardedAt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 8, record.receivedAt);
    sqlite3_bind_int(stmt, 9, record.id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        setError("执行SQL语句失败: " + String(sqlite3_errmsg(db)));
        return false;
    }
    
    return true;
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
    
    const char* sql = "SELECT id, from_number, to_number, content, rule_id, forwarded, status, forwarded_at, received_at FROM sms_records ORDER BY received_at DESC LIMIT ? OFFSET ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return records;
    }
    
    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SMSRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.fromNumber = String((char*)sqlite3_column_text(stmt, 1));
        record.toNumber = String((char*)sqlite3_column_text(stmt, 2));
        record.content = String((char*)sqlite3_column_text(stmt, 3));
        record.ruleId = sqlite3_column_int(stmt, 4);
        record.forwarded = sqlite3_column_int(stmt, 5) == 1;
        record.status = String((char*)sqlite3_column_text(stmt, 6));
        record.forwardedAt = String((char*)sqlite3_column_text(stmt, 7));
        record.receivedAt = sqlite3_column_int64(stmt, 8);
        records.push_back(record);
    }
    
    sqlite3_finalize(stmt);
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
        setError("数据库未就绪");
        return record;
    }
    
    const char* sql = "SELECT id, from_number, to_number, content, rule_id, forwarded, status, forwarded_at, received_at FROM sms_records WHERE id=?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return record;
    }
    
    sqlite3_bind_int(stmt, 1, recordId);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.fromNumber = String((char*)sqlite3_column_text(stmt, 1));
        record.toNumber = String((char*)sqlite3_column_text(stmt, 2));
        record.content = String((char*)sqlite3_column_text(stmt, 3));
        record.ruleId = sqlite3_column_int(stmt, 4);
        record.forwarded = sqlite3_column_int(stmt, 5) == 1;
        record.status = String((char*)sqlite3_column_text(stmt, 6));
        record.forwardedAt = String((char*)sqlite3_column_text(stmt, 7));
        record.receivedAt = sqlite3_column_int64(stmt, 8);
    }
    
    sqlite3_finalize(stmt);
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
    const char* sql = "DELETE FROM sms_records WHERE received_at < ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return 0;
    }
    
    sqlite3_bind_int64(stmt, 1, cutoffTime);
    
    rc = sqlite3_step(stmt);
    int deletedCount = 0;
    if (rc == SQLITE_DONE) {
        deletedCount = sqlite3_changes(db);
    } else {
        setError("执行SQL语句失败: " + String(sqlite3_errmsg(db)));
    }
    
    sqlite3_finalize(stmt);
    return deletedCount;
}

/**
 * @brief 获取短信记录总数
 * @return int 短信记录总数
 */
int DatabaseManager::getSMSRecordCount() {
    if (!isReady()) {
        setError("数据库未就绪");
        return 0;
    }
    
    const char* sql = "SELECT COUNT(*) FROM sms_records";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return 0;
    }
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

/**
 * @brief 按数量清理短信记录（保留最新的指定数量）
 * @param keepCount 保留的记录数量
 * @return int 删除的记录数
 */
int DatabaseManager::cleanupSMSRecordsByCount(int keepCount) {
    if (!isReady()) {
        setError("数据库未就绪");
        return 0;
    }
    
    // 首先获取当前记录总数
    int totalCount = getSMSRecordCount();
    if (totalCount <= keepCount) {
        debugPrint("当前记录数(" + String(totalCount) + ")未超过保留数量(" + String(keepCount) + ")，无需清理");
        return 0;
    }
    
    // 删除最旧的记录，保留最新的keepCount条
    const char* sql = "DELETE FROM sms_records WHERE id NOT IN (SELECT id FROM sms_records ORDER BY received_at DESC LIMIT ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备SQL语句失败: " + String(sqlite3_errmsg(db)));
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, keepCount);
    
    rc = sqlite3_step(stmt);
    int deletedCount = 0;
    if (rc == SQLITE_DONE) {
        deletedCount = sqlite3_changes(db);
        debugPrint("按数量清理完成，删除了 " + String(deletedCount) + " 条记录，保留最新 " + String(keepCount) + " 条");
    } else {
        setError("执行SQL语句失败: " + String(sqlite3_errmsg(db)));
    }
    
    sqlite3_finalize(stmt);
    return deletedCount;
}

/**
 * @brief 检查并执行短信记录清理（如果超过指定数量）
 * @param maxCount 最大允许的记录数量
 * @param keepCount 清理后保留的记录数量
 * @return int 删除的记录数，0表示无需清理
 */
int DatabaseManager::checkAndCleanupSMSRecords(int maxCount, int keepCount) {
    if (!isReady()) {
        setError("数据库未就绪");
        return 0;
    }
    
    int currentCount = getSMSRecordCount();
    debugPrint("当前短信记录数: " + String(currentCount) + ", 最大允许: " + String(maxCount));
    
    if (currentCount > maxCount) {
        debugPrint("短信记录数超过限制，开始清理...");
        return cleanupSMSRecordsByCount(keepCount);
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
    const char* countSql = "SELECT COUNT(*) FROM ap_config";
    sqlite3_stmt* countStmt;
    
    int rc = sqlite3_prepare_v2(db, countSql, -1, &countStmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("准备查询语句失败: " + String(sqlite3_errmsg(db)));
        return false;
    }
    
    int count = 0;
    if (sqlite3_step(countStmt) == SQLITE_ROW) {
        count = sqlite3_column_int(countStmt, 0);
    }
    sqlite3_finalize(countStmt);
    
    if (count == 0) {
        // 插入默认AP配置
        const char* insertSql = "INSERT INTO ap_config (ssid, password, enabled, channel, max_connections, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?)";
        sqlite3_stmt* insertStmt;
        
        rc = sqlite3_prepare_v2(db, insertSql, -1, &insertStmt, nullptr);
        if (rc != SQLITE_OK) {
            setError("准备插入语句失败: " + String(sqlite3_errmsg(db)));
            return false;
        }
        
        String timestamp = getCurrentTimestamp();
        // 使用SQLITE_TRANSIENT确保字符串被复制，避免内存访问错误
        sqlite3_bind_text(insertStmt, 1, DEFAULT_AP_SSID, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertStmt, 2, DEFAULT_AP_PASSWORD, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(insertStmt, 3, 1);
        sqlite3_bind_int(insertStmt, 4, DEFAULT_AP_CHANNEL);
        sqlite3_bind_int(insertStmt, 5, DEFAULT_AP_MAX_CONNECTIONS);
        sqlite3_bind_text(insertStmt, 6, timestamp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertStmt, 7, timestamp.c_str(), -1, SQLITE_TRANSIENT);
        
        rc = sqlite3_step(insertStmt);
        sqlite3_finalize(insertStmt);
        
        if (rc != SQLITE_DONE) {
            setError("插入默认AP配置失败: " + String(sqlite3_errmsg(db)));
            return false;
        }
        debugPrint("默认AP配置已创建");
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
    std::lock_guard<std::mutex> lock(dbMutex);
    
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
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (!db) {
        setError("数据库连接无效");
        return false;
    }
    
    debugPrint("执行查询: " + sql);
    
    char* errMsg = nullptr;
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