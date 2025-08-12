/**
 * @file terminal_manager.cpp
 * @brief 终端管理器实现 - 提供SMS转发规则的数据库配置管理功能和CLI接口
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "terminal_manager.h"
#include "../database_manager/database_manager.h"
#include "../log_manager/log_manager.h"
#include "../push_manager/push_manager.h"
#include <regex>

// 默认配置
const TerminalConfig TerminalManager::DEFAULT_CONFIG = {
    .enabled = true,
    .maxRules = 100,
    .enableCache = true,
    .cacheSize = 50,
    .enableValidation = true,
    .enableLogging = true
};

// ==================== 单例实现 ====================

TerminalManager& TerminalManager::getInstance() {
    static TerminalManager instance;
    return instance;
}

TerminalManager::TerminalManager() 
    : initialized(false), cliRunning(false), ruleManager(nullptr), config(DEFAULT_CONFIG) {
    ruleManager = new ForwardRuleManager();
}

TerminalManager::~TerminalManager() {
    cleanup();
    if (ruleManager) {
        delete ruleManager;
        ruleManager = nullptr;
    }
}

// ==================== 初始化和清理 ====================

bool TerminalManager::initialize() {
    if (initialized) {
        return true;
    }
    
    // 初始化规则管理器
    if (!ruleManager || !ruleManager->initialize()) {
        lastError = "Failed to initialize rule manager";
        return false;
    }
    
    initialized = true;
    
    if (config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "TerminalManager initialized successfully");
    }
    
    return true;
}

void TerminalManager::cleanup() {
    if (cliRunning) {
        stopCLI();
    }
    
    if (ruleManager) {
        ruleManager->cleanup();
    }
    
    initialized = false;
    
    if (config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "TerminalManager cleaned up");
    }
}

// ==================== 转发规则管理接口实现 ====================

int TerminalManager::addForwardRule(const ForwardRule& rule) {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return -1;
    }
    
    if (config.enableValidation) {
        RuleValidationError validationResult = ruleManager->validateRule(rule);
        if (validationResult != RULE_VALID) {
            lastError = "Rule validation failed: " + 
                       ForwardRuleManager::getValidationErrorDescription(validationResult);
            return -1;
        }
    }
    
    int ruleId = ruleManager->addRule(rule);
    if (ruleId > 0 && config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "Added forward rule: " + rule.ruleName + " (ID: " + String(ruleId) + ")");
    }
    
    return ruleId;
}

bool TerminalManager::updateForwardRule(const ForwardRule& rule) {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return false;
    }
    
    if (config.enableValidation) {
        RuleValidationError validationResult = ruleManager->validateRule(rule);
        if (validationResult != RULE_VALID) {
            lastError = "Rule validation failed: " + 
                       ForwardRuleManager::getValidationErrorDescription(validationResult);
            return false;
        }
    }
    
    bool result = ruleManager->updateRule(rule);
    if (result && config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "Updated forward rule: " + rule.ruleName + " (ID: " + String(rule.id) + ")");
    }
    
    return result;
}

bool TerminalManager::deleteForwardRule(int ruleId) {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return false;
    }
    
    bool result = ruleManager->deleteRule(ruleId);
    if (result && config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "Deleted forward rule ID: " + String(ruleId));
    }
    
    return result;
}

ForwardRule TerminalManager::getForwardRule(int ruleId) {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        ForwardRule emptyRule;
        emptyRule.id = -1;
        return emptyRule;
    }
    
    return ruleManager->getRule(ruleId);
}

std::vector<ForwardRule> TerminalManager::getForwardRules(const RuleQueryCondition& condition) {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return std::vector<ForwardRule>();
    }
    
    return ruleManager->getRules(condition);
}

// ==================== 规则状态管理接口实现 ====================

bool TerminalManager::enableRule(int ruleId) {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return false;
    }
    
    bool result = ruleManager->enableRule(ruleId);
    if (result && config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "Enabled rule ID: " + String(ruleId));
    }
    
    return result;
}

bool TerminalManager::disableRule(int ruleId) {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return false;
    }
    
    bool result = ruleManager->disableRule(ruleId);
    if (result && config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "Disabled rule ID: " + String(ruleId));
    }
    
    return result;
}

bool TerminalManager::setRulePriority(int ruleId, int priority) {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return false;
    }
    
    bool result = ruleManager->setRulePriority(ruleId, priority);
    if (result && config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "Set rule ID " + String(ruleId) + " priority to " + String(priority));
    }
    
    return result;
}

// ==================== 规则测试和验证接口实现 ====================

bool TerminalManager::testRule(int ruleId, const String& sender, const String& content) {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return false;
    }
    
    return ruleManager->testRuleMatch(ruleId, sender, content);
}

bool TerminalManager::validateRuleConfig(const ForwardRule& rule) {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return false;
    }
    
    RuleValidationError result = ruleManager->validateRule(rule);
    if (result != RULE_VALID) {
        lastError = ForwardRuleManager::getValidationErrorDescription(result);
        return false;
    }
    
    return true;
}

// ==================== 统计信息接口实现 ====================

int TerminalManager::getRuleCount() {
    if (!initialized || !ruleManager) {
        return 0;
    }
    
    return ruleManager->getRuleCount();
}

int TerminalManager::getEnabledRuleCount() {
    if (!initialized || !ruleManager) {
        return 0;
    }
    
    return ruleManager->getEnabledRuleCount();
}

std::vector<ForwardRule> TerminalManager::getMostUsedRules(int limit) {
    if (!initialized || !ruleManager) {
        return std::vector<ForwardRule>();
    }
    
    return ruleManager->getMostUsedRules(limit);
}

// ==================== 批量操作接口实现 ====================

bool TerminalManager::enableAllRules() {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return false;
    }
    
    bool result = ruleManager->enableAllRules();
    if (result && config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "Enabled all rules");
    }
    
    return result;
}

bool TerminalManager::disableAllRules() {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return false;
    }
    
    bool result = ruleManager->disableAllRules();
    if (result && config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "Disabled all rules");
    }
    
    return result;
}

bool TerminalManager::deleteAllRules() {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return false;
    }
    
    bool result = ruleManager->deleteAllRules();
    if (result && config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "Deleted all rules");
    }
    
    return result;
}

bool TerminalManager::importRules(const std::vector<ForwardRule>& rules) {
    if (!initialized || !ruleManager) {
        lastError = "Terminal manager not initialized";
        return false;
    }
    
    bool result = ruleManager->importRules(rules);
    if (result && config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "Imported " + String(rules.size()) + " rules");
    }
    
    return result;
}

std::vector<ForwardRule> TerminalManager::exportRules() {
    if (!initialized || !ruleManager) {
        return std::vector<ForwardRule>();
    }
    
    return ruleManager->getRules();
}

// ==================== CLI命令行接口实现 ====================

void TerminalManager::startCLI() {
    if (!initialized) {
        Serial.println("Error: Terminal manager not initialized");
        return;
    }
    
    cliRunning = true;
    printWelcome();
    printPrompt();
    
    if (config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "CLI started");
    }
}

void TerminalManager::stopCLI() {
    cliRunning = false;
    Serial.println("\nCLI stopped.");
    
    if (config.enableLogging) {
        LogManager::getInstance().logInfo(LOG_MODULE_SYSTEM, "CLI stopped");
    }
}

bool TerminalManager::isCLIRunning() {
    return cliRunning;
}

void TerminalManager::handleSerialInput() {
    if (!cliRunning) {
        return;
    }
    
    while (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                Serial.println(); // 换行
                processCommand(inputBuffer);
                inputBuffer = "";
                printPrompt();
            }
        } else if (c == '\b' || c == 127) { // 退格键
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
                Serial.print("\b \b"); // 删除字符
            }
        } else if (c >= 32 && c <= 126) { // 可打印字符
            inputBuffer += c;
            Serial.print(c); // 回显
        }
    }
}

bool TerminalManager::processCommand(const String& command) {
    if (command.isEmpty()) {
        return true;
    }
    
    std::vector<String> args;
    String cmd = parseCommand(command, args);
    cmd.toLowerCase();
    
    // 执行命令
    if (cmd == "help" || cmd == "h") {
        executeHelpCommand(args);
    } else if (cmd == "list" || cmd == "ls") {
        executeListCommand(args);
    } else if (cmd == "add") {
        executeAddCommand(args);
    } else if (cmd == "delete" || cmd == "del" || cmd == "rm") {
        executeDeleteCommand(args);
    } else if (cmd == "enable" || cmd == "en") {
        executeEnableCommand(args);
    } else if (cmd == "disable" || cmd == "dis") {
        executeDisableCommand(args);
    } else if (cmd == "test") {
        executeTestCommand(args);
    } else if (cmd == "status" || cmd == "stat") {
        executeStatusCommand(args);
    } else if (cmd == "import") {
        executeImportCommand(args);
    } else if (cmd == "export") {
        executeExportCommand(args);
    } else if (cmd == "exit" || cmd == "quit" || cmd == "q") {
        stopCLI();
        return false;
    } else if (cmd == "clear" || cmd == "cls") {
        Serial.print("\033[2J\033[H"); // 清屏
    } else {
        Serial.println("未知命令: " + cmd);
        Serial.println("输入 'help' 查看可用命令。");
    }
    
    return true;
}

// ==================== CLI命令解析 ====================

String TerminalManager::parseCommand(const String& command, std::vector<String>& args) {
    args.clear();
    
    String trimmed = command;
    trimmed.trim();
    
    if (trimmed.isEmpty()) {
        return "";
    }
    
    // 支持引号的参数解析
    int pos = 0;
    String cmd = "";
    bool firstToken = true;
    
    while (pos < trimmed.length()) {
        // 跳过前导空格
        while (pos < trimmed.length() && trimmed.charAt(pos) == ' ') {
            pos++;
        }
        
        if (pos >= trimmed.length()) {
            break;
        }
        
        String token = "";
        bool inQuotes = false;
        char quoteChar = '\0';
        
        // 解析一个token（保留所有引号）
        while (pos < trimmed.length()) {
            char c = trimmed.charAt(pos);
            
            if (!inQuotes) {
                if (c == '"' || c == '\'') {
                    // 开始引号，保留引号字符
                    inQuotes = true;
                    quoteChar = c;
                    token += c;  // 保留引号
                    pos++;
                    continue;
                } else if (c == ' ') {
                    // 空格结束token
                    break;
                }
            } else {
                if (c == quoteChar) {
                    // 结束引号，保留引号字符
                    token += c;  // 保留引号
                    inQuotes = false;
                    pos++;
                    continue;
                } else if (c == '\\' && pos + 1 < trimmed.length()) {
                    // 转义字符
                    pos++;
                    if (pos < trimmed.length()) {
                        char nextChar = trimmed.charAt(pos);
                        if (nextChar == 'n') {
                            token += '\n';
                        } else if (nextChar == 't') {
                            token += '\t';
                        } else if (nextChar == 'r') {
                            token += '\r';
                        } else if (nextChar == '\\') {
                            token += '\\';
                        } else if (nextChar == '"') {
                            token += '"';
                        } else if (nextChar == '\'') {
                            token += '\'';
                        } else {
                            token += nextChar;
                        }
                    }
                    pos++;
                    continue;
                }
            }
            
            token += c;
            pos++;
        }
        
        // 添加解析的token（包括空字符串）
        if (firstToken) {
            cmd = token;
            firstToken = false;
        } else {
            args.push_back(token);
        }
    }
    
    return cmd;
}

String TerminalManager::parseNamedCommand(const String& command, std::map<String, String>& namedArgs, std::vector<String>& positionalArgs) {
    namedArgs.clear();
    positionalArgs.clear();
    
    String trimmed = command;
    trimmed.trim();
    
    if (trimmed.isEmpty()) {
        return "";
    }
    
    // 使用支持引号的解析方式
    std::vector<String> tokens;
    int pos = 0;
    String cmd = "";
    bool firstToken = true;
    
    while (pos < trimmed.length()) {
        // 跳过前导空格
        while (pos < trimmed.length() && trimmed.charAt(pos) == ' ') {
            pos++;
        }
        
        if (pos >= trimmed.length()) {
            break;
        }
        
        String token = "";
        bool inQuotes = false;
        char quoteChar = '\0';
        
        // 解析一个token（保留所有引号）
        while (pos < trimmed.length()) {
            char c = trimmed.charAt(pos);
            
            if (!inQuotes) {
                if (c == '"' || c == '\'') {
                    // 开始引号，保留引号字符
                    inQuotes = true;
                    quoteChar = c;
                    token += c;  // 保留引号
                    pos++;
                    continue;
                } else if (c == ' ') {
                    // 空格结束token
                    break;
                }
            } else {
                if (c == quoteChar) {
                    // 结束引号，保留引号字符
                    token += c;  // 保留引号
                    inQuotes = false;
                    pos++;
                    continue;
                } else if (c == '\\' && pos + 1 < trimmed.length()) {
                    // 转义字符
                    pos++;
                    if (pos < trimmed.length()) {
                        char nextChar = trimmed.charAt(pos);
                        if (nextChar == 'n') {
                            token += '\n';
                        } else if (nextChar == 't') {
                            token += '\t';
                        } else if (nextChar == 'r') {
                            token += '\r';
                        } else if (nextChar == '\\') {
                            token += '\\';
                        } else if (nextChar == '"') {
                            token += '"';
                        } else if (nextChar == '\'') {
                            token += '\'';
                        } else {
                            token += nextChar;
                        }
                    }
                    pos++;
                    continue;
                }
            }
            
            token += c;
            pos++;
        }
        
        if (firstToken) {
            cmd = token;
            firstToken = false;
        } else {
            tokens.push_back(token);
        }
    }
    
    // 首先处理config参数的特殊情况，直接从原始命令中提取
    int configPos = trimmed.indexOf("config=");
    if (configPos >= 0) {
        int valueStart = configPos + 7; // "config="的长度
        
        // 找到config值的结束位置
        int valueEnd = trimmed.length();
        int braceCount = 0;
        bool inQuotes = false;
        char quoteChar = '\0';
        
        // 智能解析：考虑JSON中的嵌套括号和引号
        for (int i = valueStart; i < trimmed.length(); i++) {
            char c = trimmed.charAt(i);
            
            if (!inQuotes) {
                if (c == '"' || c == '\'') {
                    inQuotes = true;
                    quoteChar = c;
                } else if (c == '{') {
                    braceCount++;
                } else if (c == '}') {
                    braceCount--;
                } else if (c == ' ' && braceCount == 0) {
                    // 只有在不在引号内且括号平衡时，空格才表示参数结束
                    valueEnd = i;
                    break;
                }
            } else {
                if (c == quoteChar) {
                    // 检查是否是转义的引号
                    bool isEscaped = false;
                    int backslashCount = 0;
                    for (int j = i - 1; j >= 0 && trimmed.charAt(j) == '\\'; j--) {
                        backslashCount++;
                    }
                    isEscaped = (backslashCount % 2 == 1);
                    
                    if (!isEscaped) {
                        inQuotes = false;
                        quoteChar = '\0';
                    }
                }
            }
        }
        
        // 提取原始JSON值，完全保留格式
        if (valueEnd > valueStart) {
            String configValue = trimmed.substring(valueStart, valueEnd);
            namedArgs["config"] = configValue;
            
            // 从原始命令中移除config参数，避免重复处理
            String beforeConfig = trimmed.substring(0, configPos);
            String afterConfig = (valueEnd < trimmed.length()) ? trimmed.substring(valueEnd) : "";
            trimmed = beforeConfig + afterConfig;
            trimmed.trim();
        }
    }
    
    // 重新解析tokens（不包含config参数）
    tokens.clear();
    pos = 0;
    firstToken = true;
    
    while (pos < trimmed.length()) {
        // 跳过前导空格
        while (pos < trimmed.length() && trimmed.charAt(pos) == ' ') {
            pos++;
        }
        
        if (pos >= trimmed.length()) {
            break;
        }
        
        String token = "";
        bool inQuotes = false;
        char quoteChar = '\0';
        
        // 解析一个token（保留所有引号）
        while (pos < trimmed.length()) {
            char c = trimmed.charAt(pos);
            
            if (!inQuotes) {
                if (c == '"' || c == '\'') {
                    // 开始引号，保留引号字符
                    inQuotes = true;
                    quoteChar = c;
                    token += c;  // 保留引号
                    pos++;
                    continue;
                } else if (c == ' ') {
                    // 空格结束token
                    break;
                }
            } else {
                if (c == quoteChar) {
                    // 结束引号，保留引号字符
                    token += c;  // 保留引号
                    inQuotes = false;
                    pos++;
                    continue;
                } else if (c == '\\' && pos + 1 < trimmed.length()) {
                    // 转义字符
                    pos++;
                    if (pos < trimmed.length()) {
                        char nextChar = trimmed.charAt(pos);
                        if (nextChar == 'n') {
                            token += '\n';
                        } else if (nextChar == 't') {
                            token += '\t';
                        } else if (nextChar == 'r') {
                            token += '\r';
                        } else if (nextChar == '\\') {
                            token += '\\';
                        } else if (nextChar == '"') {
                            token += '"';
                        } else if (nextChar == '\'') {
                            token += '\'';
                        } else {
                            token += nextChar;
                        }
                    }
                    pos++;
                    continue;
                }
            }
            
            token += c;
            pos++;
        }
        
        if (firstToken) {
            cmd = token;
            firstToken = false;
        } else {
            tokens.push_back(token);
        }
    }
    
    // 解析其他命名参数和位置参数
    for (const String& token : tokens) {
        if (token.indexOf('=') > 0) {
            // 命名参数格式: key=value
            int equalPos = token.indexOf('=');
            String key = token.substring(0, equalPos);
            String value = token.substring(equalPos + 1);
            
            namedArgs[key] = value;
        } else {
            // 位置参数
            positionalArgs.push_back(token);
        }
    }
    
    return cmd;
}

// ==================== CLI命令执行实现 ====================

void TerminalManager::executeHelpCommand(const std::vector<String>& args) {
    // 如果有参数，显示特定渠道的详细配置说明
    if (args.size() > 0) {
        String channelName = args[0];
        channelName.toLowerCase();
        showChannelConfigHelp(channelName);
        return;
    }
    
    Serial.println("\n=== ESP-SMS-Relay 终端管理器 CLI ===");
    Serial.println("可用命令:");
    Serial.println();
    Serial.println("通用命令:");
    Serial.println("  help, h [渠道名]           - 显示帮助信息，可指定渠道查看详细配置");
    Serial.println("  status, stat               - 显示系统状态");
    Serial.println("  clear, cls                 - 清屏");
    Serial.println("  exit, quit, q              - 退出CLI");
    Serial.println();
    Serial.println("规则管理:");
    Serial.println("  list, ls [enabled|disabled] - 列出转发规则");
    Serial.println("  add <名称> <发送方> <类型> <配置> [关键词] [默认转发] - 添加新规则");
    Serial.println("  delete, del, rm <id>       - 根据ID删除规则");
    Serial.println("  enable, en <id>            - 根据ID启用规则");
    Serial.println("  disable, dis <id>          - 根据ID禁用规则");
    Serial.println("  test <id> <发送方> <内容>   - 测试规则匹配");
    Serial.println();
    Serial.println("数据管理:");
    Serial.println("  import                     - 导入规则（交互式）");
    Serial.println("  export                     - 导出所有规则");
    Serial.println();
    
    // 显示可用的推送渠道
    showAvailableChannels();
    
    Serial.println("\n=== 快速配置示例 ===");
    Serial.println("企业微信:");
    Serial.println("  add \"银行提醒\" \"95588\" \"wechat\" \"{\"webhook_url\":\"https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY\"}\" \"余额\" false");
    Serial.println();
    Serial.println("微信公众号:");
    Serial.println("  add \"公众号推送\" \"95588\" \"wechat_official\" \"{\"app_id\":\"wx123456789\",\"app_secret\":\"your_app_secret\",\"open_ids\":\"openid1,openid2\",\"template_id\":\"template_id\",\"template_format\":{\"content\":{\"value\":\"{content}\",\"color\":\"#173177\"}}}\" \"余额\" false");
    Serial.println();
    Serial.println("钉钉:");
    Serial.println("  add \"钉钉通知\" \"10086\" \"dingtalk\" \"{\"webhook_url\":\"https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN\"}\" \"流量\" false");
    Serial.println();
    Serial.println("自定义Webhook:");
    Serial.println("  add \"自定义推送\" \"*\" \"webhook\" \"{\"webhook_url\":\"https://api.example.com/webhook\",\"method\":\"POST\"}\" \"\" false");
    Serial.println();
    Serial.println("💡 提示: 输入 'help 渠道名' 查看详细配置说明，例如:");
    Serial.println("  help wechat          - 查看企业微信详细配置");
    Serial.println("  help wechat_official - 查看微信公众号详细配置");
    Serial.println("  help dingtalk        - 查看钉钉详细配置");
    Serial.println("  help webhook         - 查看Webhook详细配置");
}

void TerminalManager::executeListCommand(const std::vector<String>& args) {
    RuleQueryCondition condition;
    
    // 解析参数
    if (args.size() > 0) {
        String filter = args[0];
        filter.toLowerCase();
        
        if (filter == "enabled") {
            condition.filterByEnabled = true;
            condition.enabledValue = true;
        } else if (filter == "disabled") {
            condition.filterByEnabled = true;
            condition.enabledValue = false;
        }
    }
    
    condition.orderByPriority = true;
    
    std::vector<ForwardRule> rules = getForwardRules(condition);
    
    if (rules.empty()) {
        Serial.println("未找到规则。");
        return;
    }
    
    Serial.println("\n=== 转发规则 ===");
    printRules(rules);
}

void TerminalManager::executeAddCommand(const std::vector<String>& args) {
    // 只支持命名参数格式
    executeAddCommandWithNamedParams(args);
}

void TerminalManager::executeAddCommandWithNamedParams(const std::vector<String>& args) {
    // 重新构建完整命令字符串
    String fullCommand = "add";
    for (const String& arg : args) {
        fullCommand += " " + arg;
    }
    
    // 调试输出：显示完整命令
    Serial.println("[DEBUG] Full command: '" + fullCommand + "'");
    Serial.println("[DEBUG] Command length: " + String(fullCommand.length()));
    
    // 使用支持引号的解析器解析命名参数
    std::map<String, String> namedParams;
    std::vector<String> positionalParams;
    parseNamedCommand(fullCommand, namedParams, positionalParams);
    
    // 调试输出：显示解析后的参数
    Serial.println("[DEBUG] Parsed named parameters:");
    for (auto& pair : namedParams) {
        Serial.println("[DEBUG]   " + pair.first + " = '" + pair.second + "'");
    }
    
    // 检查必需参数
    if (namedParams.find("name") == namedParams.end() || 
        namedParams.find("sender") == namedParams.end() || 
        namedParams.find("type") == namedParams.end() || 
        namedParams.find("config") == namedParams.end()) {
        
        Serial.println("用法: add name=<规则名称> sender=<发送方> type=<推送类型> config=<推送配置> [keywords=<关键词>] [default=<true/false>] [enabled=<true/false>]");
        Serial.println("示例: add name=银行提醒 sender=95588 type=wechat_official config={\"app_id\":\"wx123\",\"app_secret\":\"secret\",\"open_ids\":\"openid1,openid2\",\"template_id\":\"template123\",\"template_format\":{\"content\":{\"value\":\"{content}\",\"color\":\"#173177\"}}} keywords=余额 default=false");
        Serial.println("\n参数说明:");
        Serial.println("  name     - 规则名称 (必需)");
        Serial.println("  sender   - 发送方号码或模式 (必需)");
        Serial.println("  type     - 推送类型: wechat/wechat_official/dingtalk/webhook (必需)");
        Serial.println("  config   - JSON格式的推送配置 (必需)");
        Serial.println("  keywords - 短信内容关键词过滤 (可选)");
        Serial.println("  default  - 是否默认转发: true/false (可选，默认false)");
        Serial.println("  enabled  - 是否启用规则: true/false (可选，默认true)");
        return;
    }
    
    ForwardRule rule;
    rule.ruleName = namedParams["name"];
    rule.sourceNumber = namedParams["sender"];
    rule.pushType = namedParams["type"];
    rule.pushConfig = namedParams["config"];
    
    // 可选参数
    rule.keywords = namedParams.find("keywords") != namedParams.end() ? namedParams["keywords"] : "";
    
    // 默认转发参数
    if (namedParams.find("default") != namedParams.end()) {
        String defaultStr = namedParams["default"];
        defaultStr.toLowerCase();
        rule.isDefaultForward = (defaultStr == "true" || defaultStr == "1" || defaultStr == "yes");
    } else {
        rule.isDefaultForward = false;
    }
    
    // 启用状态参数
    if (namedParams.find("enabled") != namedParams.end()) {
        String enabledStr = namedParams["enabled"];
        enabledStr.toLowerCase();
        rule.enabled = (enabledStr == "true" || enabledStr == "1" || enabledStr == "yes");
    } else {
        rule.enabled = true;
    }
    
    addRuleAndShowResult(rule);
}

void TerminalManager::addRuleAndShowResult(const ForwardRule& rule) {
    int ruleId = addForwardRule(rule);
    if (ruleId > 0) {
        Serial.println("规则添加成功，ID: " + String(ruleId));
        Serial.println("规则详情:");
        Serial.println("  名称: " + rule.ruleName);
        Serial.println("  发送方: " + rule.sourceNumber);
        Serial.println("  推送类型: " + rule.pushType);
        Serial.println("  关键词: " + (rule.keywords.isEmpty() ? "无" : rule.keywords));
        Serial.println("  默认转发: " + String(rule.isDefaultForward ? "是" : "否"));
        Serial.println("  启用状态: " + String(rule.enabled ? "是" : "否"));
    } else {
        Serial.println("添加规则失败: " + getLastError());
    }
}

void TerminalManager::executeDeleteCommand(const std::vector<String>& args) {
    if (args.size() < 1) {
        Serial.println("用法: delete <规则ID>");
        return;
    }
    
    int ruleId = args[0].toInt();
    if (ruleId <= 0) {
        Serial.println("无效的规则ID: " + args[0]);
        return;
    }
    
    // 先显示要删除的规则
    ForwardRule rule = getForwardRule(ruleId);
    if (rule.id == -1) {
        Serial.println("未找到规则: " + String(ruleId));
        return;
    }
    
    Serial.println("正在删除规则:");
    printRule(rule);
    
    if (deleteForwardRule(ruleId)) {
        Serial.println("规则删除成功。");
    } else {
        Serial.println("删除规则失败: " + getLastError());
    }
}

void TerminalManager::executeEnableCommand(const std::vector<String>& args) {
    if (args.size() < 1) {
        Serial.println("用法: enable <规则ID>");
        return;
    }
    
    int ruleId = args[0].toInt();
    if (ruleId <= 0) {
        Serial.println("无效的规则ID: " + args[0]);
        return;
    }
    
    if (enableRule(ruleId)) {
        Serial.println("规则启用成功。");
    } else {
        Serial.println("启用规则失败: " + getLastError());
    }
}

void TerminalManager::executeDisableCommand(const std::vector<String>& args) {
    if (args.size() < 1) {
        Serial.println("用法: disable <规则ID>");
        return;
    }
    
    int ruleId = args[0].toInt();
    if (ruleId <= 0) {
        Serial.println("无效的规则ID: " + args[0]);
        return;
    }
    
    if (disableRule(ruleId)) {
        Serial.println("规则禁用成功。");
    } else {
        Serial.println("禁用规则失败: " + getLastError());
    }
}

void TerminalManager::executeTestCommand(const std::vector<String>& args) {
    if (args.size() < 3) {
        Serial.println("用法: test <规则ID> <发送方> <内容>");
        Serial.println("示例: test 1 \"95588\" \"您的余额为1000元\"");
        return;
    }
    
    int ruleId = args[0].toInt();
    if (ruleId <= 0) {
        Serial.println("无效的规则ID: " + args[0]);
        return;
    }
    
    String sender = args[1];
    String content = args[2];
    
    // 显示测试信息
    Serial.println("\n测试规则ID: " + String(ruleId));
    Serial.println("发送方: " + sender);
    Serial.println("内容: " + content);
    Serial.println();
    
    bool matches = testRule(ruleId, sender, content);
    if (matches) {
        Serial.println("✓ 规则匹配测试数据");
    } else {
        Serial.println("✗ 规则不匹配测试数据");
    }
}

void TerminalManager::executeStatusCommand(const std::vector<String>& args) {
    Serial.println("\n=== 终端管理器状态 ===");
    Serial.println("已初始化: " + String(initialized ? "是" : "否"));
    Serial.println("CLI运行中: " + String(cliRunning ? "是" : "否"));
    Serial.println("总规则数: " + String(getRuleCount()));
    Serial.println("已启用规则: " + String(getEnabledRuleCount()));
    Serial.println("已禁用规则: " + String(getRuleCount() - getEnabledRuleCount()));
    
    if (!lastError.isEmpty()) {
        Serial.println("最后错误: " + lastError);
    }
    
    Serial.println("\n配置信息:");
    Serial.println("  最大规则数: " + String(config.maxRules));
    Serial.println("  缓存启用: " + String(config.enableCache ? "是" : "否"));
    Serial.println("  验证启用: " + String(config.enableValidation ? "是" : "否"));
    Serial.println("  日志启用: " + String(config.enableLogging ? "是" : "否"));
}

void TerminalManager::executeImportCommand(const std::vector<String>& args) {
    Serial.println("导入功能尚未实现。");
    Serial.println("此功能将允许从JSON格式导入规则。");
}

void TerminalManager::executeExportCommand(const std::vector<String>& args) {
    std::vector<ForwardRule> rules = exportRules();
    
    if (rules.empty()) {
        Serial.println("没有规则可导出。");
        return;
    }
    
    Serial.println("\n=== 导出规则 (JSON格式) ===");
    Serial.println("[");
    
    for (size_t i = 0; i < rules.size(); i++) {
        const ForwardRule& rule = rules[i];
        Serial.println("  {");
        Serial.println("    \"id\": " + String(rule.id) + ",");
        Serial.println("    \"ruleName\": \"" + rule.ruleName + "\",");
        Serial.println("    \"sourceNumber\": \"" + rule.sourceNumber + "\",");
        Serial.println("    \"keywords\": \"" + rule.keywords + "\",");
        Serial.println("    \"pushType\": \"" + rule.pushType + "\",");
        Serial.println("    \"pushConfig\": " + rule.pushConfig + ",");
        Serial.println("    \"enabled\": " + String(rule.enabled ? "true" : "false") + ",");
        Serial.println("    \"isDefaultForward\": " + String(rule.isDefaultForward ? "true" : "false") + ",");
        Serial.println("    \"createdAt\": \"" + rule.createdAt + "\",");
        Serial.println("    \"updatedAt\": \"" + rule.updatedAt + "\"");
        Serial.print("  }");
        if (i < rules.size() - 1) {
            Serial.println(",");
        } else {
            Serial.println();
        }
    }
    
    Serial.println("]");
}

// ==================== CLI辅助方法 ====================

void TerminalManager::printPrompt() {
    Serial.print("sms-relay> ");
}

void TerminalManager::printWelcome() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("  ESP-SMS-Relay 终端管理器 CLI");
    Serial.println("========================================");
    Serial.println("输入 'help' 查看可用命令。");
    Serial.println("输入 'exit' 退出。");
    Serial.println();
}

void TerminalManager::printRule(const ForwardRule& rule) {
    Serial.println("  ID: " + String(rule.id));
    Serial.println("  名称: " + rule.ruleName);
    Serial.println("  来源号码: " + rule.sourceNumber);
    Serial.println("  关键词: " + rule.keywords);
    Serial.println("  推送类型: " + rule.pushType);
    Serial.println("  推送配置: " + rule.pushConfig);
    Serial.println("  启用状态: " + String(rule.enabled ? "是" : "否"));
    Serial.println("  默认转发: " + String(rule.isDefaultForward ? "是" : "否"));
    Serial.println("  创建时间: " + rule.createdAt);
    Serial.println("  更新时间: " + rule.updatedAt);
    Serial.println();
}

void TerminalManager::printRules(const std::vector<ForwardRule>& rules) {
    Serial.println("总计: " + String(rules.size()) + " 条规则\n");
    
    for (const ForwardRule& rule : rules) {
        Serial.println("[" + String(rule.id) + "] " + rule.ruleName + 
                      " (" + rule.pushType + ", " + String(rule.enabled ? "已启用" : "已禁用") + ")");
        Serial.println("    来源: " + rule.sourceNumber);
        if (!rule.keywords.isEmpty()) {
            Serial.println("    关键词: " + rule.keywords);
        }
        Serial.println("    推送配置: " + rule.pushConfig);
        Serial.println("    默认转发: " + String(rule.isDefaultForward ? "是" : "否"));
        Serial.println("    更新时间: " + rule.updatedAt);
        Serial.println();
    }
}

// ==================== 动态帮助内容生成 ====================

/**
 * @brief 显示可用的推送渠道
 */
void TerminalManager::showAvailableChannels() {
    PushManager& pushManager = PushManager::getInstance();
    std::vector<String> channels = pushManager.getAvailableChannels();
    
    if (channels.empty()) {
        Serial.println("\n❌ 暂无可用的推送渠道");
        return;
    }
    
    Serial.println("\n=== 可用推送渠道 ===");
    for (const String& channelName : channels) {
        PushChannelRegistry::ChannelMetadata metadata = pushManager.getChannelMetadata(channelName);
        Serial.println("📡 " + channelName + " - " + metadata.description);
    }
    Serial.println("\n总计: " + String(channels.size()) + " 个推送渠道");
}

/**
 * @brief 显示特定渠道的详细配置帮助
 * @param channelName 渠道名称
 */
void TerminalManager::showChannelConfigHelp(const String& channelName) {
    PushManager& pushManager = PushManager::getInstance();
    std::vector<String> channels = pushManager.getAvailableChannels();
    
    // 检查渠道是否存在
    bool channelExists = false;
    for (const String& channel : channels) {
        if (channel.equalsIgnoreCase(channelName)) {
            channelExists = true;
            break;
        }
    }
    
    if (!channelExists) {
        Serial.println("\n❌ 未找到推送渠道: " + channelName);
        Serial.println("\n可用渠道:");
        for (const String& channel : channels) {
            Serial.println("  - " + channel);
        }
        return;
    }
    
    // 获取渠道的详细帮助信息
    std::vector<PushChannelHelp> helpList = pushManager.getAllChannelHelp();
    PushChannelHelp targetHelp;
    bool helpFound = false;
    
    for (const PushChannelHelp& help : helpList) {
        if (help.channelName.equalsIgnoreCase(channelName)) {
            targetHelp = help;
            helpFound = true;
            break;
        }
    }
    
    if (!helpFound) {
        Serial.println("\n❌ 无法获取渠道 " + channelName + " 的帮助信息");
        return;
    }
    
    // 获取渠道的配置示例
    std::vector<PushChannelExample> examples = pushManager.getAllChannelExamples();
    PushChannelExample targetExample;
    bool exampleFound = false;
    
    for (const PushChannelExample& example : examples) {
        if (example.channelName.equalsIgnoreCase(channelName)) {
            targetExample = example;
            exampleFound = true;
            break;
        }
    }
    
    // 显示详细配置帮助
    String upperChannelName = channelName;
    upperChannelName.toUpperCase();
    Serial.println("\n=== " + upperChannelName + " 推送渠道详细配置 ===");
    Serial.println("📋 描述: " + targetHelp.description);
    Serial.println();
    
    // 显示配置字段说明
    if (!targetHelp.configFields.isEmpty()) {
        Serial.println("⚙️  配置字段说明:");
        Serial.println(targetHelp.configFields);
        Serial.println();
    }
    
    // 显示配置示例
    if (exampleFound && !targetExample.configExample.isEmpty()) {
        Serial.println("📝 配置示例:");
        Serial.println(targetExample.configExample);
        Serial.println();
    }
    
    // 显示规则示例
    if (!targetHelp.ruleExample.isEmpty()) {
        Serial.println("🔧 完整规则示例:");
        Serial.println(targetHelp.ruleExample);
        Serial.println();
    }
    
    // 显示使用说明
    if (exampleFound && !targetExample.usage.isEmpty()) {
        Serial.println("📖 使用说明:");
        Serial.println(targetExample.usage);
        Serial.println();
    }
    
    // 显示故障排除
    if (!targetHelp.troubleshooting.isEmpty()) {
        Serial.println("🔍 故障排除:");
        Serial.println(targetHelp.troubleshooting);
        Serial.println();
    }
    
    // 显示快速添加命令
    Serial.println("⚡ 快速添加命令模板:");
    Serial.println("add \"规则名称\" \"发送方号码\" \"" + channelName + "\" '{配置JSON}' \"关键词\" false");
    Serial.println();
    Serial.println("💡 提示: 将上述配置示例中的JSON复制到'{配置JSON}'位置，并替换YOUR_KEY等占位符为实际值");
}

/**
 * @brief 生成推送渠道帮助信息
 * @return String 动态生成的推送渠道帮助信息
 */
String TerminalManager::generateChannelHelp() {
    PushManager& pushManager = PushManager::getInstance();
    std::vector<PushChannelHelp> helpList = pushManager.getAllChannelHelp();
    
    if (helpList.empty()) {
        return "暂无可用的推送渠道。";
    }
    
    String helpContent = "\n推送渠道详细说明:\n";
    
    for (const PushChannelHelp& help : helpList) {
        helpContent += "\n=== " + help.channelName + " ===\n";
        helpContent += "描述: " + help.description + "\n";
        
        if (!help.configFields.isEmpty()) {
            helpContent += "配置字段: " + help.configFields + "\n";
        }
        
        if (!help.ruleExample.isEmpty()) {
            helpContent += "规则示例: " + help.ruleExample + "\n";
        }
        
        if (!help.troubleshooting.isEmpty()) {
            helpContent += "故障排除: " + help.troubleshooting + "\n";
        }
    }
    
    return helpContent;
}

/**
 * @brief 生成推送渠道配置示例
 * @return String 动态生成的配置示例
 */
String TerminalManager::generateChannelExamples() {
    PushManager& pushManager = PushManager::getInstance();
    std::vector<PushChannelExample> examples = pushManager.getAllChannelExamples();
    
    if (examples.empty()) {
        return "暂无可用的推送渠道配置示例。";
    }
    
    String exampleContent = "\n推送渠道配置示例:\n";
    
    for (const PushChannelExample& example : examples) {
        exampleContent += "\n" + example.channelName + "(" + example.description + "):";
        exampleContent += "\n  配置示例: " + example.configExample;
        
        if (!example.usage.isEmpty()) {
            exampleContent += "\n  使用说明: " + example.usage;
        }
        
        if (!example.helpText.isEmpty()) {
            exampleContent += "\n  帮助信息: " + example.helpText;
        }
        
        exampleContent += "\n";
    }
    
    return exampleContent;
}

// ==================== 错误处理 ====================

String TerminalManager::getLastError() {
    if (!lastError.isEmpty()) {
        return lastError;
    }
    
    if (ruleManager) {
        return ruleManager->getLastError();
    }
    
    return "无错误";
}